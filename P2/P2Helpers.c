//============================P2Helpers.c===================================
//==========================================================================
//==================Helper functions for convenience========================
//==========================================================================
//=====These are functions for easy manipulation of memory and lists========
//==========================================================================
//==========================================================================

#include "P2Helpers.h"

void FreeMems(int num, ...)
{
	va_list args;
	va_start(args, num);
	void *target;

	int i;
	for(i = 0; i < num; i++)
	{
		target = va_arg(args, void *);
		if(target != NULL)
		{
			free(target);
			target = NULL;
		}
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
void AddConRec(ConRec **pList, int *ListNum, int *ListMax, ConRec *new)
{
	if((*pList) == NULL)
	{
		(*pList) = malloc(sizeof(ConRec) * (*ListMax));
	}
	else if((*ListNum) == (*ListMax))
	{
		(*ListMax) *= 2;
		(*pList) = realloc((*pList), sizeof(ConRec) * (*ListMax));
	}
	
	memcpy((*pList) + (*ListNum), new, sizeof(ConRec));
	(*ListNum)++;
}

//helper function for manipulating record list
void RemoveConRec(ConRec *list, int *listNum, ConRec *target)
{
	int i, j;
	for(i = 0; i < (*listNum); i++)
		if(list + i == target)
		{
			char buff[MAX_BUFF_LEN];
			sprintf(buff, "removing %d", target->pid);
			perror(buff);
			CloseFD(target->ctopFD);
			CloseFD(target->ptocFD + 1);
			for(j = i; j < (*listNum) - 1; j++)
				list[j] = list[j + 1];
			(*listNum)--;
			break;
		}
}



