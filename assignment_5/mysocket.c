#include "mysocket.h"

/*
    CS39006 - Networks Laboratory, Spring Semester 2022-2023
    Assignment No: 5
    Group No: 16
    Members: Ashwani Kumar Kamal (20CS10011), Kartik Pontula (20CS10031)
    Program Synopsis: Message oriented TCP implementation library.
*/

pthread_t sender, receiver;
BUFFER *Send_Message;
BUFFER *Received_Message;

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
    return connect_status;
}

int my_send(int __fd, const void *__buf, size_t __n, int __flags)
{
    // if Send_Message is full, sleep and check again
    // else, push (__fd, message) to Send_Message
}

int my_recv(int __fd, void *__buf, size_t __n, int __flags)
{
    // if Receive_Message is empty, sleep and check again
    // else, insert (__fd, NULL), wait for NULL to be non-NULL, retrieve the message from there
}

int my_close(int __fd)
{
    sleep(5); // As instructed by AG

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
    while(1)
    {
        sleep(SEND_ROUTINE_TIMEOUT);
        // check if any (__fd, message) exists [wait on condition]
        // chunk message into MAX_SEND_SIZE and send() until done
        pthread_testcancel();
    }
}

void *receive_routine()
{
    while(1)
    {
        // check for (__fd, NULL) [wait on condition]
        // recv() with the socket __fd until done
        // aggregate full message and update the tuple with the message
        pthread_testcancel();
    }
}

void init_buffer(BUFFER **buffer)
{
    *buffer = (BUFFER *)malloc(sizeof(BUFFER));
    (*buffer)->list = (char **)malloc(sizeof(char *) * MAX_BUFFER_SIZE);

    for (int i = 0; i < MAX_BUFFER_SIZE; i++)
        (*buffer)->list[i] = (char *)malloc(sizeof(char) * MAX_MESSAGE_SIZE);

    (*buffer)->size = 10;
    (*buffer)->head = -1;
    (*buffer)->tail = -1;
}

void dealloc_buffer(BUFFER **buffer)
{
    free((*buffer)->size);
    free((*buffer)->head);
    free((*buffer)->tail);
    for (int i = 0; i < MAX_BUFFER_SIZE; i++)
        free((*buffer)->list[i]);
    free((*buffer)->list);
    free((*buffer));
}

void enqueue(BUFFER *buffer, char *message)
{
    if (buffer->head == -1)
    {
        buffer->head = 0;
        buffer->tail = 0;
    }
    else
        buffer->tail = (buffer->tail + 1) % buffer->size;

    strcpy(buffer->list[buffer->tail], message);
}

char *dequeue(BUFFER *buffer)
{
    char *message = (char *)malloc(sizeof(char) * MAX_MESSAGE_SIZE);
    strcpy(message, buffer->list[buffer->head]);

    if (buffer->head == buffer->tail)
    {
        buffer->head = -1;
        buffer->tail = -1;
    }
    else
        buffer->head = (buffer->head + 1) % buffer->size;

    return message;
}