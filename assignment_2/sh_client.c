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
// ## Assignment - 2                      ##
// #########################################
// # GCC version: gcc (GCC) 12.1.1 20220730

const unsigned PORT = 20000;
const unsigned BUF_SIZE = 50;
const unsigned USERNAME_SIZE = 25;
const unsigned LOCAL_BUF_SIZE = 500;

// recv and print function prototype
void recv_print(int, char *, int);
// recv and store string function prototype
void recv_str(int, char *, char *, int);

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;

    char *local_buf = (char *)malloc(sizeof(char) * LOCAL_BUF_SIZE);

    // 1 byte extra for null string
    char *buf = (char *)malloc(sizeof(char) * (BUF_SIZE + 1));

    // Server specification
    serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(PORT);

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Unable to create socket\n");
        exit(0);
    }

    // Connection request
    if ((connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        perror("Unable to connect to server\n");
        exit(0);
    }

    // Recieve result and print
    recv_print(sockfd, buf, BUF_SIZE);

    char username[USERNAME_SIZE];
    scanf("%s", username);
    getchar();

    strcpy(buf, username);

    // send username
    send(sockfd, buf, strlen(buf) + 1, 0);
    // recieve FOUND | NOT-FOUND
    recv_str(sockfd, local_buf, buf, BUF_SIZE);

    if (strcmp(local_buf, "NOT-FOUND") == 0)
    {
        printf("Invalid username\n");
        exit(EXIT_FAILURE);
    }
    else if (strcmp(local_buf, "FOUND"))
    {
        printf("Invalid protocol: %s %d\n", local_buf, strlen(local_buf));
        exit(EXIT_FAILURE);
    }

    // shell prompt on successful authentication
    printf("$");

    while (fgets(buf, BUF_SIZE, stdin))
    {
        // exit command
        if (strcmp(buf, "exit\n") == 0)
            exit(0);

        // trim trailing newline
        if (strlen(buf) > 0 && buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0';

        // send command
        send(sockfd, buf, strlen(buf) + 1, 0);

        // recv results in batches of size BUF_SIZE
        recv_str(sockfd, local_buf, buf, BUF_SIZE);

        // err cases
        if (strcmp(local_buf, "$$$$") == 0)
        {
            printf("Invalid command\n$");
            continue;
        }
        else if (strcmp(local_buf, "####") == 0)
        {
            printf("Error in running command\n$");
            continue;
        }

        // print results of recv
        printf("%s\n", local_buf);
        printf("$");
    }

    close(sockfd);
    return 0;
}

void recv_print(int sockfd, char *buf, int buf_size)
{
    int t;
    while ((t = recv(sockfd, buf, buf_size, 0)) >= 0)
    {
        int i;
        for (i = 0; i < t; i++)
        {
            if (buf[i] == '\0')
                break;
            printf("%c", buf[i]);
        }
        if (i < t)
            break;
    }
}

void recv_str(int sockfd, char *local_buf, char *buf, int buf_size)
{
    int t;
    // local_buf[0] = '\0';
    for (int i = 0; i < LOCAL_BUF_SIZE; i++)
        local_buf[i] = '\0';
    while ((t = recv(sockfd, buf, buf_size, 0)) > 0)
    {
        int i;
        for (i = 0; i < t; i++)
        {
            // if a null is found in current recv string
            if (buf[i] == '\0')
            {
                break;
            }
        }
        if (i < t)
        {
            // null was found, so concatenate and break
            strcat(local_buf, buf);
            break;
        }
        // null was not found, add null and concatenate
        buf[buf_size] = '\0';
        strcat(local_buf, buf);
    }
}