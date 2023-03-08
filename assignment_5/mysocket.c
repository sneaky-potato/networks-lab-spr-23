#include "mysocket.h"

int my_socket(int __domain, int __type, int __protocol)
{
    int sockfd = socket(__domain, __type, __protocol);
    if (sockfd < 0)
    {
        perror("Error opening socket");
        exit(1);
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
    int send_status = send(__fd, __buf, __n, __flags);
    if (send_status < 0)
    {
        perror("Error sending message");
        exit(1);
    }
    return send_status;
}

int my_recv(int __fd, void *__buf, size_t __n, int __flags)
{
    int recv_status = recv(__fd, __buf, __n, __flags);
    if (recv_status < 0)
    {
        perror("Error receiving message");
        exit(1);
    }
    return recv_status;
}

int my_close(int __fd)
{
    int close_status = close(__fd);
    if (close_status < 0)
    {
        perror("Error closing socket");
        exit(1);
    }
    return close_status;
}
