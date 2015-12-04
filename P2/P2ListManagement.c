#include "P2ListManagement.h"

int InitConRec(ConRec* rec, int *ctopFD, int *ptocFD, pid_t pid, Status conStatus)
{
	memcpy(rec->ctopFD, ctopFD, 2 * sizeof(int));
	memcpy(rec->ptocFD, ptocFD, 2 * sizeof(int));
	rec->pid = pid;
	rec->conStatus = conStatus;
	sem_init(&rec->newEntry, 0, 0);
	return 0;
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
			sem_destroy(&target->newEntry);
			for(j = i; j < (*listNum) - 1; j++)
				list[j] = list[j + 1];
			(*listNum)--;
			break;
		}
}

void EmptyConRec(void *pcrlnTarget)
{
	ConRecListNum crlnTarget = (*(ConRecListNum *)pcrlnTarget);

	if((*crlnTarget.pList) != NULL)
	{
		//Clean up new topic indicator
		int i;
		for(i = 0; i < (*crlnTarget.pNum); i++)
		{
			ConRec *tempCR = (*crlnTarget.pList) + i;
			sem_destroy(&tempCR->newEntry);
        }
		//Clean up list
		free(*crlnTarget.pList);
		(*crlnTarget.pList) = NULL;
		(*crlnTarget.pNum) = 0;
		(*crlnTarget.pMax) = 0;
	}
}

void FreeConRecLists(int num, ...)
{
	va_list args;
	va_start(args, num);
	ConRecListNum crlnTarget;

	int i;
	for(i = 0; i < num; i++)
	{
		crlnTarget = va_arg(args, ConRecListNum);
		EmptyConRec(&crlnTarget);
	}
	va_end(args);
}

int CleanUpList(ConRecListNum crlnList, fd_set *rfds)
{
	ConRec *list = (*crlnList.pList);
	int num = (*crlnList.pNum);

	int i;
	for(i = 0; i < num; i++)
		if(FD_ISSET(list[i].ctopFD[0], rfds) ||
		   FD_ISSET(list[i].ctopFD[1], rfds) ||
		   FD_ISSET(list[i].ptocFD[0], rfds) ||
		   FD_ISSET(list[i].ptocFD[1], rfds))
			RemoveConRec(crlnList, list + i);
	return 0;
}

void CloseList(ConRec *list, int num)
{
	int i;
	for(i = 0; i < num; i++)
	{
		CloseFD(list[i].ctopFD);
		CloseFD(list[i].ctopFD + 1);
		CloseFD(list[i].ptocFD);
		CloseFD(list[i].ptocFD + 1);
	}
}
