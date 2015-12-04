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
#include <unistd.h>
#include "P2Helpers.h"
#include "P2ListManagement.h"
#include "P2MessageManagement.h"

//Helper function for spawning children and message drivers
int DispatchMessage(ConRecListNum crlnList, fd_set *rfds, MsgNode *msgRec,
		     int (*MsgHandler)(ConRecListNum, ConRec *, char *));
int SpawnChild(int num, ConRecListNum crlnListNum, int (*fun) (pid_t, int, int),
	       int numFree, ...);
int RunServer(ConRecListNum crlnPub, ConRecListNum crlnSub,
	      int (*MsgHandler)(ConRecListNum, ConRec *, char *));

#endif
