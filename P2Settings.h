#ifndef CIS415_P2_Settings
#define CIS415_P2_Settings

//Adjustable settings
//#define VERBOSE
#define MESSAGE_TIMEOUT 0
#define MESSAGE_SENDING_INTERVAL 1
#define MESSAGE_RECEIVING_INTERVAL 0
#define ARCH_BUFF_SIZE 5
#define MAXENTRIES 50

//Init numbers and constant strings
#define MSG_PUB_IDENTIFIER  "pub"
#define MSG_SUB_IDENTIFIER  "sub"
#define MSG_SUB_RECEIVING   "receiving messages"
#define MSG_TOPIC           "topic"
#define MSG_CONNECT         "connect"
#define MSG_ACCEPT          "accept"
#define MSG_SUCCESS         "success"
#define MSG_RETRY           "retry"
#define MSG_TERMINATE       "terminate"
#define MSG_TOPIC_BROADCAST 0

#define INIT_LIST_MAX 20
#define INIT_TOPIC_NUM 10

#define ENTRYSIZE 250
#define MAX_BUFF_LEN 250

#endif
