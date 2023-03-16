#include <stdio.h>
#include <stdlib.h>
#include "mysocket.h"
#include <signal.h>

int newsockfd;
// void ctrlc_handler(int signum) // signal handler to be executed upon Ctrl+C
// {
//     my_close(newsockfd);
//     exit(EXIT_SUCCESS);
// }

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Supply port number as command line argument\n");
        exit(EXIT_FAILURE);
    }
    int PORT = atoi(argv[1]);

    // signal(SIGINT, &ctrlc_handler);

    int sockfd;
    int clilen;
    struct sockaddr_in cli_addr, serv_addr;

    char *buf = (char *)malloc(sizeof(char) * 100);

    // Create socket
    sockfd = my_socket(AF_INET, SOCK_MyTCP, 0);

    // Server specification
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Connection request
    my_bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    my_listen(sockfd, 5);

    printf("Server running on port: %d\nWaiting for incoming connections...\n", PORT);

    // Accept connection from client
    printf("Waiting to accept a client...\n");
    clilen = sizeof(cli_addr);
    newsockfd = my_accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

    my_recv(newsockfd, buf, 100, 0);
    printf("Received: %s\n", buf);
    my_send(newsockfd, buf, strlen(buf) + 1, 0);

    my_close(sockfd);
    return 0;
}