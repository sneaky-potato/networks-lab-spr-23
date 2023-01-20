#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// #########################################
// ## Ashwani Kumar Kamal (20CS10011)     ##
// ## Networks Laboratory                 ##
// ## Assignment - 2                      ##
// #########################################
// # GCC version: gcc (GCC) 12.1.1 20220730

int PORT = 20000;
const unsigned BUF_SIZE = 50;

int main()
{
    int sockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr, serv_addr;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Cannot create socket\n");
        exit(0);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(&cli_addr, 0, sizeof(cli_addr));

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

    printf("Server running on port: %d\nWaiting for incoming connections...\n", PORT);

    int bytes_recvd;

    char *buffer = (char *)malloc(sizeof(char) * BUF_SIZE);

    while (1)
    {

        clilen = sizeof(cli_addr);
        bytes_recvd = recvfrom(sockfd, (char *)buffer, BUF_SIZE, 0, (struct sockaddr *)&cli_addr, &clilen);

        if (bytes_recvd < 0)
        {
            perror("recvfrom  error\n");
            exit(EXIT_FAILURE);
        }

        buffer[bytes_recvd] = '\0';

        printf("%s\n", buffer);

        // Get current date time information from <time.h>
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);

        // Copy date time information
        strcpy(buffer, asctime(tm));
        // Send date time information
        sendto(sockfd, (char *)buffer, BUF_SIZE, 0, (struct sockaddr *)&cli_addr, clilen);
    }

    close(sockfd);

    return 0;
}