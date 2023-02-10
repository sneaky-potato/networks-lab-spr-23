/* 
    CS39006 - Networks Laboratory, Spring Semester 2022-2023
    Assignment 4
    Members: Ashwani Kumar Kamal (20CS10011), Kartik Pontula (20CS10031)
    Program Synopsis: Client-side socket program for HTTP requests
    Usage:
        gcc client.c -o cliexec
        ./cliexec
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

#define SERVPORT 8000
#define BUFSIZE 256
void showError(char* s)
{
    fprintf(stderr, "ERROR: %s\n", s);
    exit(EXIT_FAILURE);
}

int main()
{
    int sockfd;
    char buf[BUFSIZE], *cmd, *url, *filename;
    // char ws[] = {'\n', '\r', '\a', '\t', ' ', '\0'};
    char *ws = " \n\r\a\t";
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
        cmd = strtok(buf, ws); if(!cmd){printf("oi you almost cost me a segfault, watch it\n"); continue;}
        if(strcmp(cmd, "QUIT") == 0)
        {
            if(strtok(NULL, ws)) printf("writing just QUIT is enough\n");
            printf("Client terminated.\n"); break;
        }
        else if(strcmp(cmd, "GET") == 0)
        {
            printf("get.\n");
            url = strtok(NULL, ws); if(!url) {printf("url wher\n"); continue;}
            printf("URL is %s\n", url);
            if(strtok(NULL, ws)) printf("tmi dude, GET <url> is enough\n");
        }
        else if(strcmp(cmd, "PUT") == 0)
        {
            printf("put.\n");
            url = strtok(NULL, ws);
            filename = strtok(NULL, ws);
            if(!url) {printf("url wher\n"); continue;}
            if(!filename) {printf("filename wher\n"); continue;}
            printf("URL is %s\n", url);
            printf("filename is %s\n", filename);
            if(strtok(NULL, ws)) printf("tmi dude, PUT <url> <filename> is enough\n");
        }
        else printf("get good at following protocol you doofus\n");
    }
    return 0;
}
