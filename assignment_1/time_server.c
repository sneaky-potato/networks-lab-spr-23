#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

//#########################################
//## Ashwani Kumar Kamal (20CS10011)     ##
//## Networks Laboratory                 ##
//## Assignment - 1                      ##
//#########################################
//# GCC version: gcc (GCC) 12.1.1 20220730

int PORT = 20001;

int main()
{
	int sockfd, newsockfd;
	int clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;
	char buf[100];

	// Create socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

	// Server specification
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

	// Bind socket connection
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5);

    printf("Server running on port: %d\nWaiting for incoming connections...\n", PORT);

	while (1) {

		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

		if (newsockfd < 0) {
			perror("Accept error\n");
			exit(0);
		}
		
		char *client_ip = inet_ntoa(cli_addr.sin_addr);
		int client_port = ntohs(cli_addr.sin_port);

		// Get current date time information from <time.h>
		time_t t = time(NULL);
  		struct tm *tm = localtime(&t);

		// Copy date time information
  		strcpy(buf, asctime(tm));
		// Send date time information
		send(newsockfd, buf, strlen(buf) + 1, 0);
		// Recieve any string messages
		recv(newsockfd, buf, 100, 0);
		printf("Recieved: %s\n", buf);

		close(newsockfd);
	}
	return 0;
}
