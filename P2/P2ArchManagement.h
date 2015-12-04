#ifndef CIS415_P2_ArchManagement
#define CIS415_P2_ArchManagement

#include <pthread.h>
#include <semaphore.h>
#include "P2Helpers.h"
#include "P2TopicQManagement.h"

typedef struct
{
    TopicEntry *buff1;
    TopicEntry *buff2;
    TopicEntry **buffCur;
    pthread_mutex_t switchLock;

} TopicDoubleBuffer;

typedef struct
{
    int buffSize;
    int itemNum;
    int terminateIndicator;
    pthread_t tid;
    TopicDoubleBuffer buff;
    sem_t waitLock;
} Arch;

void InitArch(Arch *arch, int buffSize);
void UnInitArch(Arch *arch);

TopicEntry *AppendToArch(Arch *arch, TopicEntry *entry);
void WriteToFile(TopicEntry *buff);

void *ArchFun(void *arg);


#endif
