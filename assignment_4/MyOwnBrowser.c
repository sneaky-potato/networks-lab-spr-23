/* 
    CS39006 - Networks Laboratory, Spring Semester 2022-2023
    Assignment 4
    Name: Kartik Pontula
    Roll No: 20CS10031
    Program Synopsis: Client-side socket program for HTTP requests
    Usage:
        gcc MyOwnBrowser.c -o cli
        ./cli
*/
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/poll.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

#define BUFSIZE 256
void showError(char* s)
{
    fprintf(stderr, "ERROR: %s\n", s);
    exit(EXIT_FAILURE);
}

int main()
{
    int sockfd, port;
    char buf[BUFSIZE], *cmd, *url, *filename, *urlcpy, *ip;
    // char ws[] = {'\n', '\r', '\a', '\t', ' ', '\0'};
    char *ws = " \n\r\a\t", *ipsep = "/:";
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) showError("Socket creation failure.");
    // open user prompt, store result and shit
    // memset(&servaddr, 0, sizeof(servaddr));
    // servaddr.sin_family = htons(AF_INET);
    // servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // servaddr.sin_port = htons(SERVPORT);

    for(;1;)
    {
        fflush(stdin);
        printf("MyOwnBrowser> ");
        // accept input
        // extract cmd, url, IP, port
        memset(buf, 0, BUFSIZE);
        fgets(buf, BUFSIZE, stdin);
        buf[strlen(buf) - 1] = '\0';
        cmd = strtok(buf, ws); if(!cmd){printf("Please enter a non-empty command\n"); continue;}
        if(strcmp(cmd, "QUIT") == 0)
        {
            if(strtok(NULL, ws)) printf("Format: QUIT\n");
            printf("Client terminated.\n"); break;
        }
        else if(strcmp(cmd, "GET") == 0)
        {
            url = strtok(NULL, ws); if(!url) {printf("Missing URL\n"); continue;}
            if(strtok(NULL, ws)) printf("Format: GET <url>\n");
            // code to parse IP and port
            // NOTE: Don't use urlcpy outside this else-if block
            urlcpy = (char*)malloc((strlen(url)+1)*sizeof(char)); strcpy(urlcpy, url);

            if(strstr(urlcpy, "http://")!=urlcpy){printf("Enter a URL that begins with http:// \n"); continue;}
            urlcpy += strlen("http://");
            char* portbegin = rindex((const char*)urlcpy, ':');
            ip = strtok(urlcpy, ipsep); if(!ip){printf("what did you even enter to get this prompt?\n"); continue;}
            // at this point in the code, urlcpy is <IPADDR>/etc/filename.jpg (with port number)
            if(portbegin)
            {
                sscanf(portbegin+1, "%d", &port);
                // ERROR: port entered is not short int
            }
            else port = 80;
            printf("URL is %s\n", url);
            printf("IP is %s\n", ip);
            printf("port is %d\n", port);
        }
        else if(strcmp(cmd, "PUT") == 0)
        {
            url = strtok(NULL, ws);
            filename = strtok(NULL, ws);
            if(!url) {printf("Missing URL\n"); continue;}
            if(!filename) {printf("Missing filename\n"); continue;}
            if(strtok(NULL, ws)) printf("Format: PUT <url> <filename>\n");
            // yeah same shit again
            urlcpy = (char*)malloc((strlen(url)+1)*sizeof(char)); strcpy(urlcpy, url);

            if(strstr(urlcpy, "http://")!=urlcpy){printf("Enter a URL beginning with http:// \n"); continue;}
            urlcpy += strlen("http://");
            char* portbegin = rindex((const char*)urlcpy, ':');
            ip = strtok(urlcpy, ipsep); if(!ip){printf("what did you even enter to get this prompt?\n"); continue;}
            // at this point in the code, urlcpy is <IPADDR>/etc/filename.jpg (with port number)
            if(portbegin)
            {
                sscanf(portbegin+1, "%d", &port);
                // ERROR: port entered is not short int
            }
            else port = 80;
            printf("URL is %s\n", url);
            printf("IP is %s\n", ip);
            printf("port is %d\n", port);
            printf("filename is %s\n", filename);
        }
        else printf("Unrecognized command.\n");
    }
    return 0;
}
