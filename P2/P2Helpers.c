//============================P2Helpers.c===================================
//==========================================================================
//==================Helper functions for convenience========================
//==========================================================================
//=====These are functions for easy manipulation of memory and lists========
//==========================================================================
//==========================================================================

#include "P2Helpers.h"

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

int WaitForChildren()
{
	int stat;
	while(wait(&stat) != -1 )
		if(WIFEXITED(stat) == 0)
			return -1;

	return 0;
}

int CountDiscon(ConRecListNum crlnList)
{
	ConRec *list = (*crlnList.pList);
	int num = (*crlnList.pNum), counter = 0;

	int i;
	for(i = 0; i < num; i++)
		if(list[i].conStatus == DISCONNECTED)
			counter++;
	
	return counter;
}
