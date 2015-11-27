//============================P2Helpers.h===================================
//==========================================================================
//==================Helper functions for convenience========================
//==========================================================================
#ifndef CIS415_P2_HELPERS
#define CIS415_P2_HELPERS

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "P2Structs.h"

#define MAX_BUFF_LEN 255

//helper function for returning the max of the two
int Max(int a, int b);
//helper function for asserting two strings are equal
int AssertStr(const char* str1, const char *str2);
//helper function for closing the fd and settting it to -1
int CloseFD(int *pTargetFD);
//helper fundtion for waiting for children to finish
int WaitForChildren();
//helper function for counting disconnected item in the ConRec list
int CountDiscon(ConRecListNum crlnList);

#endif
