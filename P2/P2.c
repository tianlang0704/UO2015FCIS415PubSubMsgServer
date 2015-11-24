#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "P2Structs.h"
#include "P2Helpers.h"
#include "P2MessageDrivers.h"

#define INIT_LIST_MAX 20

int PubMsgHandler(int readFD, int writeFD)
{
	char msg[MAX_BUFF_LEN];
	sprintf(msg, "pub %d connect", getpid());
	SyncSendMessage(readFD, writeFD, msg, msg, MAX_BUFF_LEN);
	AssertStr(msg, "accept");
	sprintf(msg, "pub %d topic 1", getpid());
	SyncSendMessage(readFD, writeFD, msg, msg, MAX_BUFF_LEN);
	AssertStr(msg, "accept");
	SyncSendMessage(readFD, writeFD, "end", msg, MAX_BUFF_LEN);
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
	SyncSendMessage(readFD, writeFD, "end", msg, MAX_BUFF_LEN);
	AssertStr(msg, "accept");
	return 0;
}

//ServerMessage handler
int ServerMsgHandler(ConRec *sender, const char *msg)
{
	char conType[10], infoType[10];
	int topicBuff;
	
	sscanf(msg, "%s %*s %s %d", conType, infoType, &topicBuff);
	if(strcmp(conType, "pub") == 0)
		if(strcmp(infoType, "topic") == 0)
			AddTopic(sender, topicBuff);
	
	return SendMessage(sender->ptocFD[1], "accept");
}



int main()
{
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
	//                number of list to free, lists to free after child exe 
	SpawnChild(3, crlnPub, PubMsgHandler, 2, crlnPub, crlnSub);
	SpawnChild(2, crlnSub, SubMsgHandler, 2, crlnPub, crlnSub);
	RunServer(crlnPub, crlnSub, ServerMsgHandler, SYNC);

	WaitForChildren();
	CloseList(pubList, pubNum);
	CloseList(subList, subNum);
	FreeConRecLists(2, crlnPub, crlnSub);
	return 0;
}
