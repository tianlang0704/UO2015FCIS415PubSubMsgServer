#ifndef CIS415_P2_STRUCTS
#define CIS415_P2_STRUCTS

#include <stdlib.h>
#include <semaphore.h>

typedef enum {CONNECTED, DISCONNECTED} Status;

//Struct to record pipe connection info
typedef struct
{
	int ctopFD[2];
	int ptocFD[2];
	pid_t pid;
	sem_t newEntry;     //indicates there is new topic
	Status conStatus;
} ConRec;

//Struct to track list and its numbers in a whole
typedef struct
{
	int *pNum;
	int *pMax;
	ConRec **pList;
} ConRecListNum;

//Struct to track connection and the messgae as a whole
typedef struct
{
    ConRecListNum crlnList;
	ConRec *pConRec;
} ListConRec;

//Strct to tract a function pointer and its sole argument
typedef struct
{
	void (*fun)(void *arg);
	void *arg;
} FunArg;

#endif
