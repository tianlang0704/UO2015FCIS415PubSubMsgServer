#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "P2Settings.h"
#include "P2Structs.h"
#include "P2Helpers.h"
#include "P2MessageDrivers.h"
#include "P2ListManagement.h"
#include "P2ThreadManagement.h"
#include "P2TopicQManagement.h"
#include "P2TopicQListManagement.h"

ThreadInfo *m_ThreadList;
TopicQ *m_TopicStore;
int m_TopicNum;

//Pub process action script
int PubMsgHandler(pid_t pidPub, int readFD, int writeFD)
{
	char msg[MAX_BUFF_LEN];
	sprintf(msg, "pub %d connect", getpid());
	print(msg);
	SyncSendMessage(readFD, writeFD, msg, msg, MAX_BUFF_LEN);
	AssertStr(msg, MSG_ACCEPT);

    int i;
    for(i = 0; i < m_TopicNum; i++)
    {
        sleep(MESSAGE_SENDING_INTERVAL);
        sprintf(msg, "topic %d pub:%d,sendtime:%d", i + 1, getpid(), (int)time(NULL));
        print(msg);
        do{
        SyncSendMessage(readFD, writeFD, msg, msg, MAX_BUFF_LEN);
        }while(strcmp(msg, MSG_ACCEPT) != 0);
    }

	SyncSendMessage(readFD, writeFD, MSG_TERMINATE, msg, MAX_BUFF_LEN);
	AssertStr(msg, MSG_ACCEPT);
	return 0;
}

//Sub process action script
int SubMsgHandler(pid_t pidSub, int readFD, int writeFD)
{
	char msg[MAX_BUFF_LEN];
	char buff[MAX_BUFF_LEN];

	sprintf(msg, "sub %d connect", getpid());
	print(msg);
	SyncSendMessage(readFD, writeFD, msg, msg, MAX_BUFF_LEN);
	AssertStr(msg, MSG_ACCEPT);

    int topicNum = m_TopicNum > 1 ? m_TopicNum : 2;
    int i;
    for(i = 0; i < topicNum - 1; i++)
    {
        sprintf(msg, "sub %d topic %d", getpid(), i + 1);
#ifdef VERBOSE
        print(msg);
#endif
        SyncSendMessage(readFD, writeFD, msg, msg, MAX_BUFF_LEN);
        AssertStr(msg, MSG_ACCEPT);
    }

	SyncSendMessage(readFD, writeFD, "end", msg, MAX_BUFF_LEN);
	AssertStr(msg, MSG_ACCEPT);

    SendMessage(writeFD, MSG_SUB_RECEIVING);
    while(ReadMessage(readFD, msg, MAX_BUFF_LEN) && strcmp(msg, MSG_TERMINATE) != 0)
    {
        sprintf(buff, "Sub %d: %s, receivetime:%d", pidSub, msg, (int)time(NULL));
        print(buff);
        SendMessage(writeFD, MSG_SUCCESS);
        sleep(MESSAGE_RECEIVING_INTERVAL);
    }

	return 0;
}

//Pub thread proxy action script
void *PubThreadHandler(void *arg)
{
	ListConRec *listCR = arg;
	ConRecListNum crlnList = listCR->crlnList;
	int readFD = listCR->pConRec->ctopFD[0];
	int writeFD = listCR->pConRec->ptocFD[1];
	int topicID;
	int senderID = listCR->pConRec->pid;
	char buff[MAX_BUFF_LEN] = {0};
	char msg[MAX_BUFF_LEN] = {0};
	char content[MAX_BUFF_LEN] = {0};

	while(strcmp(msg, MSG_TERMINATE) != 0)
	{
		WaitForMessage(readFD);
		ReadMessage(readFD, msg, MAX_BUFF_LEN);

        sscanf(msg, "%s", buff);
		if(strcmp(buff, MSG_TOPIC) == 0)    //dealing with sending topic
		{
            sscanf(msg, "%*s %d %s", &topicID, content);
#ifdef VERBOSE
            sprintf(buff, "pub %d enqueuing entry, topicID: %d, content: %s",
                    senderID, topicID, content);
            print(buff);
#endif
            if(Enq(&m_TopicStore, topicID, senderID, content) != NULL)
                SendMessage(writeFD, MSG_ACCEPT);
            else
                SendMessage(writeFD, MSG_RETRY);
		}
        else
            SendMessage(writeFD, MSG_ACCEPT);
	}

    if(SyncDisconnect(crlnList, listCR->pConRec) == 0)
    {
#ifdef VERBOSE
        print("boread casting termination message.");
#endif
        Enq(&m_TopicStore, MSG_TOPIC_BROADCAST, senderID, MSG_TERMINATE);
    }

	pthread_exit(0);
}

//Sub thread proxy action script
void *SubThreadHandler(void *arg)
{
	ListConRec *listCR = arg;
	int readFD = listCR->pConRec->ctopFD[0];
	int writeFD = listCR->pConRec->ptocFD[1];
	int topicID;
	char buff[MAX_BUFF_LEN] = {0};
	char msg[MAX_BUFF_LEN] = {0};

    //subscribing topics
    //Everysub subs topic 0 first, it's the broadcast channel.
    SubToTopic(&m_TopicStore, listCR->pConRec, MSG_TOPIC_BROADCAST);
	while(strcmp(msg, "end") != 0)
	{
		WaitForMessage(readFD);
		ReadMessage(readFD, msg, MAX_BUFF_LEN);
        sscanf(msg, "%*s %*s %s", buff);
        if(strcmp(buff, MSG_TOPIC) == 0)
        {
				sscanf(msg, "%*s %*s %*s %d", &topicID);
				SubToTopic(&m_TopicStore, listCR->pConRec, topicID);
				SendMessage(writeFD, MSG_ACCEPT);
        }
        else
            SendMessage(writeFD, MSG_ACCEPT);
	}

    //start to read messages from topic store and send them
    ReadMessage(readFD, msg, MAX_BUFF_LEN);
    if(strcmp(msg, MSG_SUB_RECEIVING) == 0)
    {
        while(WaitForTopics(m_TopicStore, listCR->pConRec, &topicID) > -1)
        {
            TopicEntry *tempEntry = Deq(&m_TopicStore, topicID, listCR->pConRec, MESSAGE_TIMEOUT);
            if(tempEntry != NULL)
            {
#ifdef VERBOSE
                sprintf(buff, "sub %d dequeuing entry, topicID: %d, content: %s",
                        listCR->pConRec->pid, topicID, tempEntry->data);
                print(buff);
#endif
                if(strcmp(tempEntry->data, MSG_TERMINATE) == 0)
                {
                //break the waiting loop if termination message is received
                    SyncSendMessage(readFD, writeFD, MSG_TERMINATE, msg, MAX_BUFF_LEN);
                    break;
                }
                else
                {
                //send the topic and content to the sub
                    sprintf(buff, "topic %d %s", topicID, tempEntry->data);
                    SyncSendMessage(readFD, writeFD, buff, msg, MAX_BUFF_LEN);
                }
            }
        }
    }

	pthread_exit(0);
}

//Server thread spawnder script
int ServerMsgHandler(ConRecListNum crlnList, ConRec *sender, char *msg)
{
	char senderType[10], infoType[10];
	int topicBuff;

	sscanf(msg, "%s %*s %s %d", senderType, infoType, &topicBuff);
	if(strcmp(senderType, MSG_PUB_IDENTIFIER) == 0)
	{
        if(strcmp(infoType, MSG_CONNECT) == 0)
        {
            sender->conStatus = CONNECTED;
            SpawnThread(m_ThreadList, PubThreadHandler, (ListConRec){crlnList, sender});
        }
	}

	if(strcmp(senderType, MSG_SUB_IDENTIFIER) == 0)
	{
        if(strcmp(infoType, MSG_CONNECT) == 0)
        {
            sender->conStatus = CONNECTED;
            SpawnThread(m_ThreadList, SubThreadHandler, (ListConRec){crlnList, sender});

        }
	}

    char buff[MAX_BUFF_LEN];
    sprintf(buff, "child %d connected and proxy thread created", sender->pid);
    print(buff);
	return SendMessage(sender->ptocFD[1], MSG_ACCEPT);
}

int main(int argc, char**argv)
{
	m_ThreadList = InitThreadInfoList();
	m_TopicStore = InitTopicQList();
	m_TopicNum = atoi(argv[3]);
	ConRec *pubList = NULL;
	int pubMax = INIT_LIST_MAX;
	int pubNum = 0;
	ConRecListNum crlnPub = {&pubNum, &pubMax, &pubList};
	ConRec *subList = NULL;
	int subMax = INIT_LIST_MAX;
	int subNum = 0;
	ConRecListNum crlnSub = {&subNum, &subMax, &subList};

	char buff[MAX_BUFF_LEN] = {0};
	sprintf(buff, "main process start, pid: %d", getpid());
	print(buff);

	//SpawnChild args:number to spawn, list to save, msg handler,
	//                number of uninit functions to call,
	//		  uinit function & arguments
	SpawnChild(atoi(argv[1]), crlnPub, PubMsgHandler, 3,  //*TODO:create funarg set
		   (FunArg){EmptyConRec, &crlnPub},
		   (FunArg){EmptyConRec, &crlnSub},
		   (FunArg){UnInitThreadInfoList, m_ThreadList},
		   (FunArg){UnInitTopicQList, m_TopicStore});
	SpawnChild(atoi(argv[2]), crlnSub, SubMsgHandler, 3,
		   (FunArg){EmptyConRec, &crlnPub},
		   (FunArg){EmptyConRec, &crlnSub},
		   (FunArg){UnInitThreadInfoList, m_ThreadList},
		   (FunArg){UnInitTopicQList, m_TopicStore});
    //Start server
	RunServer(crlnPub, crlnSub, ServerMsgHandler);

    //Clean up everything
	CloseList(pubList, pubNum);
	CloseList(subList, subNum);
	FreeConRecLists(2, crlnPub, crlnSub);
	UnInitTopicQList(m_TopicStore);
	UnInitThreadInfoList(m_ThreadList);
	return 0;
}
