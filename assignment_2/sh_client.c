#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//#########################################
//## Ashwani Kumar Kamal (20CS10011)     ##
//## Networks Laboratory                 ##
//## Assignment - 2                      ##
//#########################################
//# GCC version: gcc (GCC) 12.1.1 20220730

const unsigned PORT = 20001;
const unsigned BUF_SIZE = 50;
const unsigned USERNAME_SIZE = 25;


int main()
{
	int sockfd ;
	struct sockaddr_in serv_addr;

	char *buf;
	char *result;

	buf = (char*)malloc(sizeof(char) * BUF_SIZE);

	// Recieving result string
	result = (char*)malloc(sizeof(char)*100);

	// Server specification
	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(PORT);

	for(int i=0; i < 6; i++) buf[i] = '\0';

	// Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	// Connection request
	if ((connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}
	
	
	// Recieve result
    recv(sockfd, buf, BUF_SIZE, 0);

    if(strcmp(buf, "LOGIN:") != 0) {
        printf("Invalid protocol: %s\n", buf);
        exit(EXIT_FAILURE);
    }

    printf("%s", buf);

    char username[USERNAME_SIZE];
    scanf("%s", username);
    getchar();

    strcpy(buf, username);

    // send username
    send(sockfd, buf, strlen(buf) + 1, 0);  
    // recieve FOUND | NOT-FOUND
    recv(sockfd, buf, BUF_SIZE, 0);

    if(strcmp(buf, "NOT-FOUND") == 0) {
        printf("Invalid username\n");
        exit(EXIT_FAILURE);
    } else if(strcmp(buf, "FOUND")) {
        printf("Invalid protocol: %s %d\n", buf, strlen(buf));
        exit(EXIT_FAILURE);
    }

    printf("$");

    while(fgets(buf, BUF_SIZE, stdin)) {
        if(strcmp(buf, "exit\n") == 0) exit(0);

        if(strlen(buf) > 0 && buf[strlen(buf) - 1] == '\n') buf[strlen(buf) - 1] = '\0';
        // send command
        send(sockfd, buf, strlen(buf) + 1, 0);

        // recieve results
        while(recv(sockfd, buf, BUF_SIZE, 0) >= 0) {

            if(strcmp(buf, "$$$$") == 0) {
                printf("Invalid  command");
                break;
            } else if(strcmp(buf, "####") == 0) {
                printf("Error in running command");
                break;
            }
            if(buf[0] == '\0') break;
            printf("%s", buf);
        }
        printf("\n");
        printf("$");
    }

    close(sockfd);
	return 0;
}
