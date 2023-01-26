#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>

// #########################################
// ## Ashwani Kumar Kamal (20CS10011)     ##
// ## Networks Laboratory                 ##
// ## Assignment - 2                      ##
// #########################################
// # GCC version: gcc (GCC) 12.1.1 20220730

const unsigned BUF_SIZE = 50;
const unsigned LOCAL_BUF_SIZE = 500;
const unsigned USERNAME_SIZE = 25;

// send results in batches function prototype
void send_results(int, char *, char *, int);
// recv and store string function prototype
void recv_str(int, char *, char *, int);

int main(int argc, char const *argv[])
{
    if (argc < 4)
    {
        printf("Supply port numbers as command line argument");
        exit(EXIT_FAILURE);
    }

    int PORT = atoi(argv[1]);
    int porta = atoi(argv[2]);
    int portb = atoi(argv[3]);

    int sockfd, newsockfd;
    int clilen;
    struct sockaddr_in cli_addr, lb_addr;

    // 1 byte extra for null string
    char *buf = (char *)malloc(sizeof(char) * (BUF_SIZE + 1));

    // local buf for recv and storing successively
    char *local_buf = (char *)malloc(sizeof(char) * LOCAL_BUF_SIZE);

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Cannot create socket\n");
        exit(0);
    }

    // Server specification
    lb_addr.sin_family = AF_INET;
    lb_addr.sin_addr.s_addr = INADDR_ANY;
    lb_addr.sin_port = htons(PORT);

    // Connection request
    if (bind(sockfd, (struct sockaddr *)&lb_addr, sizeof(lb_addr)) < 0)
    {
        perror("Unable to bind local address\n");
        exit(0);
    }

    listen(sockfd, 5);

    printf("Load balancer running on port: %d\nWaiting for incoming connections...\n", PORT);

    while (1)
    {
        // Accept connection fromm client
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if (newsockfd < 0)
        {
            printf("Accept error\n");
            exit(0);
        }

        // Fork server process
        if (fork() == 0)
        {
            // close old socket
            close(sockfd);

            // send login prompt
            strcpy(buf, "LOGIN:");
            send(newsockfd, buf, strlen(buf) + 1, 0);

            // recv username
            recv_str(newsockfd, local_buf, buf, BUF_SIZE);

            close(newsockfd);
            exit(0);
        }

        close(newsockfd);
    }
    return 0;
}

void recv_str(int sockfd, char *local_buf, char *buf, int buf_size)
{
    int t;
    local_buf[0] = '\0';
    while ((t = recv(sockfd, buf, buf_size, 0)) > 0)
    {
        int i;
        for (i = 0; i < t; i++)
        {
            if (buf[i] == '\0')
                break;
        }
        if (i < t)
        {
            strcat(local_buf, buf);
            break;
        }
        buf[buf_size] = '\0';
        strcat(local_buf, buf);
    }
}

void send_results(int sockfd, char *to_send, char *buf, int buf_size)
{
    int n = strlen(to_send);
    // (n + 1) for accountig for null character of to_send
    // number of send calls to send data completely (ceil ((n+1) / buf_size))
    int send_n = ((n + 1) + buf_size - 1) / buf_size;

    int r = 0;

    for (int i = 0; i < send_n; i++)
    {
        int j;
        for (j = 0; j < buf_size; j++)
            buf[j] = '\0';

        for (j = 0; j < buf_size; j++)
        {
            if (r <= n)
            {
                // copy till null character
                buf[j] = to_send[r++];
            }
        }
        int t = send(sockfd, buf, (strlen(buf) + 1 > BUF_SIZE ? BUF_SIZE : strlen(buf) + 1), 0);
    }
}
