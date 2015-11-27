//======================P2MessageDrivers.c==================================
//==========================================================================
//==================Helper functions for messaging==========================
//==========================================================================
//=====These are functions for easy sending and receiving messages==========
//==========================================================================
//==========================================================================
#include "P2MessageDrivers.h"



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
			{
				FunArg fa = va_arg(args, FunArg);
				fa.fun(fa.arg);
			}
			va_end(args);
			exit(0);
		}
	
		//Parent clean-up, init, and record
		CloseFD(ptocFD);
		CloseFD(ctopFD + 1);
		InitConRec(&recBuff, ctopFD, ptocFD, child, 0, 0, NULL, DISCONNECTED);
		AddConRec(crlnList, &recBuff);
	}
	return maxFD;
}

//Helper function for populating file description set for select function
//to use to detect ready pipe
//it takes ConRecListNum to add the whole list of ctopFD[0] to set
//returns the largest fd number added.
int AppendctopFDReadToSet(fd_set *set, fd_set *exc, int num, ...)
{
	va_list args;
	va_start(args, num);
	int fdMax = 0;
	
	int i;
	for(i = 0; i < num; i++)
	{
		ConRecListNum crlnBuff = va_arg(args, ConRecListNum);
		
		int j;
		for(j = 0; j < (*crlnBuff.pNum); j++)
		{
			int target = (*crlnBuff.pList)[j].ctopFD[0];
			if(!FD_ISSET(target, exc))
			{
				FD_SET(target, set);
				fdMax = Max(fdMax, target);
			}
		}
	}

	va_end(args);
	return fdMax;
}

int AppendConnectedctopFDReadToSet(ConRecListNum crlnList, fd_set *set)
{
	int num = (*crlnList.pNum);
	ConRec *list = *crlnList.pList;
	int i;
	for(i = 0; i < num; i++)
		if(list[i].conStatus == CONNECTED)
			FD_SET(list[i].ctopFD[0], set);

	return 0;
}

int DispatchMessage(ConRecListNum crlnList, fd_set *rfds, MsgNode *msgRec,
		     int (*MsgHandler)(ConRec *, char *))
{
	char msg[MAX_BUFF_LEN];
	ConRec **pList = crlnList.pList;
	int *pNum = crlnList.pNum;
	int readFD, failCounter = 0;

	int i;
	for(i = 0; i < (*pNum); i++)
	{
		readFD = (*pList)[i].ctopFD[0];
		if(FD_ISSET(readFD, rfds))
		{
			if(ReadMessage(readFD, msg, MAX_BUFF_LEN) > 0)
			{
				char *storedMsg = InsertMsg(msgRec, msg);
				if (MsgHandler((*pList) + i, storedMsg) != -1)
					FD_CLR(readFD, rfds);
				else
					failCounter++;
			}
			else
			{
				failCounter++;
			}
		}
	}

	return failCounter;
}

//Function for running server and running message loop
int RunServer(ConRecListNum crlnPub,
	      ConRecListNum crlnSub,
	      int (*MsgHandler)(ConRec *, char *))
{	
	int i, readFD = 0, res = 0, maxFD = 0;	
	char buff[MAX_BUFF_LEN] = {0};
	MsgNode *msgRec = InitMsgList();

	while(CountDiscon(crlnPub) > 0 || CountDiscon(crlnSub) > 0)
	{
		sprintf(buff, "pub discount = %d, sub discount = %d", 
				CountDiscon(crlnPub),
				CountDiscon(crlnSub));
		perror(buff);

		fd_set rfds;
		fd_set efds;
		FD_ZERO(&rfds);
		FD_ZERO(&efds);

		AppendConnectedctopFDReadToSet(crlnPub, &efds);
		AppendConnectedctopFDReadToSet(crlnSub, &efds);
		if((res = WaitForMessageLists(&rfds, &efds, 2, crlnPub, crlnSub)) < 0)
			return -1;

		sprintf(buff, "::::::::::::::res = %d", res);
		perror(buff);

		DispatchMessage(crlnPub, &rfds, msgRec, MsgHandler);
		CleanUpList(crlnPub, &rfds);
		DispatchMessage(crlnSub, &rfds, msgRec, MsgHandler);
		CleanUpList(crlnSub, &rfds);
	}
	perror("Out of RunServer");

	WaitForChildren();
	UnInitMsgList(msgRec);
	return 0;
}
