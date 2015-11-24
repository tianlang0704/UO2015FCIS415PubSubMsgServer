#ifndef CIS415_P2_STRUCTS
#define CIS415_P2_STRUCTS

#include <stdlib.h>

//Struct to record pipe connection info
typedef struct
{
	int ctopFD[2];
	int ptocFD[2];
	pid_t pid;
	int topicNum;
	int topicMax;
	int *topic;
} ConRec;

//Strct to track list and its numbers in a whole
typedef struct 
{
	int *pNum;
	int *pMax;
	ConRec **pList;
} ConRecListNum;

typedef enum {SYNC, ASYNC} SyncMode;

#endif
