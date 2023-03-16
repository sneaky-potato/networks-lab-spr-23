/*
    CS39006 - Networks Laboratory, Spring Semester 2022-2023
    Assignment No: 5
    Group No: 16
    Members: Ashwani Kumar Kamal (20CS10011), Kartik Pontula (20CS10031)
    Program Synopsis: Message oriented TCP implementation library.
*/

#include "mysocket.h"

pthread_t sender = 0;
pthread_t receiver = 0;
pthread_mutex_t mutex_send = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_recv = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_sockfd = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_send_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_send_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_recv_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_recv_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_sockfd = PTHREAD_COND_INITIALIZER;

BUFFER *Send_Message = NULL;
BUFFER *Received_Message = NULL;

int global_sockfd = -1, global_flags = 0;

int min(int a, int b) { return (a < b) ? a : b; }

int my_socket(int __domain, int __type, int __protocol)
{
    if (__type != SOCK_MyTCP)
    { 
        perror("Error: Only SOCK_MyTCP is supported");
        exit(EXIT_FAILURE);
    }
    // Initialization
    if(!Send_Message)
        init_buffer(Send_Message);
    if(!Received_Message)
        init_buffer(Received_Message);

    // Thread creation
    if(!sender)
        if(pthread_create(&sender, NULL, send_routine, NULL) != 0) printf("Debug: S creation failed\n");
    if(!receiver)
        if(pthread_create(&receiver, NULL, receive_routine, NULL) != 0) printf("Debug: R creation failed\n");

    printf("Debug: S and R created\n");
    // Socket creation
    int sockfd = socket(__domain, SOCK_STREAM, __protocol);
    if (sockfd < 0)
    {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }
    printf("Debug: my_socket() successful\n");
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
    printf("Debug: my_bind() successful\n");
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
    printf("Debug: my_listen() successful\n");
    return listen_status;
}

int my_accept(int __fd, struct sockaddr *__addr, socklen_t *__restrict __addr_len)
{
    printf("Debug: Inside my_accept()\n");
    int accept_status = accept(__fd, __addr, __addr_len);
    if (accept_status < 0)
    {
        perror("Error accepting connection");
        exit(1);
    }
    // Set global_sockfd
    // pthread_mutex_lock(&mutex_sockfd);
    global_sockfd = accept_status;
    global_flags = 0;
    // pthread_mutex_unlock(&mutex_sockfd);
    // pthread_cond_signal(&cond_sockfd);
    printf("Debug: Global sockfd set by my_accept()\n");
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
    printf("Debug: connect() successful\n");
    // pthread_mutex_lock(&mutex_sockfd);
    global_sockfd = __fd;
    global_flags = 0;
    // pthread_mutex_unlock(&mutex_sockfd);
    // pthread_cond_signal(&cond_sockfd);
    printf("Debug: Global sockfd set by my_connect()\n");
    return connect_status;
}

int my_send(int __fd, const void *__buf, size_t __n, int __flags)
{
    printf("Debug: Inside my_send()\n");
    if(__fd != global_sockfd)
    {
        perror("Error: my_send() socket has no connection");
        exit(EXIT_FAILURE);
    }
    global_flags = __flags;
    // TODO: add error handling for n > MAX_MESSAGE_SIZE

    printf("Debug: my_send lock begins\n");
    pthread_mutex_lock(&mutex_send);
    while (Send_Message->size == MAX_BUFFER_SIZE)
    {
        pthread_cond_wait(&cond_send_full, &mutex_send);
    }
    printf("Debug: this ok?\n");
    enqueue(Send_Message, __buf, __n);
    pthread_mutex_unlock(&mutex_send);
    pthread_cond_signal(&cond_send_empty);
    printf("Debug: my_send() successful\n");
    return __n;
}

int my_recv(int __fd, void *__buf, size_t __n, int __flags)
{
    printf("Debug: Inside my_recv()\n");
    if(__fd != global_sockfd)
    {
        perror("Error: my_recv() socket has no connection");
        exit(EXIT_FAILURE);
    }
    // TODO: add error handling for n > MAX_MESSAGE_SIZE

    global_flags = __flags; // but R has already run recv without these flags xD


    pthread_mutex_lock(&mutex_recv);
    while (Received_Message->size == MAX_BUFFER_SIZE)
    {
        pthread_cond_wait(&cond_recv_full, &mutex_recv);
    }
    printf("Debug: this ok?\n");
    enqueue(Received_Message, __buf, __n);
    pthread_mutex_unlock(&mutex_recv);
    pthread_cond_signal(&cond_recv_empty);
    printf("Debug: my_recv lock begins\n");
    pthread_mutex_lock(&mutex_recv);
    while (Send_Message->size == 0)
    {
        pthread_cond_wait(&cond_send_empty, &mutex_recv);
    }
    Message *msgptr = dequeue(Received_Message);
    pthread_mutex_unlock(&mutex_recv);
    pthread_cond_signal(&cond_send_full);
    printf("Debug: RM dequeued by my_recv()\n");
    int msglen = msgptr->msglen;
    memmove(__buf, msgptr->msg, min(__n, msglen));

    free(msgptr);
    printf("Debug: my_recv() successful\n");
    return min(__n, msglen);
}

int my_close(int __fd)
{
    // sleep(5); // As instructed by AG

    pthread_mutex_lock(&mutex_sockfd);
    global_sockfd = -1;
    pthread_mutex_unlock(&mutex_sockfd);
    printf("Debug: global sockfd set to -1\n");
    pthread_cancel(sender);
    pthread_join(sender, NULL);
    printf("Debug: S terminated\n");
    pthread_cancel(receiver);
    pthread_join(receiver, NULL);
    printf("Debug: R terminated\n");

    if(Send_Message)
        dealloc_buffer(Send_Message);
    if(Received_Message)
        dealloc_buffer(Received_Message);
    printf("Buffers dealloced\n");

    int close_status = close(__fd);
    if (close_status < 0)
    {
        perror("Error closing socket");
        exit(1);
    }
    printf("Debug: my_close() successful\n");
    return close_status;
}

void *send_routine()
{
    char *buf = (char *)malloc(sizeof(char) * MAX_BUFFER_SIZE);
    while (1)
    {
        printf("S: Waiting for valid global_sockfd\n");
        // pthread_mutex_lock(&mutex_sockfd);
        while(global_sockfd == -1)
        {
            // pthread_cond_wait(&cond_sockfd, &mutex_sockfd);
            sleep(SEND_ROUTINE_TIMEOUT);
        }
        // pthread_mutex_unlock(&mutex_sockfd);
        // inelegant but works
        int sockfd = global_sockfd;
        int flags = global_flags;
        printf("S: Connected socket detected\n");
        printf("S: Waiting for message from Send_Message\n");
        pthread_mutex_lock(&mutex_send);
        while (Send_Message->size == 0) // S will stay blocked until my_send is run
        {
            pthread_cond_wait(&cond_send_empty, &mutex_send);
        }
        Message *msgptr = dequeue(Send_Message);
        pthread_mutex_unlock(&mutex_send);
        pthread_cond_signal(&cond_send_full);
        printf("S: Msg dequeued from Send_Message\n");
        int msglen = msgptr->msglen;
        memset(buf, 0, MAX_BUFFER_SIZE);
        memmove(buf, msgptr->msg, msglen);
        free(msgptr);
        printf("S: Beginning to call send()\n");
        // TODO: perhaps consider placing start and end markers around msglen
        if(send(sockfd, &(msglen), sizeof(int), flags) != sizeof(int))
            printf("Couldn't send msglen in one call\n");
        printf("S: Sent msglen\n");
        int nsend = 0;
        while (nsend < msglen)
        {
            int bytes_sent = send(sockfd, buf + nsend, min(MAX_CHUNK_SIZE, msglen - nsend), flags);
            printf("S: Sent %d bytes\n", bytes_sent);
            if (bytes_sent == -1)
                continue;
            nsend += bytes_sent;
        }
        printf("S: Sent total %d bytes, will sleep now\n", nsend);
        pthread_testcancel();
        sleep(SEND_ROUTINE_TIMEOUT);

    }
}

void *receive_routine()
{
    char *buf = (char *)malloc(sizeof(char) * MAX_BUFFER_SIZE);
    while (1)
    {
        printf("R: Waiting for valid global_sockfd\n");
        // pthread_mutex_lock(&mutex_sockfd);
        while(global_sockfd == -1)
        {
            // pthread_cond_wait(&cond_sockfd, &mutex_sockfd);
            sleep(RECV_ROUTINE_TIMEOUT);
        }
        // pthread_mutex_unlock(&mutex_sockfd);
        // inelegant again
        int sockfd = global_sockfd;
        int flags = global_flags;
        printf("R: Connected socket detected\n");
        printf("R: Beginning to call recv()\n");
        int msglen = -1;
        if(recv(sockfd, &(msglen), sizeof(int), flags) != sizeof(int))
            printf("Couldn't receive msglen in one call\n");
        printf("R: Received msglen %d\n", msglen);
        memset(buf, 0, MAX_BUFFER_SIZE);
        int nrecv = 0;
        while (nrecv < msglen)
        {
            int bytes_recved = recv(sockfd, buf + nrecv, min(MAX_CHUNK_SIZE, msglen - nrecv), flags);
            printf("R: Received %d bytes\n", bytes_recved);
            if (bytes_recved == -1) 
                continue;
            nrecv += bytes_recved;
        }
        printf("R: Received total %d bytes\n", nrecv);
        printf("R: Waiting for an empty slot in Received_Message\n");
        pthread_mutex_lock(&mutex_recv);
        while (Received_Message->size == MAX_BUFFER_SIZE)
        {
            pthread_cond_wait(&cond_recv_full, &mutex_recv);
        }
        enqueue(Received_Message, buf, msglen);
        pthread_mutex_unlock(&mutex_recv);
        pthread_cond_signal(&cond_recv_empty);
        printf("R: Enqueued msg to Received_Message\n");
        pthread_testcancel();
    }
}

void init_buffer(BUFFER *buffer)
{
    buffer = (BUFFER *)malloc(sizeof(BUFFER));
    printf("Debug: init malloc 1\n");
    (buffer)->list = (Message **)malloc(sizeof(Message *) * MAX_BUFFER_SIZE);
    printf("Debug: init malloc 2\n");
    for (int i = 0; i < MAX_BUFFER_SIZE; i++)
        (buffer)->list[i] = (Message *)malloc(sizeof(Message));
    printf("Debug: init malloc 3\n");
    (buffer)->size = 10;
    (buffer)->head = -1;
    (buffer)->tail = -1;
}

void dealloc_buffer(BUFFER *buffer)
{
    for (int i = 0; i < MAX_BUFFER_SIZE; i++)
        free((buffer)->list[i]);
    printf("Debug: dealloc 1\n");
    free((buffer)->list);
    printf("Debug: dealloc 2\n");
    free((buffer));
    printf("Debug: dealloc 3\n");
    buffer = NULL;
}

void enqueue(BUFFER *buffer, const char *message, int msglen)
{
    if (buffer->head == -1)
    {
        buffer->head = 0;
        buffer->tail = 0;
    }
    else
        buffer->tail = (buffer->tail + 1) % buffer->size;
    printf("Debug: enqueue fine upto now\n");
    Message *msgptr = (Message *)malloc(sizeof(Message));
    printf("Debug: enqueue malloc success\n");
    memmove(msgptr->msg, message, msglen);
    msgptr->msglen = msglen;
    buffer->list[buffer->tail] = msgptr;
    printf("Debug: enqueue done\n");
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