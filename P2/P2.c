#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "P2Structs.h"
#include "P2Helpers.h"
#include "P2MessageDrivers.h"
#include "P2ListManagement.h"
#include "P2TopicManagement.h"
#include "P2ThreadManagement.h"

#define INIT_LIST_MAX 20

ThreadInfo *m_TList;

int PubMsgHandler(int readFD, int writeFD)
{
	char msg[MAX_BUFF_LEN];
	sprintf(msg, "pub %d connect", getpid());
	SyncSendMessage(readFD, writeFD, msg, msg, MAX_BUFF_LEN);
	AssertStr(msg, "accept");
	sprintf(msg, "pub %d topic 1", getpid());
	SyncSendMessage(readFD, writeFD, msg, msg, MAX_BUFF_LEN);
	AssertStr(msg, "accept");
	SyncSendMessage(readFD, writeFD, "terminate", msg, MAX_BUFF_LEN);
	AssertStr(msg, "accept");
	return 0;
}

int SubMsgHandler(int readFD, int writeFD)
{
	char msg[MAX_BUFF_LEN];
	sprintf(msg, "sub %d connect", getpid());
	SyncSendMessage(readFD, writeFD, msg, msg, MAX_BUFF_LEN);
	AssertStr(msg, "accept");
	sprintf(msg, "sub %d topic 2", getpid());
	SyncSendMessage(readFD, writeFD, msg, msg, MAX_BUFF_LEN);
	AssertStr(msg, "accept");
	SyncSendMessage(readFD, writeFD, "terminate", msg, MAX_BUFF_LEN);
	AssertStr(msg, "accept");
	return 0;
}

void *ThreadMessageHandler(void *arg)
{
	ConRecMsg *senderMsg = arg;
	int readFD = senderMsg->pConRec->ctopFD[0];
	int writeFD = senderMsg->pConRec->ptocFD[1];
	char buff[MAX_BUFF_LEN] = {0}, msg[MAX_BUFF_LEN] = {0};

	while(strcmp(msg, "terminate") != 0)
	{
		WaitForMessage(readFD);
		ReadMessage(readFD, msg, MAX_BUFF_LEN);
		
		sprintf(buff, "--------------------msg received:\n   %s", msg);
		perror(buff);
		SendMessage(writeFD, "accept");
	}

	pthread_exit(0);
}

//ServerMessage handler
int ServerMsgHandler(ConRec *sender, char *msg)
{
	char conType[10], infoType[10];
	int topicBuff;
	
	sscanf(msg, "%s %*s %s %d", conType, infoType, &topicBuff);
	if(strcmp(infoType, "connect") == 0)
	{
		sender->conStatus = CONNECTED;
		SpawnThread(m_TList, ThreadMessageHandler, (ConRecMsg){sender, msg});
	}
	
	return SendMessage(sender->ptocFD[1], "accept");
}

int main()
{
	m_TList = InitThreadInfoList();
	ConRec *pubList = NULL;
	int pubMax = INIT_LIST_MAX;
	int pubNum = 0;
	ConRecListNum crlnPub = {&pubNum, &pubMax, &pubList};
	ConRec *subList = NULL;
	int subMax = INIT_LIST_MAX;
	int subNum = 0;
	ConRecListNum crlnSub = {&subNum, &subMax, &subList}; 

	char buff[MAX_BUFF_LEN] = {0};
	sprintf(buff, "parent pid: %d", getpid());
	perror(buff);
	
	//SpawnChild args:number to spawn, list to save, msg handler, 
	//                number of uninit functions to call, 
	//		  uinit function & arguments 
	SpawnChild(3, crlnPub, PubMsgHandler, 3, 
		   (FunArg){EmptyConRec, &crlnPub}, 
		   (FunArg){EmptyConRec, &crlnSub},
		   (FunArg){UnInitThreadInfoList, m_TList});
	SpawnChild(2, crlnSub, SubMsgHandler, 3, 
		   (FunArg){EmptyConRec, &crlnPub}, 
		   (FunArg){EmptyConRec, &crlnSub},
		   (FunArg){UnInitThreadInfoList, m_TList});
	RunServer(crlnPub, crlnSub, ServerMsgHandler);

	CloseList(pubList, pubNum);
	CloseList(subList, subNum);
	FreeConRecLists(2, crlnPub, crlnSub);
	UnInitThreadInfoList(m_TList);
	return 0;
}
