#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// #########################################
// ## Ashwani Kumar Kamal (20CS10011)     ##
// ## Networks Laboratory                 ##
// ## Assignment - 3                      ##
// #########################################
// # GCC version: gcc (GCC) 12.1.1 20220730

const unsigned BUF_SIZE = 50;
const unsigned LOCAL_BUF_SIZE = 500;
const unsigned LOAD_UPPER = 100;
const unsigned LOAD_LOWER = 1;

// recv and store string function prototype
void recv_str(int, char *, char *, int);

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        printf("Supply port number as command line argument");
        exit(EXIT_FAILURE);
    }

    int PORT = atoi(argv[1]);
    int sockfd, newsockfd;
    int clilen;
    struct sockaddr_in cli_addr, serv_addr;

    // seed for random function
    srand((unsigned)PORT);

    // 1 byte extra for null string
    char *buf = (char *)malloc(sizeof(char) * (BUF_SIZE + 1));

    // local buf for recv and storing successively
    char *local_buf = (char *)malloc(sizeof(char) * LOCAL_BUF_SIZE);

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Cannot create socket\n");
        exit(0);
    }

    // Server specification
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Bind socket connection
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Unable to bind local address\n");
        exit(0);
    }

    listen(sockfd, 5);

    printf("Server running on port: %d\nWaiting for incoming connections...\n", PORT);

    while (1)
    {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if (newsockfd < 0)
        {
            perror("Accept error\n");
            exit(0);
        }

        // recv results
        recv_str(newsockfd, local_buf, buf, BUF_SIZE);

        if (strcmp(local_buf, "Send Load") == 0)
        {
            // printf("Sending \n");
            int load = (rand() % (LOAD_UPPER - LOAD_LOWER + 1)) + LOAD_LOWER;

            send(newsockfd, &load, sizeof(load), 0);
            printf("Load sent: %d\n", load);
        }
        else if (strcmp(local_buf, "Send Time") == 0)
        {
            // printf("Sending time\n");
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);

            // Copy date time information
            strcpy(buf, asctime(tm));
            // Send date time information
            send(newsockfd, buf, strlen(buf) + 1, 0);
        }
        else
        {
            printf("Something went wrong\n");
        }

        close(newsockfd);
    }
    return 0;
}

void recv_str(int sockfd, char *local_buf, char *buf, int buf_size)
{
    int t;
    local_buf[0] = '\0';
    // while there is something to recv
    while ((t = recv(sockfd, buf, buf_size, 0)) > 0)
    {
        int i;
        // check for null character
        for (i = 0; i < t; i++)
        {
            if (buf[i] == '\0')
                break;
        }
        if (i < t)
        {
            // null found
            strcat(local_buf, buf);
            break;
        }
        // no null found, add null
        buf[buf_size] = '\0';
        strcat(local_buf, buf);
    }
}
