#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#define DEFAULT_SERVER "localhost" //default address client will be connecting to
#define DEFAULT_PORT "9034" // default port client will be connecting to 
#define MAX_DATA_SIZE 100 // max number of bytes we can get at once 
#define USER_NAME_LEN 20   //max length of USER NAME
typedef struct client_data{
    int sockfd;
    pthread_t sender_thread;
    pthread_t receiver_thread;
    char user_name[USER_NAME_LEN];
    char send_buf[MAX_DATA_SIZE];
    char recv_buf[MAX_DATA_SIZE];
}client_data;

#endif //client.h