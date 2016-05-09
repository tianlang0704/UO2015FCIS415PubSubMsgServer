#include "P2ThreadManagement.h"

ThreadInfo *InitThreadInfoList()
{
	ThreadInfo *temp;
	temp = malloc(sizeof(ThreadInfo)); //Dealloc in UnInit
	temp->head = temp;
	temp->tail = temp;
	temp->prev = NULL;
	temp->next = NULL;
	temp->tid = 0;
	temp->arg = NULL;
	temp->argSize = 0;

	return temp;
}

void UnInitThreadInfoList(void *head)
{
	ThreadInfo *cur, *next = head;
	while(next != NULL)
	{
		cur = next;
		next = cur->next;

		if(cur->tid != 0)
			pthread_join(cur->tid, NULL);

		if(cur->arg != NULL)
			free(cur->arg);
		free(cur);
		cur = NULL;
	}
}

ThreadInfo *InsertThreadInfo(ThreadInfo *head, pthread_t tid, void *arg, int argSize)
{
	ThreadInfo *temp;
	temp = malloc(sizeof(ThreadInfo)); // Dealloc in UnInit
	temp->head = head;
	temp->tail = temp;
	temp->prev = head->tail;
	temp->next = NULL;
	head->tail->next = temp;
	head->tail = temp;

	temp->arg = malloc(argSize);	//Dealloc in UnInit
	memcpy(temp->arg, arg, argSize);
	temp->tid = tid;
	temp->argSize = argSize;

	return temp;
}

int SpawnThread(ThreadInfo *tList, void *(*ThreadFun)(void *), ListConRec listCR)
{
	ThreadInfo *storedInfo = InsertThreadInfo(tList, 0, &listCR, sizeof(listCR));
	return pthread_create(&storedInfo->tid, NULL, ThreadFun, storedInfo->arg);
}
