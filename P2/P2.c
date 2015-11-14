#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/poll.h>

#define PUB_SEM "CIS415_P2_Pub_SEM"

#define INIT_LIST_MAX 20
#define MAX_TOPIC 20
#define MAX_BUFF_LEN 255

typedef struct
{
	int ctopFd[2];
	int ptocFd[2];
	int pid;
	int topic[MAX_TOPIC];
} ConRec;

ConRec *m_PubList;
int m_PubMax = INIT_LIST_MAX;
int m_PubNum = 0;
ConRec *m_SubList;
int m_SubMax = INIT_LIST_MAX;
int M_SubNum = 0;


int ReadMessage(int readFd, char *out, int maxLen)
{
	int index = 0, temp = 0;
	char buff[255];
	struct pollfd poFd = {readFd, POLLIN, 0};

	sprintf(buff, "%d: in read message", getpid());
	perror(buff);

	poll(&poFd, 1 , -1);
	index = read(readFd, out, maxLen);

	sprintf(buff, "%d: after read message", getpid());
	perror(buff);
	out[index] = 0;
	return index;
}

int SendMessage(int writeFd, const char *msg)
{
	int len = strlen(msg), res = 0;
	char buff[255];
	
	sprintf(buff, "%d: in send mesesage", getpid());
	perror(buff);
	
	res = write(writeFd, msg, len);
	
	sprintf(buff, "%d: after send message", getpid());
	perror(buff);
	return res;
}

int SyncSendMessage(int readFd, 
		    int writeFd, 
		    const char *msg, 
		    char *out, 
		    int maxLen)
{
	SendMessage(writeFd, msg);
	return ReadMessage(readFd, out, maxLen);
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

int RunPub(int ctopFd[2], int ptocFd[2])
{
	char buff[MAX_BUFF_LEN];
	int writeFd = ctopFd[1], readFd = ptocFd[0];
	//child close child-to-parent read, it only writes to this
	//and close parent-to-child write, it only reads from this
	close(ctopFd[0]);
	close(ptocFd[1]);
	sprintf(buff, "pub %d connect", getpid());
	SyncSendMessage(readFd, writeFd, buff, buff, MAX_BUFF_LEN);
	Assert(buff, "accept");
	sprintf(buff, "pub %d topic 1", getpid());
	SyncSendMessage(readFd, writeFd, buff, buff, MAX_BUFF_LEN);
	Assert(buff, "accept");
	SendMessage(ctopFd[1], "end");
	return 0;
}

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

int SpawnChild(int num,
	       ConRec **saveList,
	       int *itemNum, 
	       int *itemMax, 
	       int (*fun) (int*, int*))
{
	int ctopFd[2], ptocFd[2];
	char buff[250];
	ConRec recBuff;

	int i;
	for(i = 0; i < num; i++)
	{
		if(pipe(ctopFd) < 0 || pipe(ptocFd) < 0)
		{
			perror("Error piping");
			exit(EXIT_FAILURE);
		}
	
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
			sleep(1);
			fun(ctopFd, ptocFd);
			exit(0);
		}
		
		close(ptocFd[0]);
		close(ctopFd[1]);
		memcpy(recBuff.ctopFd, ctopFd, 2 * sizeof(int));
		memcpy(recBuff.ptocFd, ptocFd, 2 * sizeof(int));
		AddConRec(saveList, itemNum, itemMax, &recBuff);
	}
}

int main()
{
	char buff[MAX_BUFF_LEN] = {0};

	sprintf(buff, "parent pid: %d", getpid());
	perror(buff);

	SpawnChild(3, &m_PubList, &m_PubNum, &m_PubMax, RunPub);
	
	while(ReadMessage(m_PubList[0].ctopFd[0], buff, MAX_BUFF_LEN))
	{
		char buff2[250];
		sprintf(buff2, "Message received: %s", buff);
		perror(buff2);
		SendMessage(m_PubList[0].ptocFd[1], "accept");
	}

	//wait(-1, NULL, 0);
	free(m_PubList);
	free(m_SubList);
	return 0;
}
