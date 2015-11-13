#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define PUB_SEM "CIS415_P2_Pub_SEM"

#define MAX_PUB 255
#define MAX_SUB 255
#define MAX_BUFF_LEN 255

int ReadMessage(int readFd, char *out, int maxLen)
{
	perror("123123");
	int index = 0;
	while((index < maxLen - 1) && (read(readFd, out + index, 1) > 0))
	{
		perror("123123");
		index++;
	}
	perror("321321");
	//*TODO add error handling;
	out[index] = 0;
	return index;
}

int SendMessage(int writeFd, const char *msg)
{
	int len = strlen(msg), res = 0;
	res = write(writeFd, msg, len);
	//close(dupFd);
	close(writeFd);
	return res;
}

int SyncSendMessage(int pipefd[2], const char *msg, char *out, int maxLen)
{
	SendMessage(pipefd[1], msg);
	return ReadMessage(pipefd[0], out, maxLen);
}

int Assert(const char* str1, const char *str2)
{
	if(str1 != str2)
	{
		char buff[MAX_BUFF_LEN];
		sprintf(buff, "%d: failed assertion: %s, %s", 
				getpid(), str1, str2);
		perror(buff);
		exit(EXIT_FAILURE);
	}
	return 0;
}

int Pub(int pipefd[2])
{
	char buff[MAX_BUFF_LEN];

	sprintf(buff, "pub %d connect", getpid());
	SyncSendMessage(pipefd, buff, buff, MAX_BUFF_LEN);
	Assert(buff, "accept");
	sprintf(buff, "pub %d topic 1", getpid());
	SyncSendMessage(pipefd, buff, buff, MAX_BUFF_LEN);
	Assert(buff, "accept");
	SendMessage(pipefd[1], "end");
}

int main()
{
	int pipefd[2], oldFd;
	char buff[MAX_BUFF_LEN] = {0};

	if(pipe(pipefd) < 0)
	{
		perror("Error piping");
		exit(EXIT_FAILURE);
	}

	pid_t test = fork();
	if(test < 0)
	{
		perror("Error forking");
		exit(EXIT_FAILURE);
	}
	else if(test == 0)
	{
		sleep(3);
		write(pipefd[1], "hello", 5);
		close(pipefd[1]);
		//close(pipefd[0]);
		//SendMessage(pipefd[1], "hello");
		//Pub(pipefd);
		exit(0);
	}
	oldFd = pipefd[1];
	pipefd[1] = fcntl(oldFd, F_DUPFD, 0);
	close(oldFd);
	while(ReadMessage(pipefd[0], buff, MAX_BUFF_LEN))
	{
		perror(buff);
		printf("Message received: %s", buff);
		//SendMessage(pipefd[1], "accept");
	}

	//wait(-1, NULL, 0);
	return 0;
}
