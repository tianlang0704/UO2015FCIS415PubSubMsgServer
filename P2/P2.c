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
int ServerMsgHandler(ConRec *target, char *msg)
{
	char buff[250];
	sprintf(buff, "Message received: %s", msg);
	perror(buff);
	return SendMessage(target->ptocFD[1], "accept");
}



int main()
{
	ConRec *pubList = NULL;
	int pubMax = INIT_LIST_MAX;
	int pubNum = 0;
	ConRec *subList = NULL;
	int subMax = INIT_LIST_MAX;
	int subNum = 0;

	char buff[MAX_BUFF_LEN] = {0};
	sprintf(buff, "parent pid: %d", getpid());
	perror(buff);
	
	SpawnChild(3, &pubList, &pubNum, &pubMax, PubMsgHandler, 2, pubList, subList);
	SpawnChild(2, &subList, &subNum, &subMax, SubMsgHandler, 2, pubList, subList);
	RunServer(&pubList, &subList, &pubNum, &subNum, ServerMsgHandler);

	CloseList(pubList, pubNum);
	CloseList(subList, subNum);

	FreeMems(2, pubList, subList);
	return 0;
}
