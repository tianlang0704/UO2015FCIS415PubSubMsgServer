//=========================P2TopicManagement.c==============================
//==========================================================================
//==============Functions for managing and storing topics===================
//==========================================================================
#include "P2TopicManagement.h"

#define INIT_TOPIC_NUM 10

void AddTopic(ConRec *target, int topic)
{
	if(target->topicMax == 0)
	{
		target->topic = malloc(INIT_TOPIC_NUM * sizeof(int));
		target->topicMax = INIT_TOPIC_NUM;
	}
	else if(target->topicMax == target->topicNum)
	{
		target->topicMax *= 2;
		target->topic = realloc(target->topic, target->topicMax);
	}
	
	target->topic[target->topicNum] = topic;
	target->topicNum++;
}

void EmptyTopics(ConRec *target)
{
	if(target->topic != NULL)
	{
		free(target->topic);
		target->topic = NULL;
		target->topicNum = 0;
		target->topicMax = 0;
	}

}


