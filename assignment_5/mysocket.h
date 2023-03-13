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

// 0 so that it does not conflict with the other types
#define SOCK_MyTCP 0
#define MAX_BUFFER_SIZE 10
#define MAX_MESSAGE_SIZE 5000

typedef struct _BUFFER
{
    char **list;
    int size;
    int head;
    int tail;
} BUFFER;

int my_socket(int __domain, int __type, int __protocol);

int my_bind(int __fd, const struct sockaddr *__addr, socklen_t __len);
int my_listen(int __fd, int __n);
int my_accept(int __fd, struct sockaddr *__addr, socklen_t *__restrict __addr_len);
int my_connect(int __fd, const struct sockaddr *__addr, socklen_t __len);

int my_send(int __fd, const void *__buf, size_t __n, int __flags);
int my_recv(int __fd, void *__buf, size_t __n, int __flags);
int my_close(int __fd);