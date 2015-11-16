//============================P2Helpers.h===================================
//==========================================================================
//==================Helper functions for convenience========================
//==========================================================================
//=====These are functions for easy manipulation of memory and lists========
//==========================================================================
//==========================================================================
#ifndef CIS415_P2_HELPERS
#define CIS415_P2_HELPERS

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "P2Structs.h"

#define MAX_BUFF_LEN 255

//helper function for freeing memories and setting their pointers to NULL
void FreeMems(int num, ...);
//helper function for returning the max of the two
int Max(int a, int b);
//helper function for asserting two strings are equal
int AssertStr(const char* str1, const char *str2);
//helper function for closing the fd and settting it to -1
int CloseFD(int *pTargetFD);
//helper function for closing the fd in a list of connection record
int CloseList(ConRec *list, int num);
//helper function for manipulating connection record list
void AddConRec(ConRec **pList, int *ListNum, int *ListMax, ConRec *new);
//helper function for manipulating connection record list
void RemoveConRec(ConRec *list, int *listNum, ConRec *target);

#endif
