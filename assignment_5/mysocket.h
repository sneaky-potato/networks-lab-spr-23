#ifndef __MYSOCKET_H
#define __MYSOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>

/*
    CS39006 - Networks Laboratory, Spring Semester 2022-2023
    Assignment No: 5
    Group No: 16
    Members: Ashwani Kumar Kamal (20CS10011), Kartik Pontula (20CS10031)
    Program Synopsis: Headers and definitions for Message oriented TCP library.
*/

// 0 so that it does not conflict with the other types
#define SOCK_MyTCP 0
#define MAX_BUFFER_SIZE 10
#define MAX_SEND_SIZE 1000
#define MAX_MESSAGE_SIZE 5000
#define SEND_ROUTINE_TIMEOUT 1

// Buffer struct (circular queue)
typedef struct _BUFFER // need to update: associate each message with a sockfd
{
    char **list;
    int size;
    int head;
    int tail;
} BUFFER;

// Socket functions
int my_socket(int __domain, int __type, int __protocol);

int my_bind(int __fd, const struct sockaddr *__addr, socklen_t __len);
int my_listen(int __fd, int __n);
int my_accept(int __fd, struct sockaddr *__addr, socklen_t *__restrict __addr_len);
int my_connect(int __fd, const struct sockaddr *__addr, socklen_t __len);

int my_send(int __fd, const void *__buf, size_t __n, int __flags);
int my_recv(int __fd, void *__buf, size_t __n, int __flags);
int my_close(int __fd);

// Helper functions
void *send_routine();
void *receive_routine();
void init_buffer(BUFFER **buffer);
void dealloc_buffer(BUFFER **buffer);
void enqueue(BUFFER *buffer, char *message);
char *dequeue(BUFFER *buffer);

#endif