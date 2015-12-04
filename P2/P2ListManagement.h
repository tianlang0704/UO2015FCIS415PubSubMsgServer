#ifndef CIS415_P2_ListManagement
#define CIS415_P2_ListManagement

#include <stdarg.h>
#include <string.h>
#include <semaphore.h>
#include "P2Structs.h"
#include "P2Helpers.h"

//helper functions for manipulating connection record list
int InitConRec(ConRec* rec, int *ctopFD, int *ptocFD, pid_t pid, Status conStatus);
void AddConRec(ConRecListNum crlnList, ConRec *new);
void RemoveConRec(ConRecListNum crlnList, ConRec *target);
void EmptyConRec(void *crlnTarget);
int CleanUpList(ConRecListNum crlnList, fd_set *rfds);
//helper function for closing the fd in a list of connection record
void CloseList(ConRec *list, int num);
//helper function for freeing memories and setting their pointers to NULL
//It takes ConRecListNum.
void FreeConRecLists(int num, ...);

#endif

