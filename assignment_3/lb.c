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
#include <poll.h>
#include <time.h>

// #########################################
// ## Ashwani Kumar Kamal (20CS10011)     ##
// ## Networks Laboratory                 ##
// ## Assignment - 3                      ##
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

    struct pollfd poll_set[1];
    int poll_result;

    time_t start, end;
    int server_a_load, server_b_load;

    int sockfd, newsockfd;
    int servesockfd;
    int clilen;
    struct sockaddr_in cli_addr, lb_addr;
    struct sockaddr_in sa_addr, sb_addr;

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

    // Load balancer specification
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

    // Server A specification
    sa_addr.sin_family = AF_INET;
    sa_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa_addr.sin_port = htons(porta);

    // Server B specification
    sb_addr.sin_family = AF_INET;
    sb_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sb_addr.sin_port = htons(portb);

    poll_set->fd = sockfd;
    poll_set->events = POLLIN;

    printf("Load balancer running on port: %d\nWaiting for incoming connections...\n", PORT);

    while (1)
    {
        if ((servesockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("Cannot create server socket\n");
            exit(0);
        }
        // Connection request to server A
        if ((connect(servesockfd, (struct sockaddr *)&sa_addr, sizeof(sa_addr))) < 0)
        {
            perror("Unable to connect to server A\n");
            exit(0);
        }
        strcpy(buf, "Send Load");

        send(servesockfd, buf, strlen(buf) + 1, 0);
        // Recieve load from server A
        recv(servesockfd, &server_a_load, sizeof(server_a_load), 0);

        printf("Load received from %s:%d = %d\n", inet_ntoa(sa_addr.sin_addr), porta, server_a_load);
        close(servesockfd);

        if ((servesockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("Cannot create server socket\n");
            exit(0);
        }
        // Connection request to server B
        if ((connect(servesockfd, (struct sockaddr *)&sb_addr, sizeof(sb_addr))) < 0)
        {
            perror("Unable to connect to server B\n");
            exit(0);
        }
        strcpy(buf, "Send Load");

        send(servesockfd, buf, strlen(buf) + 1, 0);
        // Recieve load from server B
        recv(servesockfd, &server_b_load, sizeof(server_b_load), 0);

        printf("Load received from %s:%d = %d\n", inet_ntoa(sb_addr.sin_addr), portb, server_b_load);
        close(servesockfd);

        // Get current time pointer
        start = time(NULL);
        end = start + 5;

        // wait for 5 seconds for a client request
        while (start < end)
        {
            // Get current remaining time from the 5 seconds
            int diff = difftime(end, start) * 1000;
            printf("Start of polling: %s\n", ctime(&start));

            // update start time
            start = time(NULL);

            // Poll with the remaining time
            if ((poll_result = poll(poll_set, 1, diff)) < 0)
            {
                perror("poll error");
                break;
            }
            // poll ended with a timeout
            if (poll_result == 0)
            {
                printf("No clients. Collecting loads\n");
                break;
            }

            clilen = sizeof(cli_addr);
            newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

            if (newsockfd < 0)
            {
                perror("Accept error\n");
                exit(0);
            }

            if (fork() == 0)
            {
                // close old socket
                close(sockfd);

                if ((servesockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                {
                    printf("Cannot create server socket\n");
                    exit(0);
                }

                if (server_a_load < server_b_load)
                {
                    // Connection request to server A
                    if (connect(servesockfd, (const struct sockaddr *)&sa_addr, sizeof(sa_addr)) < 0)
                    {
                        perror("Unable to connect to server A\n");
                        exit(0);
                    }
                    printf("\nSending client request to %s:%d\n", inet_ntoa(sa_addr.sin_addr), porta);
                }
                else
                {
                    // Connection request to server B
                    if (connect(servesockfd, (const struct sockaddr *)&sb_addr, sizeof(sb_addr)) < 0)
                    {
                        perror("Unable to connect to server B\n");
                        exit(0);
                    }
                    printf("\nSending client request to %s:%d\n", inet_ntoa(sb_addr.sin_addr), portb);
                }
                strcpy(buf, "Send Time");
                // Send time request to server (whichever got connected)
                send(servesockfd, buf, strlen(buf) + 1, 0);

                // Recieve time
                recv_str(servesockfd, local_buf, buf, BUF_SIZE);

                // Send time back to client in batches
                send_results(newsockfd, local_buf, buf, BUF_SIZE);

                // close connections
                close(servesockfd);
                close(newsockfd);
                exit(0);
            }
            close(newsockfd);
        }
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
