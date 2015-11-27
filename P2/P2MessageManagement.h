#ifndef CIS415_P2_MessageManagement
#define CIS415_P2_MessageManagement
#include <fcntl.h>
#include <sys/poll.h>
#include <signal.h>
#include <string.h>
#include "P2Helpers.h"

typedef struct MsgNodeTag
{
	struct MsgNodeTag *head; //only recorded in the head
	struct MsgNodeTag *tail; //only recorded in the head
	struct MsgNodeTag *prev;
	struct MsgNodeTag *next;
	char *msg;
} MsgNode;

//helper funtions for sotring messages
MsgNode *InitMsgList();
void UnInitMsgList(MsgNode *head);
char *InsertMsg(MsgNode *head, char *msg);
void PrintMsgList(MsgNode *head);
//helper functions for sending and receiving messages
int ReadMessage(int readFD, char *out, int maxLen);
int SendMessage(int writeFD, const char *msg);
int SyncSendMessage(int readFD, int writeFD, const char *msg, char *out, int maxLen);
int WaitForMessage(int targetFD);
int WaitForMessageLists(fd_set *rfds, fd_set *exc, int num, ...);

#endif
