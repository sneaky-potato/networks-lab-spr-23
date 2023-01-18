#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>

// #########################################
// ## Ashwani Kumar Kamal (20CS10011)     ##
// ## Networks Laboratory                 ##
// ## Assignment - 2                      ##
// #########################################
// # GCC version: gcc (GCC) 12.1.1 20220730

int PORT = 20000;

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;
    struct pollfd poll_set[1];

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Unable to create socket\n");
        exit(0);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    // Server specification
    serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(PORT);

    int n;
    socklen_t len;
    int attempts = 0;

    for (attempts = 0; attempts < 5; attempts++)
    {
        char *buffer = (char *)malloc(sizeof(char) * 100);

        char *hello = "time request";
        sendto(sockfd, (const char *)hello, strlen(hello), 0, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));

        poll_set[0].fd = sockfd;
        poll_set[0].events = POLLIN;

        int poll_result;
        int bytes_recvd;
        socklen_t servlen;

        if ((poll_result = poll(poll_set, 1, 3000)) < 0)
        {
            perror("poll error");
            exit(EXIT_FAILURE);
        }

        if (poll_result == 0)
        {
            continue;
        }

        if (poll_set[0].revents & POLLIN)
        {
            bytes_recvd = recvfrom(sockfd, (char *)buffer, 100, 0, (struct sockaddr *)&serv_addr, &servlen);

            if (bytes_recvd < 0)
            {
                perror("recvfrom  error\n");
                exit(EXIT_FAILURE);
            }

            buffer[bytes_recvd] = '\0';
            printf("%s\n", buffer);

            break;
        }
    }
    if (attempts == 5)
        printf("Timeout exceeded\n");

    close(sockfd);
    return 0;
}