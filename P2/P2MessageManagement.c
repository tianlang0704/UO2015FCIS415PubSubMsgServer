#include "P2MessageManagement.h"

MsgNode *InitMsgList()
{
	MsgNode *temp;
	temp = malloc(sizeof(MsgNode)); //Dealloc in UnInit
	temp->head = temp;
	temp->tail = temp;
	temp->prev = NULL;
	temp->next = NULL;
	temp->msg = NULL;

	return temp;
}

void UnInitMsgList(MsgNode *head)
{
	MsgNode *cur, *next = head;
	while(next != NULL)
	{
		cur = next;
		next = cur->next;

		if(cur->msg != NULL)
			free(cur->msg);
		free(cur);
		cur = NULL;
	}
}

char *InsertMsg(MsgNode *head, char *msg)
{
	MsgNode *temp;
	temp = malloc(sizeof(MsgNode)); // Dealloc in UnInit
	temp->head = head;
	temp->tail = temp;
	temp->prev = head->tail;
	temp->next = NULL;
	head->tail->next = temp;
	head->tail = temp;

	int msgLen = strlen(msg);
	temp->msg = malloc(msgLen + 1); //Dealloc in UnInit
	strcpy(temp->msg, msg);

	return temp->msg;
}

void PrintMsgList(MsgNode *head)
{
	MsgNode *cur = head;
	char buff[MAX_BUFF_LEN];

	perror("Message List:\n");
	while(cur != NULL)
	{
		sprintf(buff, "  %s\n", cur->msg);
		perror(buff);
		cur = cur->next;
	}
}

int ReadMessage(int readFD, char *out, int maxLen)
{
	int res = 0;
	struct pollfd poFD = {readFD, POLLIN, 0};

	poll(&poFD, 1 , -1);
	res = read(readFD, out, maxLen);

	out[res] = 0;
	return res;
}

int SendMessage(int writeFD, const char *msg)
{
	int len = strlen(msg), res = 0;
	struct sigaction OriginalAct;

    //*TODO: check before sending message.
	sigaction(SIGPIPE, NULL, &OriginalAct);
	signal(SIGPIPE, SIG_IGN);
	res = write(writeFD, msg, len);
	sigaction(SIGPIPE, &OriginalAct, NULL);

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

int WaitForMessage(int targetFD)
{
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(targetFD, &rfds);

	return select(targetFD + 1, &rfds, NULL, NULL, NULL);
}

int WaitForMessageLists(fd_set *rfds, fd_set *exc, int num, ...)
{
	FD_ZERO(rfds);
	va_list args;
	va_start(args, num);
	int fdMax = 0;

	int i;
	for(i = 0; i < num; i++)
	{
		ConRecListNum crlnList = va_arg(args, ConRecListNum);
		fdMax = Max(fdMax, AppendctopFDReadToSet(rfds, exc, 1, crlnList));
	}
	va_end(args);

	return select(fdMax + 1, rfds, NULL, NULL, NULL);
}
