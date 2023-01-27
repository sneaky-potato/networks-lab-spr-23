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
// ## Assignment - 3                      ##
// #########################################
// # GCC version: gcc (GCC) 12.1.1 20220730

const unsigned PORT = 3000;
const unsigned BUF_SIZE = 50;
const unsigned LOCAL_BUF_SIZE = 500;

// recv and store string function prototype
void recv_str(int, char *, char *, int);

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;

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
    printf("%s\n", local_buf);

    close(sockfd);
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