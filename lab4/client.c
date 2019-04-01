/*
** client.c -- a stream socket client demo
*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "client.h"


void sig_handler(){
    pthread_exit(NULL);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sender_main(client_data* data){
    //printf("In sender_main\n");
    struct sigaction my_action;
    my_action.sa_handler = sig_handler;
    sigaction(SIGUSR1, &my_action, NULL);
    while(1){
        fgets(data->send_buf, MAX_DATA_SIZE, stdin);
        char temp_buf[MAX_DATA_SIZE];
        memset(temp_buf, 0, MAX_DATA_SIZE);
        sprintf(temp_buf, "%s: %s", data->user_name, data->send_buf);
        // printf("%s\n", temp_buf);
        // printf("%s\n",data->send_buf);
        int numbytes = 0;
        if((numbytes = send(data->sockfd, temp_buf, strlen(temp_buf), 0)) == -1) {
        // if((numbytes = send(data->sockfd, data->send_buf, strlen(data->send_buf), 0)) == -1) {
            perror("send");
            exit(1);
        }
        memset(data->send_buf, 0, MAX_DATA_SIZE);
    }
}

void receiver_main(client_data* data){
    // printf("In receiver_main\n");
    int numbytes = 0;
    while(1){
        if ((numbytes = recv(data->sockfd, data->recv_buf, MAX_DATA_SIZE-1, 0)) <= 0) {
            if (numbytes < 0){
                perror("recv");
            }
            else {
                printf("connection closed.\nexiting...\n");
            }
            close(data->sockfd);
            pthread_kill(data->sender_thread,SIGUSR1);
            pthread_exit(NULL);
        }
        else{
            data->recv_buf[numbytes] = '\0';
            printf("%s", data->recv_buf);
            memset(data->recv_buf, 0, MAX_DATA_SIZE);
        }
    }
}

int main(int argc, char *argv[])
{
    int numbytes;  
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    client_data data;
    memset(&data, 0, sizeof data);

    if (argc != 4) {
        fprintf(stderr,"usage: client [server_address] [server_port] [user_name]\n");
        exit(1);
    }

    if (strlen(argv[3]) > USER_NAME_LEN){
        fprintf(stderr,"error: user name length <= %d\n", USER_NAME_LEN);
        exit(1);
    }
    strncpy(data.user_name,argv[3],USER_NAME_LEN);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((data.sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(data.sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(data.sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    if((numbytes = send(data.sockfd, data.user_name, strlen(data.user_name), 0)) == -1) {
        perror("send");
        exit(1);
    }

    printf("client: sent '%s'\n", data.user_name);

    // if ((numbytes = recv(data.sockfd, data.recv_buf, MAX_DATA_SIZE-1, 0)) <= 0) {
    //     if (numbytes < 0){
    //         perror("recv");
    //     }
    //     else {
    //         fprintf(stderr, "error: user name taken.\nexiting...\n");
    //     }
    //     close(data.sockfd);
    //     exit(1);
    // }

    // data.recv_buf[numbytes] = '\0';

    // printf("%s\n",data.recv_buf);
    memset(data.recv_buf, 0, sizeof data.recv_buf);

    pthread_create(&(data.sender_thread), NULL, (void *)&sender_main, &data);
    pthread_create(&(data.receiver_thread), NULL, (void *)&receiver_main, &data);

    // printf("in main, joining sender\n");
    pthread_join(data.sender_thread,NULL);
    // printf("in main, joining receiver\n");
    pthread_join(data.receiver_thread,NULL);
    // printf("in main, closing connection.\n");
    close(data.sockfd);
    return 0;
}
