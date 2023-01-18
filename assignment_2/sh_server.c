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

const unsigned PORT = 20001;
const unsigned BUF_SIZE = 50;
const unsigned USERNAME_SIZE = 25;

void send_results(int, char *, int);

int main()
{
	int sockfd, newsockfd;
	int clilen;
	struct sockaddr_in cli_addr, serv_addr;

	char *buf = (char *)malloc(sizeof(char) * BUF_SIZE);

	// Create socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Cannot create socket\n");
		exit(0);
	}

	// Server specification
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

	// Connection request
	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5);

	printf("Server running on port: %d\nWaiting for incoming connections...\n", PORT);

	while (1)
	{
		// Accept connection fromm client
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

		if (newsockfd < 0)
		{
			printf("Accept error\n");
			exit(0);
		}

		// Fork server process
		if (fork() == 0)
		{
			// close old socket
			close(sockfd);

			strcpy(buf, "LOGIN:");
			send(newsockfd, buf, strlen(buf) + 1, 0);

			recv(newsockfd, buf, BUF_SIZE, 0);

			char username_recvd[USERNAME_SIZE];
			strcpy(username_recvd, buf);

			char *filename = "users.txt";
			FILE *fp = fopen(filename, "r");

			if (fp == NULL)
			{
				printf("Error: could not open file %s", filename);
				exit(EXIT_FAILURE);
			}

			char username_buffer[USERNAME_SIZE];
			strcpy(buf, "NOT-FOUND");

			// check for username validity
			while (fgets(username_buffer, USERNAME_SIZE, fp))
			{
				// trim newline
				if (strlen(username_buffer) > 0 && username_buffer[strlen(username_buffer) - 1] == '\n')
					username_buffer[strlen(username_buffer) - 1] = '\0';
				// compare username
				if (strcmp(username_buffer, username_recvd) == 0)
				{
					strcpy(buf, "FOUND");
					break;
				}
			}
			// close the file
			fclose(fp);
			// send username FOUND | NOT-FOUND
			send(newsockfd, buf, strlen(buf) + 1, 0);

			// recieve commands
			while (recv(newsockfd, buf, BUF_SIZE, 0) > 0)
			{
				char *result = (char *)malloc(sizeof(char) * 256);

				if (strcmp(buf, "pwd") == 0)
				{
					if (getcwd(result, 256) == NULL)
					{
						result = "####";
					}

					printf("%s\n", result);
				}
				else if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ')
				{
					char *dir = &buf[3];

					if (chdir(dir) != 0)
					{
						result = "####";
					}
				}
				else
				{
					result = "$$$$";
				}
				// strcat(result, result);
				// result = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";
				// printf("%s\n", result);

				send_results(newsockfd, result, BUF_SIZE - 1);
			}

			// close connection
			close(newsockfd);
			exit(0);
		}

		close(newsockfd);
	}
	return 0;
}

void send_results(int sockfd, char *to_send, int buf_size)
{
	int n = strlen(to_send);
	int send_n = (n + buf_size - 1) / buf_size;

	char *buf = (char *)malloc(sizeof(char) * (buf_size + 1));
	int r = 0;

	for (int i = 0; i < send_n; i++)
	{
		int j;
		for (j = 0; j < buf_size; j++)
			buf[j] = '\0';

		for (j = 0; j < buf_size; j++)
		{
			if (r < n)
				buf[j] = to_send[r++];
		}
		buf[j] = '\0';
		int t = send(sockfd, buf, strlen(buf), 0);
		printf("to send=%s %d\n", buf, t);
	}
	buf[0] = '\0';
	int t = send(sockfd, buf, 1, 0);
	printf("to send=%s %d\n", buf, t);
}
