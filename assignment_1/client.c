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
//## Assignment - 1                      ##
//#########################################
//# GCC version: gcc (GCC) 12.1.1 20220730

int PORT = 20000;
int BUF_SIZE = 6;

int main()
{
	int sockfd ;
	struct sockaddr_in serv_addr;

	int i;
	char *buf;
	char *result;

	buf = (char*)malloc(sizeof(char) * BUF_SIZE);

	// Recieving result string
	result = (char*)malloc(sizeof(char)*100);

	// Server specification
	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(PORT);

	for(i=0; i < 6; i++) buf[i] = '\0';

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
	
    printf("Connected to server\n");
	printf("Press enter after entering your request expression\n");

	while(1) {
		int terminate = 0;

		// Keep reading input in packets of size BUF_SIZE
		while(fgets(buf, BUF_SIZE, stdin)) {
			// Terminate client on special input -1\n
		    if(buf[0] == '-' && buf[1] == '1' && buf[2] == '\n') 
		    {
			    printf("Terminated\n");
				terminate = 1;
			    break;
		    }

			// Send packet string of size BUF_SIZE
            printf("Sending string %s\n", buf);
            send(sockfd, buf, BUF_SIZE, 0);

			// Flag for checking if newline is present in packet or not
            int f=0;
            for(int i=0; i < BUF_SIZE - 1; i++) {
                if(buf[i] =='\n') f = 1;
            }
			
			// If a newline is found in a packet, stop reading and proceed to recieve result from server
            if(f) break;
        }

		// Termination
		if(terminate) break;

		// Recieve result
        recv(sockfd, result, 100, 0);
		printf("::Recieved result\n%s\n", result);

        printf("Press enter after entering your request expression\n");
        
		for(int i=0; i<100; i++) buf[i] = '\0';
		
	}

    close(sockfd);
	return 0;
}
