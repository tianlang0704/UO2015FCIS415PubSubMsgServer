#ifndef CIS415_P2_TopicQManagement
#define CIS415_P2_TopicQManagement

#include <pthread.h>
#include <time.h>
#include <string.h>
#include "P2Structs.h"
#include "P2Settings.h"

typedef struct TopicEntryTag
{
	struct TopicEntryTag *head;
	struct TopicEntryTag *tail;
	struct TopicEntryTag *prev;
	struct TopicEntryTag *next;
	int timeStamp;
	int pubID;
	int topicID;
	int deqCount;
	char data[ENTRYSIZE];
} TopicEntry;

//Struct to tract each subscriber's position in a topic queue
typedef struct SubEntryTag
{
	struct SubEntryTag *head;
	struct SubEntryTag *tail;
	struct SubEntryTag *next;
	ConRec *sub;
	TopicEntry *entry;
} SubEntry;

typedef struct TopicQTag
{
	struct TopicQTag *head;
	struct TopicQTag *tail;
	struct TopicQTag *next;
	int topicID;
	int entryCount;
	int subCount;
	TopicEntry *entries;
	SubEntry *subTracker;
	pthread_mutex_t accessLock;     //for accessing to the queue
} TopicQ;

//helper functions for caching the topics
TopicQ 	*InitTopicQ(int topicID);
void 	UnInitTopicQ(TopicQ *q);
void    FreeEntries(TopicEntry *entryList);
void 	UpdateSubTracker(TopicQ *q, TopicEntry *entry);
void    ShiftSubTracker(TopicQ *q, TopicEntry *entry);
TopicEntry *AppendEntry(TopicQ *q, int pubID, char data[ENTRYSIZE]);
TopicEntry *FindEntry(TopicQ *q, TopicEntry *entry);
TopicEntry *RemoveEntry(TopicQ *q, TopicEntry *entry);
SubEntry *FindSubTracker(SubEntry *subTracker, ConRec *sub);

#endif
