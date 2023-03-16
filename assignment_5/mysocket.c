/*
    CS39006 - Networks Laboratory, Spring Semester 2022-2023
    Assignment No: 5
    Group No: 16
    Members: Ashwani Kumar Kamal (20CS10011), Kartik Pontula (20CS10031)
    Program Synopsis: Message oriented TCP implementation library.
*/

#include "mysocket.h"

pthread_t sender, receiver;
pthread_mutex_t mutex_send;
pthread_mutex_t mutex_recv;
pthread_mutex_t mutex_sockfd;
pthread_cond_t cond_send_full;
pthread_cond_t cond_send_empty;
pthread_cond_t cond_recv_full;
pthread_cond_t cond_recv_empty;
pthread_cond_t cond_sockfd;

BUFFER *Send_Message;
BUFFER *Received_Message;

int global_sockfd = -1, global_flags = 0;
int ret_send = -1, ret_recv = -1;

int my_socket(int __domain, int __type, int __protocol)
{
    if (__type != SOCK_MyTCP)
    {
        perror("Error: Only SOCK_MyTCP is supported");
        exit(EXIT_FAILURE);
    }

    // Initializing the buffers
    init_buffer(&Send_Message);
    init_buffer(&Received_Message);

    pthread_mutex_init(&mutex_send, NULL);
    pthread_mutex_init(&mutex_recv, NULL);
    pthread_mutex_init(&mutex_sockfd, NULL);

    pthread_cond_init(&cond_send_empty, NULL);
    pthread_cond_init(&cond_send_full, NULL);
    pthread_cond_init(&cond_recv_empty, NULL);
    pthread_cond_init(&cond_recv_full, NULL);
    pthread_cond_init(&cond_sockfd, NULL);

    // Creating the threads
    pthread_create(&sender, NULL, send_routine, NULL);
    pthread_create(&receiver, NULL, receive_routine, NULL);

    // Creating a tcp socket
    int sockfd = socket(__domain, SOCK_STREAM, __protocol);
    if (sockfd < 0)
    {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

int my_bind(int __fd, const struct sockaddr *__addr, socklen_t __len)
{
    int bind_status = bind(__fd, __addr, __len);
    if (bind_status < 0)
    {
        perror("Error binding socket");
        exit(1);
    }
    return bind_status;
}

int my_listen(int __fd, int __n)
{
    int listen_status = listen(__fd, __n);
    if (listen_status < 0)
    {
        perror("Error listening on socket");
        exit(1);
    }
    return listen_status;
}

int my_accept(int __fd, struct sockaddr *__addr, socklen_t *__restrict __addr_len)
{
    int accept_status = accept(__fd, __addr, __addr_len);
    if (accept_status < 0)
    {
        perror("Error accepting connection");
        exit(1);
    }

    pthread_mutex_lock(&mutex_sockfd);
    global_sockfd = accept_status;
    global_flags = 0;
    pthread_mutex_unlock(&mutex_sockfd);
    pthread_mutex_signal(&cond_sockfd);

    return accept_status;
}

int my_connect(int __fd, const struct sockaddr *__addr, socklen_t __len)
{
    int connect_status = connect(__fd, __addr, __len);
    if (connect_status < 0)
    {
        perror("Error connecting to server");
        exit(1);
    }

    pthread_mutex_lock(&mutex_sockfd);
    global_sockfd = connect_status;
    global_flags = 0;
    pthread_mutex_unlock(&mutex_sockfd);
    pthread_mutex_signal(&cond_sockfd);

    return connect_status;
}

int my_send(int __fd, const void *__buf, size_t __n, int __flags)
{
    if(__fd != global_sockfd)
    {
        perror("Error: Socket has no connection");
        exit(EXIT_FAILURE);
    }
    global_flags = __flags;

    /* TODO: decide whether this should be done instead
    while(Send_Message->size == MAX_BUFFER_SIZE)
    {
        sleep(MYSEND_CALL_TIMEOUT);
    }
    */
    pthread_mutex_lock(&mutex_send);
    while (Send_Message->size == MAX_BUFFER_SIZE)
    {
        pthread_cond_wait(&cond_send_full, &mutex_send);
    }

    enqueue(Send_Message, __buf, __n);
    pthread_mutex_unlock(&mutex_send);
    pthread_cond_signal(&cond_send_empty);

    return __n;
}

int my_recv(int __fd, void *__buf, size_t __n, int __flags)
{
    if(__fd != global_sockfd)
    {
        perror("Error: Socket has no connection");
        exit(EXIT_FAILURE);
    }
    global_flags = __flags; // but R has already run recv without these flags xD

    // TODO: decide whether to sleep & repeat or cond_wait
    while (Received_Message->size == 0)
    {
        sleep(MYRECV_CALL_TIMEOUT);
    }

    Message *msgptr = dequeue(Received_Message);
    // add code here to store msg into buf
    // handle cases where msgptr->msglen > __n etc
    return 1;
}

int my_close(int __fd)
{
    // sleep(5); // As instructed by AG

    pthread_mutex_lock(&mutex_sockfd);
    global_sockfd = -1;
    pthread_mutex_unlock(&mutex_sockfd);

    pthread_cancel(sender);
    pthread_join(sender, NULL);
    pthread_cancel(receiver);
    pthread_join(receiver, NULL);

    dealloc_buffer(&Send_Message);
    dealloc_buffer(&Received_Message);

    int close_status = close(__fd);
    if (close_status < 0)
    {
        perror("Error closing socket");
        exit(1);
    }
    return close_status;
}

void *send_routine()
{
    while (1)
    {
        pthread_mutex_lock(&mutex_sockfd);
        while(global_sockfd == -1)
            pthread_cond_wait(&cond_sockfd, &mutex_sockfd);
        pthread_mutex_unlock(&mutex_sockfd);

        pthread_mutex_lock(&mutex_send);
        while (Send_Message->size == 0) // S will stay blocked until my_send is run
            pthread_cond_wait(&cond_send_empty, &mutex_send);

        Message *msgptr = dequeue(Send_Message);
        pthread_mutex_unlock(&mutex_send);
        pthread_cond_signal(&cond_send_full);

        int msglen = msgptr->msglen;
        int sockfd = global_sockfd;
        int flags = global_flags;

        char *buf = (char *)malloc(sizeof(char) * (msglen + sizeof(int)));
        // prepend the message length to the message
        memmove(buf, &msglen, sizeof(int));
        memmove(buf + sizeof(int), msgptr->msg, msglen);

        int nsend = 0;

        while (nsend < msglen + sizeof(int))
        {
            int bytes_sent = send(sockfd, buf + nsend, min(MAX_SEND_SIZE, msglen + sizeof(int) - nsend), flags);
            if (bytes_sent == -1)
                continue;
            nsend += bytes_sent;
        }
        ret_send = nsend;
        pthread_testcancel();
        sleep(SEND_ROUTINE_TIMEOUT);

    }
}

void *receive_routine()
{
    /* brief and vague pseudocode:
    wait while global_sockfd is -1 DONE
    try running recv
    if returns 0, try again
    basically acquire whatever is being sent, of size msglen
    acquire msglen, keep running recv only till that len, discard all else (think about pseudocode here)
    package msg
    push to buffer
    */

    // TODO: use cond_wait instead
    while (1)
    {
        pthread_mutex_lock(&mutex_sockfd);
        while(global_sockfd == -1)
            pthread_cond_wait(&cond_sockfd, &mutex_sockfd);
        pthread_mutex_unlock(&mutex_sockfd);

        // insert big brain recv'ing code here
        char* msg = NULL;
        int msglen = -1;

        pthread_mutex_lock(&mutex_recv);
        while (Received_Message->size == MAX_BUFFER_SIZE)
        {
            pthread_cond_wait(&cond_recv_full, &mutex_recv);
        }
        pthread_mutex_unlock(&mutex_recv);
        enqueue(Received_Message, msg, msglen);
        pthread_cond_signal(&cond_recv_empty);

        // ret_recv = // TODO: global retval
        pthread_testcancel();
    }
}

void init_buffer(BUFFER **buffer)
{
    *buffer = (BUFFER *)malloc(sizeof(BUFFER));
    (*buffer)->list = (Message **)malloc(sizeof(Message *) * MAX_BUFFER_SIZE);

    for (int i = 0; i < MAX_BUFFER_SIZE; i++)
        (*buffer)->list[i] = (Message *)malloc(sizeof(Message));

    (*buffer)->size = 10;
    (*buffer)->head = -1;
    (*buffer)->tail = -1;
}

void dealloc_buffer(BUFFER **buffer)
{
    for (int i = 0; i < MAX_BUFFER_SIZE; i++)
        free((*buffer)->list[i]);
    free((*buffer)->list);
    free((*buffer));
}

void enqueue(BUFFER *buffer, char *message, int msglen)
{
    if (buffer->head == -1)
    {
        buffer->head = 0;
        buffer->tail = 0;
    }
    else
        buffer->tail = (buffer->tail + 1) % buffer->size;

    Message *msgptr = (Message *)malloc(sizeof(Message));
    memmove(msgptr->msg, message, msglen);
    msgptr->msglen = msglen;
    buffer->list[buffer->tail] = msgptr;
}

Message *dequeue(BUFFER *buffer)
{
    Message *msgptr = buffer->list[buffer->head];
    // buffer->list[buffer->head] = NULL;
    if (buffer->head == buffer->tail)
    {
        buffer->head = -1;
        buffer->tail = -1;
    }
    else
        buffer->head = (buffer->head + 1) % buffer->size;

    return msgptr;
}