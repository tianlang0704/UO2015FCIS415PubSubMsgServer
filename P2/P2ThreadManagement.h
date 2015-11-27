#ifndef CIS415_P2_THREADMANAGEMENT
#define CIS415_P2_THREADMANAGEMENT
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "P2Structs.h"


typedef struct ThreadInfoTag
{
	struct ThreadInfoTag *head;
	struct ThreadInfoTag *tail;
	struct ThreadInfoTag *prev;
	struct ThreadInfoTag *next;
	pthread_t tid;
	void *arg;
	int argSize;
} ThreadInfo;

//helper funciton to store thread info and clean up later
ThreadInfo *InitThreadInfoList();
void UnInitThreadInfoList(void *head);
ThreadInfo *InsertThreadInfo(ThreadInfo *head, pthread_t tid, void *arg, int argSize);
int SpawnThread(ThreadInfo *tList, void *(*ThreadFun)(void *), ConRecMsg senderMsg);

#endif
