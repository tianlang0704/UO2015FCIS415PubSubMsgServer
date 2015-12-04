#include "P2TopicQListManagement.h"

TopicQ *InitTopicQList()
{
	return NULL;
}

void UnInitTopicQList(void *qList)
{
	TopicQ *curList, *nextList = qList;
	while(nextList != NULL)
	{
		curList = nextList;
		nextList = curList->next;
		UnInitTopicQ(curList);
	}
}

TopicQ *FindTopicQ(TopicQ *qList, int topicID)
{
	TopicQ *cur = qList;
	while(cur != NULL)
	{
		if(cur->topicID == topicID)
			return cur;
		cur = cur->next;
	}

	return NULL;
}

TopicQ *AppendTopicQList(TopicQ **pQList, int topicID)
{
	TopicQ *tempQ = FindTopicQ((*pQList), topicID);

	if((*pQList) == NULL)
	{
		tempQ = InitTopicQ(topicID);
		tempQ->head = tempQ;
		tempQ->tail = tempQ;
		(*pQList) = tempQ;
	}
	else if(tempQ == NULL)
	{
        tempQ = InitTopicQ(topicID);
		TopicQ *head = (*pQList);
		tempQ->head = head;
		tempQ->tail = tempQ;
        pthread_mutex_lock(&head->accessLock);
		head->tail->next = tempQ;
		head->tail = tempQ;
		pthread_mutex_unlock(&head->accessLock);
	}

	return tempQ;
}

int SubToTopic(TopicQ **pQList, ConRec *sub, int topicID)
{
	TopicQ *tempQ = AppendTopicQList(pQList, topicID);
	SubEntry *tempSubTracker;

	pthread_mutex_lock(&tempQ->accessLock);
	if(tempQ->subTracker == NULL)
	{
		tempSubTracker = malloc(sizeof(SubEntry));
		memset(tempSubTracker, 0, sizeof(SubEntry));
		tempSubTracker->sub = sub;
		tempSubTracker->head = tempSubTracker;
		tempSubTracker->tail = tempSubTracker;
		tempSubTracker->entry = tempQ->entries;
		tempQ->subTracker = tempSubTracker;
	}
	else if(FindSubTracker(tempQ->subTracker, sub) == NULL)
	{
		SubEntry *head = tempQ->subTracker;
		tempSubTracker = malloc(sizeof(SubEntry));
		memset(tempSubTracker, 0, sizeof(SubEntry));
		tempSubTracker->sub = sub;
		tempSubTracker->entry = tempQ->entries;
		tempSubTracker->head = head;
		tempSubTracker->tail = tempSubTracker;
		head->tail->next = tempSubTracker;
		head->tail = tempSubTracker;
	}
	else
	{
        pthread_mutex_unlock(&tempQ->accessLock);
		return -1;
	}
	tempQ->subCount++;
	pthread_mutex_unlock(&tempQ->accessLock);
	return 0;
}

TopicEntry *Enq(TopicQ **pQList, int topicID, int pubID, char data[ENTRYSIZE])
{
    if(pQList == NULL)
        return NULL;
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&lock);

    TopicQ *tempQ = AppendTopicQList(pQList, topicID);
    pthread_mutex_lock(&tempQ->accessLock);
	TopicEntry *res = AppendEntry(tempQ, pubID, data);
	pthread_mutex_unlock(&tempQ->accessLock);

	pthread_mutex_unlock(&lock);
	return res;
}

TopicEntry *Deq(TopicQ **pQList, int topicID, ConRec * sub, int timeOut)
{
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&lock);

	TopicQ *tempQ = FindTopicQ((*pQList), topicID);
	if(tempQ == NULL)
		return NULL;

	SubEntry *curSub = FindSubTracker(tempQ->subTracker, sub);
	if(curSub == NULL)
		return NULL;

    pthread_mutex_lock(&tempQ->accessLock);
    TopicEntry **pCurEntry = &curSub->entry;
	int timeNow = time(NULL);
	//Skip all the old messages
	while((*pCurEntry) != NULL && (*pCurEntry)->timeStamp + timeOut < timeNow)
	{
		(*pCurEntry)->deqCount++;
		if(tempQ->subCount - (*pCurEntry)->deqCount < 1)
		{
			//*TODO: move to buffer
			perror("===================MOVE TO BUFFER!!!");
			tempQ->entryCount--;
		}

		(*pCurEntry) = (*pCurEntry)->next;
	}

	if((*pCurEntry) != NULL)
	{
        TopicEntry *resEntry = (*pCurEntry);
        (*pCurEntry) = (*pCurEntry)->next;
        pthread_mutex_unlock(&tempQ->accessLock);
        pthread_mutex_unlock(&lock);
		return resEntry;
    }
	else
	{
        pthread_mutex_unlock(&tempQ->accessLock);
        pthread_mutex_unlock(&lock);
		return NULL;
    }
}

int WaitForTopics(TopicQ *qList, ConRec *sub, int *out)
{
    do
    {
        TopicQ *curQ = qList;
        while(curQ != NULL)
        {
            SubEntry *curTracker = FindSubTracker(curQ->subTracker, sub);
            if(curTracker != NULL && curTracker->entry != NULL)
            {
                (*out) = curQ->topicID;
                return 0;
            }
            curQ = curQ->next;
        }

    }while(sem_wait(&sub->newEntry) > -1);

    return -1;
}
