#include <stdio.h>
#include <stdlib.h>
#include "mysocket.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Supply port number as command line argument");
        exit(EXIT_FAILURE);
    }

    int PORT = atoi(argv[1]);

    int sockfd;
    struct sockaddr_in serv_addr;

    char *buf = (char *)malloc(sizeof(char) * 100);

    // Recieving result string
    char *result = (char *)malloc(sizeof(char) * 100);

    // Server specification
    serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(PORT);

    // Create socket
    sockfd = my_socket(AF_INET, SOCK_MyTCP, 0);

    // Connection request
    my_connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    printf("Connected to server\n");

    printf("Enter expression: ");
    scanf("%s", buf);

    my_send(sockfd, buf, 100, 0);
    my_recv(sockfd, result, 100, 0);
    printf("Result: %s", result);

    my_close(sockfd);

    return 0;
}