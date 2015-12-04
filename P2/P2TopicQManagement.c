//=========================P2TopicQManagement.c==============================
//==========================================================================
//==============Functions for managing and storing topics===================
//==========================================================================
#include "P2TopicQManagement.h"

#define INIT_TOPIC_NUM 10

TopicQ *InitTopicQ(int topicID)
{
	TopicQ *tempQ = malloc(sizeof(TopicQ)); // Dealloc in UnInit
	memset(tempQ, 0, sizeof(TopicQ));
	tempQ->topicID = topicID;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&tempQ->accessLock, &attr); //*TODO: need error check
	pthread_mutexattr_destroy(&attr);
	return tempQ;
}

void UnInitTopicQ(TopicQ *q)
{
	if(q != NULL)
	{
		TopicEntry *curEntry, *nextEntry = q->entries;
		while(nextEntry != NULL)
		{
			curEntry = nextEntry;
			nextEntry = curEntry->next;
			free(curEntry);
			curEntry = NULL;
		}
		q->entries = NULL;

		SubEntry *curTracker, *nextTracker = q->subTracker;
		while(nextTracker != NULL)
		{
			curTracker = nextTracker;
			nextTracker = curTracker->next;
			free(curTracker);
		}
		q->subTracker = NULL;

		pthread_mutex_destroy(&q->accessLock);
		free(q);
		q = NULL;
	}
}

void UpdateSubTracker(TopicQ *q, TopicEntry *entry)
{
	SubEntry *cur = q->subTracker;
	while(cur != NULL)
	{
		if(cur->entry == NULL)
		{
			cur->entry = entry;
			sem_post(&cur->sub->newEntry);
        }
		cur = cur->next;
	}
}

TopicEntry *AppendEntry(TopicQ *q, int pubID, char data[ENTRYSIZE])
{
	if(q == NULL || q->entryCount >= MAXENTRIES)
		return NULL;

	TopicEntry *temp = malloc(sizeof(TopicEntry));
	memset(temp, 0, sizeof(TopicEntry));

	temp->tail = temp;
	temp->timeStamp = time(NULL);
	temp->pubID = pubID;
	memcpy(temp->data, data, ENTRYSIZE * sizeof(char));
	if(q->entries == NULL)
	{
		temp->head = temp;
		q->entries = temp;
	}
	else
	{
		TopicEntry *head = q->entries;
		temp->head = head;
		temp->prev = head->tail;
		head->tail->next = temp;
		head->tail = temp;
	}

	UpdateSubTracker(q, temp);
	q->entryCount++;
	return temp;
}

TopicEntry *FindEntry(TopicQ *q, TopicEntry *entry)
{
    if(q == NULL || q->entries == NULL)
         return NULL;

    TopicEntry *cur = q->entries;
    while(cur != NULL)
    {
        if(cur == entry)
            return entry;

        cur = cur->next;
    }

    return NULL;
}

TopicEntry *RemoveEntry(TopicQ *q, TopicEntry *entry)
{
    if(q == NULL || q->entries == NULL)
        return NULL;

    if(FindEntry(q, entry) == NULL)
        return NULL;

    if(q->entries->head == entry)
    {
        TopicEntry *oldHead = q->entries;
        TopicEntry *newHead = oldHead->next;
        newHead->head = newHead;
        newHead->tail = oldHead->tail;
        newHead->prev = NULL;
        q->entries = newHead;
        return oldHead;
    }
    else
    {
        TopicEntry *prev = entry->prev;
        TopicEntry *next = entry->next;
        prev->next = entry->next;
        next->prev = entry->prev;
        return entry;
    }
}


SubEntry *FindSubTracker(SubEntry *subTracker, ConRec *sub)
{
	SubEntry *cur = subTracker;
	while(cur != NULL)
	{
		if(cur->sub == sub)
			return cur;
        cur = cur->next;
    }
	return NULL;
}

