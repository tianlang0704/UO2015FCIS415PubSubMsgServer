#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <signal.h>

#define PUB_SEM "CIS415_P2_Pub_SEM"

#define INIT_LIST_MAX 20
#define MAX_TOPIC 20
#define MAX_BUFF_LEN 255

typedef struct
{
	int ctopFD[2];
	int ptocFD[2];
	pid_t pid;
	int topicNum;
	int topic[MAX_TOPIC];
} ConRec;

ConRec *m_PubList = NULL;
int m_PubMax = INIT_LIST_MAX;
int m_PubNum = 0;
ConRec *m_SubList = NULL;
int m_SubMax = INIT_LIST_MAX;
int m_SubNum = 0;

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
			free(target);
	}
	va_end(args);
}

int ReadMessage(int readFD, char *out, int maxLen)
{
	int index = 0, temp = 0;
	char buff[255];
	struct pollfd poFD = {readFD, POLLIN, 0};

	poll(&poFD, 1 , -1);
	index = read(readFD, out, maxLen);

	sprintf(buff, "%d: after read message", getpid());
	perror(buff);
	out[index] = 0;
	return index;
}

int SendMessage(int writeFD, const char *msg)
{
	int len = strlen(msg), res = 0;
	struct sigaction OriginalAct;

	sigaction(SIGPIPE, NULL, &OriginalAct);
	signal(SIGPIPE, SIG_IGN);
	res = write(writeFD, msg, len);
	sigaction(SIGPIPE, &OriginalAct, NULL);
	
	char buff[255];
	sprintf(buff, "%d: after send message", getpid());
	perror(buff);
	return res;
}

int SyncSendMessage(int readFD, 
		    int writeFD, 
		    const char *msg, 
		    char *out, 
		    int maxLen)
{
	SendMessage(writeFD, msg);
	return ReadMessage(readFD, out, maxLen);
}

int Assert(const char* str1, const char *str2)
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

int RunPub(int ctopFD[2], int ptocFD[2])
{
	char msg[MAX_BUFF_LEN];
	int writeFD = ctopFD[1], readFD = ptocFD[0];
	//child close child-to-parent read, it only writes to this
	//and close parent-to-child write, it only reads from this
	close(ctopFD[0]);
	close(ptocFD[1]);
	sprintf(msg, "pub %d connect", getpid());
	SyncSendMessage(readFD, writeFD, msg, msg, MAX_BUFF_LEN);
	Assert(msg, "accept");
	sprintf(msg, "pub %d topic 1", getpid());
	SyncSendMessage(readFD, writeFD, msg, msg, MAX_BUFF_LEN);
	Assert(msg, "accept");
	SyncSendMessage(readFD, writeFD, "end", msg, MAX_BUFF_LEN);
	Assert(msg, "accept");
	close(ctopFD[1]);
	close(ptocFD[0]);
	FreeMems(2, m_PubList, m_SubList);
	return 0;
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
			close(target->ctopFD[0]);
			close(target->ptocFD[1]);
			for(j = i; j < (*listNum) - 1; j++)
				list[j] = list[j + 1];
			(*listNum)--;
			break;
		}
}

int Max(int a, int b)
{
	return a > b ? a : b;
}

//Helper function for spawning children
//return: the largest FD number
int SpawnChild(int num,
	       ConRec **saveList,
	       int *itemNum, 
	       int *itemMax, 
	       int (*fun) (int*, int*))
{
	int ctopFD[2], ptocFD[2], maxFD = 0;
	char buff[250];
	ConRec recBuff;

	int i;
	for(i = 0; i < num; i++)
	{
		if(pipe2(ctopFD, O_NONBLOCK) < 0 || pipe2(ptocFD, O_NONBLOCK) < 0)
		{
			perror("Error piping");
			exit(EXIT_FAILURE);
		}
		maxFD = Max(maxFD,
			    Max(Max(ctopFD[0], ctopFD[1]), 
				Max(ptocFD[0], ptocFD[1])));

		pid_t child = fork();
		if(child < 0)
		{
			perror("Error forking");
			exit(EXIT_FAILURE);
		}
		else if(child == 0)
		{
			sprintf(buff, "chlid %d pid: %d", i, getpid());
			perror(buff);
			fun(ctopFD, ptocFD);
			exit(0);
		}
		
		close(ptocFD[0]);
		close(ctopFD[1]);
		memcpy(recBuff.ctopFD, ctopFD, 2 * sizeof(int));
		memcpy(recBuff.ptocFD, ptocFD, 2 * sizeof(int));
		recBuff.pid = child;
		AddConRec(saveList, itemNum, itemMax, &recBuff);
	}
	return maxFD;
}

//Message handler
int MessageHandler(ConRec *target, char *msg)
{
	char buff[250];
	sprintf(buff, "Message received: %s", msg);
	perror(buff);
	return SendMessage(target->ptocFD[1], "accept");
}

typedef struct 
{
	int num;
	ConRec *pList;
} ConRecListNum;

//Helper function for populating file description set for select function
//to use to detect ready pipe
int PopulateFDSet(fd_set *set, int num, ...)
{
	va_list args;
	va_start (args, num);
	int count = 0;
	
	int i;
	for(i = 0; i < num; i++)
	{
		ConRecListNum crlnBuff = va_arg(args, ConRecListNum);
		
		int j;
		for(j = 0; j < crlnBuff.num; j++)
		{
			FD_SET(crlnBuff.pList[j].ctopFD[0], set);
			count++;
		}
	}

	va_end(args);
	return count;
}

int main()
{
	char buff[MAX_BUFF_LEN] = {0};
	fd_set rfds;
	int i, readFD = 0, res = 0, maxFD = 0;

	sprintf(buff, "parent pid: %d", getpid());
	perror(buff);

	maxFD = SpawnChild(3, &m_PubList, &m_PubNum, &m_PubMax, RunPub);
	while(m_PubNum > 0 || m_SubNum > 0)
	{
		ConRecListNum crlnPub = {m_PubNum, m_PubList};
		ConRecListNum crlnSub = {m_SubNum, m_SubList};
		FD_ZERO(&rfds);
		PopulateFDSet(&rfds, 2, crlnPub, crlnSub);
		res = select(maxFD + 1, &rfds, NULL, NULL, NULL);
		sprintf(buff, "::::::::::::::res = %d", res);
		perror(buff);
		if(res > 0)
		{
			for(i = 0; i < m_PubNum; i++)
			{
				readFD = m_PubList[i].ctopFD[0];
				sprintf(buff, "readFD = %d", readFD);
				perror(buff);
				if(FD_ISSET(readFD, &rfds))
				{
					if(ReadMessage(readFD, buff, MAX_BUFF_LEN) > 0)
						MessageHandler(m_PubList + i, buff);
					else
						RemoveConRec(m_PubList, 
							     &m_PubNum,
							     m_PubList + i);
				}
			}

			for(i = 0; i < m_SubNum; i++)
			{
				readFD = m_SubList[i].ctopFD[0];
				if(FD_ISSET(readFD, &rfds))
				{
					if(ReadMessage(readFD, buff, MAX_BUFF_LEN) > 0)
						MessageHandler(m_SubList + i, buff);
					else
						RemoveConRec(m_SubList,
							     &m_SubNum,
							     m_PubList + i);
				}
			}
		}
		
	}

	//wait(-1, NULL, 0);
	for(i = 0; i < m_PubNum; i++)
	{
		close(m_PubList[i].ptocFD[1]);
		close(m_PubList[i].ctopFD[0]);
	}
	for(i = 0; i < m_SubNum; i++)
	{
		close(m_SubList[i].ptocFD[1]);
		close(m_SubList[i].ctopFD[0]);
	}

	FreeMems(2, m_PubList, m_SubList);
	return 0;
}
