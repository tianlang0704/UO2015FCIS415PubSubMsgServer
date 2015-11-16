#ifndef CIS415_P2_STRUCTS
#define CIS415_P2_STRUCTS
#define MAX_TOPIC_NUM 20

#include <stdlib.h>

//Struct to record pipe connection info
typedef struct
{
	int ctopFD[2];
	int ptocFD[2];
	pid_t pid;
	int topicNum;
	int topic[MAX_TOPIC_NUM];
} ConRec;

//Strct to track list and its current number in a whole
typedef struct 
{
	int num;
	ConRec *pList;
} ConRecListNum;

#endif
