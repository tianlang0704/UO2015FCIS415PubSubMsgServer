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
	       ConRec **saveList,
	       int *itemNum, 
	       int *itemMax, 
	       int (*fun) (int, int),
	       int numFree,
	       ...)
{
	int ctopFD[2], ptocFD[2], maxFD = 0;
	char buff[250];
	ConRec recBuff;
	va_list args;

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
			FreeMems(1, (*saveList));	
			va_start(args, numFree);
			for(j = 0; j < numFree; j++)
				FreeMems(1, va_arg(args, void *));
			va_end(args);
			exit(0);
		}
	
		//Parent clean-up
		CloseFD(ptocFD);
		CloseFD(ctopFD + 1);
		memcpy(recBuff.ctopFD, ctopFD, 2 * sizeof(int));
		memcpy(recBuff.ptocFD, ptocFD, 2 * sizeof(int));
		recBuff.pid = child;
		AddConRec(saveList, itemNum, itemMax, &recBuff);
	}
	return maxFD;
}

//Helper function for populating file description set for select function
//to use to detect ready pipe
int PopulateFDSet(fd_set *set, int num, ...)
{
	va_list args;
	va_start (args, num);
	int count = 0;
	
	int i;
	for(i = 0; i < num; i++)
	{
		ConRecListNum crlnBuff = va_arg(args, ConRecListNum);
		
		int j;
		for(j = 0; j < crlnBuff.num; j++)
		{
			FD_SET(crlnBuff.pList[j].ctopFD[0], set);
			count++;
		}
	}

	va_end(args);
	return count;
}

//Function for running server and run message loop
int RunServer(ConRec **pPubList, 
	      ConRec **pSubList, 
	      int *pPubNum, 
	      int *pSubNum,
	      int (*MsgHandler)(ConRec *, char *))
{	
	fd_set rfds;	
	int i, readFD = 0, res = 0, maxFD = 0;	
	char buff[MAX_BUFF_LEN] = {0};

	for(i = 0; i < (*pPubNum); i++)
		maxFD = Max(maxFD,
			   Max(Max((*pPubList)[i].ctopFD[0], (*pPubList)[i].ctopFD[1]), 
			       Max((*pPubList)[i].ptocFD[0], (*pPubList)[i].ptocFD[1])));
	for(i = 0; i < (*pSubNum); i++)
		maxFD = Max(maxFD,
			   Max(Max((*pSubList)[i].ctopFD[0], (*pSubList)[i].ctopFD[1]), 
			       Max((*pSubList)[i].ptocFD[0], (*pSubList)[i].ptocFD[1])));

	while((*pPubNum) > 0 || (*pSubNum) > 0)
	{
		ConRecListNum crlnPub = {(*pPubNum), (*pPubList)};
		ConRecListNum crlnSub = {(*pSubNum), (*pSubList)};
		FD_ZERO(&rfds);
		PopulateFDSet(&rfds, 2, crlnPub, crlnSub);
		res = select(maxFD + 1, &rfds, NULL, NULL, NULL);
		sprintf(buff, "::::::::::::::res = %d", res);
		perror(buff);
		if(res > 0)
		{
			for(i = 0; i < (*pPubNum); i++)
			{
				readFD = (*pPubList)[i].ctopFD[0];
				sprintf(buff, "readFD = %d", readFD);
				perror(buff);
				if(FD_ISSET(readFD, &rfds))
				{
					if(ReadMessage(readFD, buff, MAX_BUFF_LEN) > 0)
						MsgHandler((*pPubList) + i, 
								     buff);
					else
						RemoveConRec((*pPubList), 
							     pPubNum,
							     (*pPubList) + i);
				}
			}

			for(i = 0; i < (*pSubNum); i++)
			{
				readFD = (*pSubList)[i].ctopFD[0];
				if(FD_ISSET(readFD, &rfds))
				{
					if(ReadMessage(readFD, buff, MAX_BUFF_LEN) > 0)
						MsgHandler((*pSubList) + i, 
								     buff);
					else
						RemoveConRec((*pSubList),
							     pSubNum,
							     (*pSubList) + i);
				}
			}
		}
		else
		{
			return -1;
		}
	}

	return 0;
}
