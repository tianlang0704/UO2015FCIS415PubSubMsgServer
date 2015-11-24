//======================P2MessageDrivers.c==================================
//==========================================================================
//==================Helper functions for messaging==========================
//==========================================================================
//=====These are functions for easy sending and receiving messages==========
//==========================================================================
//==========================================================================
#include "P2MessageDrivers.h"

int ReadMessage(int readFD, char *out, int maxLen)
{
	int res = 0;
	struct pollfd poFD = {readFD, POLLIN, 0};

	poll(&poFD, 1 , -1);
	res = read(readFD, out, maxLen);

	out[res] = 0;
	return res;
}

int SendMessage(int writeFD, const char *msg)
{
	int len = strlen(msg), res = 0;
	struct sigaction OriginalAct;

	sigaction(SIGPIPE, NULL, &OriginalAct);
	signal(SIGPIPE, SIG_IGN);
	res = write(writeFD, msg, len);
	sigaction(SIGPIPE, &OriginalAct, NULL);
	
	return res;
}

int SyncSendMessage(int readFD, 
		    int writeFD, 
		    const char *msg, 
		    char *out, 
		    int maxLen)
{
	SendMessage(writeFD, msg);
	return ReadMessage(readFD, out, maxLen);
}

//Helper function for spawning children
//num: 		The number of children to spawn
//**saveList:	The list to save the children info
//*itemNum:	Output item num
//*itemMax:	Output list max
//*fun:		Child function to run
//numFree:	Number of things to free after child execute *fun
//...:		Pointers to things to free after child execute *fun
//return: the largest FD number of all chlidres'
int SpawnChild(int num,
	       ConRecListNum crlnList,
	       int (*fun) (int, int),
	       int numFree,
	       ...)
{
	int ctopFD[2], ptocFD[2], maxFD = 0;
	ConRec recBuff;
	va_list args;
	char buff[250];

	int i, j;
	for(i = 0; i < num; i++)
	{
		if(pipe2(ctopFD, O_NONBLOCK) < 0 || pipe2(ptocFD, O_NONBLOCK) < 0)
		{
			perror("Error piping");
			exit(EXIT_FAILURE);
		}
		maxFD = Max(maxFD,
			    Max(Max(ctopFD[0], ctopFD[1]), 
				Max(ptocFD[0], ptocFD[1])));

		pid_t child = fork();
		if(child < 0)
		{
			perror("Error forking");
			exit(EXIT_FAILURE);
		}
		else if(child == 0)
		{
			sprintf(buff, "chlid %d pid: %d", i, getpid());
			perror(buff);
			CloseFD(ctopFD);
			CloseFD(ptocFD + 1);
			fun(ptocFD[0], ctopFD[1]);

			//Child clean-up
			CloseFD(ctopFD + 1);
			CloseFD(ptocFD);
			FreeConRecLists(1, crlnList);	
			va_start(args, numFree);
			for(j = 0; j < numFree; j++)
				FreeConRecLists(1, va_arg(args, ConRecListNum));
			va_end(args);
			exit(0);
		}
	
		//Parent clean-up, init, and record
		CloseFD(ptocFD);
		CloseFD(ctopFD + 1);
		memcpy(recBuff.ctopFD, ctopFD, 2 * sizeof(int));
		memcpy(recBuff.ptocFD, ptocFD, 2 * sizeof(int));
		recBuff.pid = child;
		recBuff.topicMax = 0;
		recBuff.topicNum = 0;
		recBuff.topic = NULL;
		AddConRec(crlnList, &recBuff);
	}
	return maxFD;
}

//Helper function for populating file description set for select function
//to use to detect ready pipe
int AppendFDSet(fd_set *set, int num, ...)
{
	va_list args;
	va_start(args, num);
	int count = 0;
	
	int i;
	for(i = 0; i < num; i++)
	{
		ConRecListNum crlnBuff = va_arg(args, ConRecListNum);
		
		int j;
		for(j = 0; j < (*crlnBuff.pNum); j++)
		{
			FD_SET((*crlnBuff.pList)[j].ctopFD[0], set);
			count++;
		}
	}

	va_end(args);
	return count;
}

int WaitForMessage(int targetFD)
{	
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(targetFD, &rfds);

	return select(targetFD + 1, &rfds, NULL, NULL, NULL);
}

int WaitForMessageLists(fd_set *rfds, int num, ...)
{	
	FD_ZERO(rfds);
	va_list args;
	va_start(args, num); 
	int maxFD = 0;

	int i, j;
	for(i = 0; i < num; i++)
	{
		ConRecListNum crlnList = va_arg(args, ConRecListNum);
		ConRec **pList = crlnList.pList;
		int *pNum = crlnList.pNum;

		for(j = 0; j < (*pNum); j++)
			maxFD = Max(maxFD,
			   	Max(Max((*pList)[j].ctopFD[0], (*pList)[j].ctopFD[1]), 
				    Max((*pList)[j].ptocFD[0], (*pList)[j].ptocFD[1])));
	
		AppendFDSet(rfds, 1, crlnList);
	}
	va_end(args);

	return select(maxFD + 1, rfds, NULL, NULL, NULL);
}

int DispatchMessage(ConRecListNum crlnList, fd_set *rfds, 
		     int (*MsgHandler)(ConRec *, const char *))
{
	char msg[MAX_BUFF_LEN];
	ConRec **pList = crlnList.pList;
	int *pNum = crlnList.pNum;
	int readFD, failCounter = 0;

	int i;
	for(i = 0; i < (*pNum); i++)
	{
		readFD = (*pList)[i].ctopFD[0];
		char buff[MAX_BUFF_LEN];
		sprintf(buff, "readFD = %d", readFD);
		perror(buff);
		if(FD_ISSET(readFD, rfds))
		{
			if(ReadMessage(readFD, msg, MAX_BUFF_LEN) > 0)
			{
				MsgHandler((*pList) + i, msg);
				FD_CLR(readFD, rfds);
			}
			else
			{
				failCounter++;
			}
		}
	}

	return failCounter;
}

int CleanUpList(ConRecListNum crlnList, fd_set *rfds)
{
	ConRec *list = (*crlnList.pList);
	int num = (*crlnList.pNum);

	int i;
	for(i = 0; i < num; i++)
		if(FD_ISSET(list[i].ctopFD[0], rfds))
			RemoveConRec(crlnList, list + i);
}

//Function for running server and running message loop
int RunServer(ConRecListNum crlnPub,
	      ConRecListNum crlnSub,
	      int (*MsgHandler)(ConRec *, const char *));
{	
	ConRec **pPubList = crlnPub.pList;
	ConRec **pSubList = crlnSub.pList; 
	int *pPubNum = crlnPub.pNum;
	int *pSubNum = crlnSub.pNum;
	int i, readFD = 0, res = 0, maxFD = 0;	
	char buff[MAX_BUFF_LEN] = {0};

	while((*pPubNum) > 0 || (*pSubNum) > 0)
	{
		fd_set rfds;
		if(WaitForMessageLists(&rfds, 2, crlnPub, crlnSub) < 0)
			return -1;

		sprintf(buff, "::::::::::::::res = %d", res);
		perror(buff);

		DispatchMessage(crlnPub, &rfds, MsgHandler);
		CleanUpList(crlnPub, &rfds);
		DispatchMessage(crlnSub, &rfds, MsgHandler);
		CleanUpList(crlnSub, &rfds);
	}
	return 0;
}
