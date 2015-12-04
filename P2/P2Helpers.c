//============================P2Helpers.c===================================
//==========================================================================
//==================Helper functions for convenience========================
//==========================================================================
//=====These are functions for easy manipulation of memory and lists========
//==========================================================================
//==========================================================================

#include "P2Helpers.h"

void print(char *str)
{
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&lock);
    fprintf(stdout, "%s\n", str);
    pthread_mutex_unlock(&lock);
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

int CountCon(ConRecListNum crlnList)
{
	ConRec *list = (*crlnList.pList);
	int num = (*crlnList.pNum), counter = 0;

	int i;
	for(i = 0; i < num; i++)
		if(list[i].conStatus == CONNECTED)
			counter++;

	return counter;
}

int SyncDisconnect(ConRecListNum crlnList, ConRec *pConRec)
{
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    int count;
    pthread_mutex_lock(&lock);
    pConRec->conStatus = DISCONNECTED;
    count = CountCon(crlnList);
    pthread_mutex_unlock(&lock);
    return count;
}

//Helper function for populating file description set for select function
//to use to detect ready pipe
//it takes ConRecListNum to add the whole list of ctopFD[0] to set
//returns the largest fd number added.
int AppendctopFDReadToSet(fd_set *set, fd_set *exc, int num, ...)
{
	va_list args;
	va_start(args, num);
	int fdMax = 0;

	int i;
	for(i = 0; i < num; i++)
	{
		ConRecListNum crlnBuff = va_arg(args, ConRecListNum);

		int j;
		for(j = 0; j < (*crlnBuff.pNum); j++)
		{
			int target = (*crlnBuff.pList)[j].ctopFD[0];
			if(!FD_ISSET(target, exc))
			{
				FD_SET(target, set);
				fdMax = Max(fdMax, target);
			}
		}
	}

	va_end(args);
	return fdMax;
}

int AppendConnectedctopFDReadToSet(ConRecListNum crlnList, fd_set *set)
{
	int num = (*crlnList.pNum);
	ConRec *list = *crlnList.pList;
	int i;
	for(i = 0; i < num; i++)
		if(list[i].conStatus == CONNECTED)
			FD_SET(list[i].ctopFD[0], set);

	return 0;
}
