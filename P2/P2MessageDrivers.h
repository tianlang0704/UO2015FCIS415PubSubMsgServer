//======================P2MessageDrivers.h==================================
//==========================================================================
//==================Helper functions for messaging==========================
//==========================================================================
//=====These are functions for easy sending and receiving messages==========
//==========================================================================
//==========================================================================
#ifndef CIS415_P2_MESSAGEDRIVERS
#define CIS415_P2_MESSAGEDRIVERS
#include <fcntl.h>
#include <sys/poll.h>
#include <signal.h>
#include "P2Helpers.h"

//helper functions for sending and receiving messages
int ReadMessage(int readFD, char *out, int maxLen);
int SendMessage(int writeFD, const char *msg);
int SyncSendMessage(int readFD, int writeFD, const char *msg, char *out, int maxLen);
//Helper function for spawning children
int SpawnChild(int num, ConRec **saveList, int *itemNum, int *itemMax, 
	       int (*fun) (int, int), int numFree, ...);
int RunServer(ConRec **pPubList, ConRec **pSubList, int *pPubNum, int *pSubNum, 
	      int (*MsgHandler)(ConRec *, char *));

#endif
