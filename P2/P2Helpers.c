//============================P2Helpers.c===================================
//==========================================================================
//==================Helper functions for convenience========================
//==========================================================================
//=====These are functions for easy manipulation of memory and lists========
//==========================================================================
//==========================================================================

#include "P2Helpers.h"

#define INIT_TOPIC_NUM 10

void FreeConRecLists(int num, ...)
{
	va_list args;
	va_start(args, num);
	ConRecListNum crlnTarget;

	int i;
	for(i = 0; i < num; i++)
	{
		crlnTarget = va_arg(args, ConRecListNum);
		EmptyConRec(crlnTarget);
	}
	va_end(args);
}

int Max(int a, int b)
{
	return a > b ? a : b;
}

int AssertStr(const char* str1, const char *str2)
{
	if(strcmp(str1, str2) != 0)
	{
		char buff[MAX_BUFF_LEN];
		sprintf(buff, "%d: failed assertion: %s, %s", 
				getpid(), str1, str2);
		perror(buff);
		exit(EXIT_FAILURE);
	}
	return 0;
}

int CloseFD(int *pTargetFD)
{
	int res = -1;
	if((*pTargetFD) != -1)
	{
		res = close((*pTargetFD));
		(*pTargetFD) = -1;
	}
	return res;
}

int CloseList(ConRec *list, int num)
{
	int i, res = -1;
	for(i = 0; i < num; i++)
	{
		CloseFD(list[i].ctopFD);
		CloseFD(list[i].ctopFD + 1);
		CloseFD(list[i].ptocFD);
		CloseFD(list[i].ptocFD + 1);
	}
}

//helper function for manipulating record list
void AddConRec(ConRecListNum crlnList, ConRec *new)		
{
	ConRec **pList = crlnList.pList;
	int *pListNum = crlnList.pNum;
	int *pListMax = crlnList.pMax;

	if((*pListMax) == 0)
			(*pListMax) = 10;

	if((*pList) == NULL)
	{
		(*pList) = malloc(sizeof(ConRec) * (*pListMax));
	}
	else if((*pListNum) == (*pListMax))
	{
		(*pListMax) *= 2;
		(*pList) = realloc((*pList), sizeof(ConRec) * (*pListMax));
	}
	
	memcpy((*pList) + (*pListNum), new, sizeof(ConRec));
	(*pListNum)++;
}

//helper function for manipulating record list
void RemoveConRec(ConRecListNum crlnList, ConRec *target)
{
	ConRec *list = (*crlnList.pList);
	int *listNum = crlnList.pNum;

	int i, j;
	for(i = 0; i < (*listNum); i++)
		if(list + i == target)
		{
			char buff[MAX_BUFF_LEN];
			sprintf(buff, "removing %d", target->pid);
			perror(buff);
			CloseFD(target->ctopFD);
			CloseFD(target->ptocFD + 1);
			EmptyTopics(target);
			for(j = i; j < (*listNum) - 1; j++)
				list[j] = list[j + 1];
			(*listNum)--;
			break;
		}
}

void EmptyConRec(ConRecListNum crlnTarget)
{
	int i;
	if((*crlnTarget.pList) != NULL)
	{
		//Clean up topics
		for(i = 0; i < (*crlnTarget.pNum); i++)
			EmptyTopics((*crlnTarget.pList) + i);
		//Clean up list
		free(*crlnTarget.pList);
		(*crlnTarget.pList) = NULL;
		(*crlnTarget.pNum) = 0;
		(*crlnTarget.pMax) = 0;
	}
}

void AddTopic(ConRec *target, int topic)
{
	if(target->topicMax == 0)
	{
		target->topic = malloc(INIT_TOPIC_NUM * sizeof(int));
		target->topicMax = INIT_TOPIC_NUM;
	}
	else if(target->topicMax == target->topicNum)
	{
		target->topicMax *= 2;
		target->topic = realloc(target->topic, target->topicMax);
	}
	
	target->topic[target->topicNum] = topic;
	target->topicNum++;
}

void EmptyTopics(ConRec *target)
{
	if(target->topic != NULL)
	{
		free(target->topic);
		target->topic = NULL;
		target->topicNum = 0;
		target->topicMax = 0;
	}

}

int WaitForChildren()
{
	int stat;
	while(wait(&stat) != -1 )
		if(WIFEXITED(stat) == 0)
			return -1;

	return 0;
}
