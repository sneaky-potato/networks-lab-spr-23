#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// #########################################
// ## Ashwani Kumar Kamal (20CS10011)     ##
// ## Networks Laboratory                 ##
// ## Assignment - 1                      ##
// #########################################
// # GCC version: gcc (GCC) 12.1.1 20220730

const unsigned PORT = 20001;
const unsigned BUF_SIZE = 50;
const unsigned LOCAL_BUF_SIZE = 500;

void recv_str(int, char *, char *, int);

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;

    int i;
    char *local_buf = (char *)malloc(sizeof(char) * LOCAL_BUF_SIZE);
    // 1 byte extra for null string
    char *buf = (char *)malloc(sizeof(char) * (BUF_SIZE + 1));

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Unable to create socket\n");
        exit(0);
    }

    // Server specification
    serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(PORT);

    // Connection request
    if ((connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        perror("Unable to connect to server\n");
        exit(0);
    }

    strcpy(buf, "Send Time");

    // Send time request string
    send(sockfd, buf, strlen(buf) + 1, 0);
    printf("Send Time\n");
    // Recieve date time information
    recv_str(sockfd, local_buf, buf, BUF_SIZE);
    printf("recv = %s\n", local_buf);

    // strcpy(buf, "Send Load");

    // // Send time request string
    // send(sockfd, buf, strlen(buf) + 1, 0);
    // printf("Send Load\n");
    // // Recieve date time information
    // int load;
    // recv(sockfd, &load, sizeof(load), 0);
    // // recv_str(sockfd, , buf, BUF_SIZE);
    // printf("recv = %d\n", load);

    close(sockfd);
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