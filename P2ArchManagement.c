#include "P2ArchManagement.h"

void InitArch(Arch *arch, int buffSize)
{
    memset(arch, 0, sizeof(Arch));
    pthread_mutex_init(&arch->buff.switchLock, NULL);
    sem_init(&arch->waitLock, 0, 0);
    arch->buffSize = buffSize;
    arch->buff.buffCur = &arch->buff.buff1;
    pthread_create(&arch->tid, NULL, ArchFun, arch);
}

void UnInitArch(Arch *arch)
{
    pthread_mutex_lock(&arch->buff.switchLock);
    if((*arch->buff.buffCur) != NULL)
    	FreeEntries((*arch->buff.buffCur));
    pthread_mutex_unlock(&arch->buff.switchLock);
    pthread_mutex_destroy(&arch->buff.switchLock);
    arch->terminateIndicator = -1;
    sem_post(&arch->waitLock);
    if(arch->tid != 0)
        pthread_join(arch->tid, NULL);
    sem_destroy(&arch->waitLock);
}

TopicEntry *AppendToArch(Arch *arch, TopicEntry *entry)
{
    if(arch->itemNum >= arch->buffSize)
    {
        pthread_mutex_lock(&arch->buff.switchLock);
        arch->buff.buffCur = (*arch->buff.buffCur) == arch->buff.buff1 ? &arch->buff.buff2 : &arch->buff.buff1;
        arch->itemNum = 0;
        pthread_mutex_unlock(&arch->buff.switchLock);
#ifdef VERBOSE
                print("=============buffer switching===========");
#endif
        sem_post(&arch->waitLock);
    }

    if((*arch->buff.buffCur) == NULL)
    {
        entry->head = entry;
        entry->tail = entry;
        entry->prev = NULL;
        entry->next = NULL;
        (*arch->buff.buffCur) = entry;
    }
    else
    {
        TopicEntry *head = (*arch->buff.buffCur);
        entry->head = head;
        entry->tail = entry;
        entry->prev = head->tail;
        entry->next = NULL;
        head->tail->next = entry;
        head->tail = entry;
    }

    arch->itemNum++;
    return entry;
}

void WriteToFile(TopicEntry *entry)
{
    TopicEntry *cur = entry;
    while(cur != NULL)
    {
        if(cur->data != NULL)
        {
                char fileName[FILENAME_MAX];
                sprintf(fileName, "topic%d", cur->topicID);
                FILE *fileW = fopen(fileName, "a");
                fprintf(fileW, "%s\n", cur->data);
                fclose(fileW);
#ifdef VERBOSE
                char strBuff[MAX_BUFF_LEN];
                sprintf(strBuff, "writting entry to filename \"%s\": %s", fileName, cur->data);
                print(strBuff);
#endif
                cur = cur->next;
        }
    }
}

void *ArchFun(void *arg)
{
    Arch *arch = arg;
    while(sem_wait(&arch->waitLock) > -1 && arch->terminateIndicator == 0)
    {
        pthread_mutex_lock(&arch->buff.switchLock);
        TopicEntry **targetBuff = (*arch->buff.buffCur) == arch->buff.buff1 ? &arch->buff.buff2 : &arch->buff.buff1;
        WriteToFile((*targetBuff));
        FreeEntries((*targetBuff));
        (*targetBuff) = NULL;
        pthread_mutex_unlock(&arch->buff.switchLock);
    }

//    if((*arch->buff.buffCur) != NULL)
//        WriteToFile((*arch->buff.buffCur));
    return NULL;
}
