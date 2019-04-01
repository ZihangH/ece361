/*
** selectserver.c -- a cheezy multiperson chat server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "server.h"
#include "lookup_table.h"
//////////////////////////////////





///////////////////////////

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
//////////////////////////////////////
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char **argv)
{
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    char *port;

    char buf[256];    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    lookup_table table = table_init();

    if (argc == 1){
        port = (char * ) DEFAULT_PORT;
    }
    else if (argc == 2) port = argv[1];
    else fprintf(stderr, "usage: server [server_port]\n");

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }

                        memset(buf, 0, sizeof buf);
                        if ((nbytes = recv(newfd, buf, sizeof buf, 0)) == -1)
                            perror("recv");

                            //FIXME: check if user name is taken. (user name stored in buf)
                            //if user name taken, disconnect
                        if (table_insert(table, buf, newfd) == NULL){
                            send(newfd, "error: user name taken\n", 22, 0);
                            if(table_evict(table, buf)){
                                printf("evicted %s\n",buf);
                            }
                            else printf("ERROR: evict %s failed!\n", buf);
                            close(newfd);
                            FD_CLR(newfd, &master);
                        }

                            //FIXME: insert new user into lookup table

                            //FIXME: broadcast new user's name to all other users

                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }

                        //FIXME: evict disconnected user from lookup table
                        lookup_table_entry *evict_entry = table_query_id(table,i);
                        if (evict_entry == NULL) perror("evict");
                        else table_evict(table,evict_entry->user_name);
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        buf[nbytes] = '\0';
                        //printf("buf: %s", buf);
                        // we got some data from a client

                        // FIXME: handle different messages
                        // message types: 
                        // broadcast, unicast, list, exit
                        
                        char source_name[256] = {0}; //buffer for source username
                        char commandtype_buf[256] = {0}; //buffer for command type
                        char command_buf[256] = {0}; //buffer for command
                        char msg_buf[256] = {0}; //buffer for the actual message
                        char dest_buf[256] = {0}; //output message
                        char list_buf[256] = {0}; //buffer for list command

                        int name_index = 0;
                        int command_index = 0;
                        for(int i = 0; i < 255; ++i){
                            if((buf[i] == ':') && (name_index == 0)){
                                name_index = i;
                            }
                            if(buf[i] == ' '){
                                if(command_index == 1){
                                    command_index = i;
                                    break;
                                }
                                ++command_index;
                            }
                        }
                        strncpy(source_name, buf, name_index);
                        //printf("source name: %s\n", source_name);

                        //command + msg
                        strcpy(commandtype_buf, buf+name_index+2); 
                        //printf("commandtype: %s\n", commandtype_buf);
                        
                        if(command_index == 1){
                            strcpy(command_buf, commandtype_buf);
                        }
                        else{
                            //command
                            strncpy(command_buf, commandtype_buf, command_index-name_index-2);
                            //msg
                            strcpy(msg_buf, buf+command_index+1);
                        }
                        //printf("command: %s \n", command_buf);
                        //printf("message: %s\n", msg_buf);

                        
                        // list: send a list of connected user names to source user
                        // message format:
                        // source_user_name: list
                        if(strcmp(command_buf, "list\n") == 0){
                            //printf("aaaaaaaa\n");
                            table_dump(table, list_buf);
                            //printf("List of user: \n");
                            //printf("%s",list_buf);
                            if (send(i, "List of user: \n", 15, 0) == -1) {
                                perror("send");
                            }
                            if (send(i, list_buf, strlen(list_buf), 0) == -1) {
                                perror("send");
                            }
                        }
                        // exit: disconnect with source user
                        // message format:
                        // source_user_name: exit
                        else if(strcmp(command_buf, "exit\n") == 0){
                            
                            lookup_table_entry *entry = table_lookup(table, source_name);
                            if(entry == NULL){
                                printf("unexpected error...\n");
                            }
                            else{    
                                printf("Exited\n");
                                close(entry->sock_fd);
                                FD_CLR(entry->sock_fd, &master);
                                table_evict(table, source_name);
                            }
                        }
                        // broadcast: send message to all users except source user
                        // message format:
                        // source_user_name: broadcast [message]
                        else if(strcmp(command_buf, "broadcast") == 0){
                            sprintf(dest_buf,"%s: %s", source_name, msg_buf);
                            for(j = 0; j <= fdmax; j++) {
                                // send to everyone!
                                if (FD_ISSET(j, &master)) {
                                    // except the listener and ourselves
                                    if (j != listener && j != i) {
                                        if (send(j, dest_buf, strlen(dest_buf), 0) == -1) {
                                            perror("send");
                                        }
                                    }
                                }
                            }
                        }
                        // unicast: send message to target user
                        // message format:
                        // source_user_name: [target_user_name]: [message]
                        else{
                            lookup_table_entry *entry = table_lookup(table, command_buf);
                            if(entry == NULL){
                                if (send(i,"Invalid command, please re-enter.\n", 34, 0) == -1) {
                                    perror("send");
                                }
                            }
                            else{
                                // send to the target user
                                sprintf(dest_buf,"%s: %s", source_name, msg_buf);
                                if (strcmp(command_buf, entry->user_name) == 0) {
                                    if (send(entry->sock_fd, dest_buf, strlen(dest_buf), 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                        
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    table_destroy(table);
    return 0;
}
