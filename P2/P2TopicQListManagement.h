#ifndef CIS415_P2_TopicQListManagement
#define CIS415_P2_TopicQListManagement

#include "P2Settings.h"
#include "P2TopicQManagement.h"
#include "P2ArchManagement.h"

TopicQ 	*InitTopicQList();
void 	UnInitTopicQList(void *qList);
TopicQ 	*AppendTopicQList(TopicQ **pQList, int topicID);
TopicQ 	*FindTopicQ(TopicQ *qList, int topicID);
TopicEntry *Enq(TopicQ **pQList, int topicID, int pubID, char data[ENTRYSIZE]);
TopicEntry *Deq(TopicQ **pQList, int topicID, ConRec * sub, int timeOut);
int         SubToTopic(TopicQ **pQList, ConRec *sub, int topicID);
int         WaitForTopics(TopicQ *qList, ConRec *sub, int *out);

#endif
