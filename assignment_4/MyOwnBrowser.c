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
    char buf[BUFSIZE], *cmd, *url, *putfilename, *putfilenamecpy, *urlcpy, *localpath, *ip, filetype[5];
    char *ws = " \n\r\a\t", *ipsep = "/:";
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) showError("Socket creation failure.");


    for(;1;)
    {
        fflush(stdin);
        printf("MyOwnBrowser> ");
        // accept user input
        memset(buf, 0, BUFSIZE);
        fgets(buf, BUFSIZE, stdin);
        buf[strlen(buf) - 1] = '\0';
        // parse user input
        cmd = strtok(buf, ws); if(!cmd){printf("Please enter a non-empty command\n"); continue;}
        // perform appropriate operation
        // remember to poll for max 3 seconds, retry if no response
        if(strcmp(cmd, "QUIT") == 0)
        {
            if(strtok(NULL, ws)) printf("Command format: QUIT\n");
            printf("Client terminated.\n"); break;
        }
        else if(strcmp(cmd, "GET") == 0)
        {
            url = strtok(NULL, ws); if(!url) {printf("Missing URL\n"); continue;}
            if(strtok(NULL, ws)) printf("Command format: GET <url>\n");
            // TODO: date

            // code to parse IP and port
            // NOTE: Don't use urlcpy outside this else-if block
            urlcpy = (char*)malloc((strlen(url)+1)*sizeof(char)); strcpy(urlcpy, url);
            localpath = (char*)malloc((strlen(url)+1)*sizeof(char));
            if(strstr(urlcpy, "http://")!=urlcpy){printf("Enter a URL that begins with http:// \n"); continue;}
            urlcpy += strlen("http://");
            char* portbegin = strrchr((const char*)urlcpy, ':');
            if(portbegin)
            {
                sscanf(portbegin+1, "%d", &port);
                // TODO: invalid port handling
                *portbegin = '\0';
            }
            else port = 80;
            ip = strtok(urlcpy, ipsep); if(!ip){printf("Please enter a non-empty IP\n"); continue;}
            // TODO: invalid IP handling
            // at this point in the code, urlcpy is <IPADDR>/etc/filename.pdf (with port number)
            urlcpy += strlen(ip)+1;
            sprintf(localpath, "/%s", urlcpy);
            strtok(urlcpy, ".");
            char* tempfiletype = strtok(NULL, ".");
            if(!tempfiletype){printf("Address must contain a filetype and not a directory\n"); continue;}
            memset(filetype, 0, 5); strcpy(filetype, tempfiletype);
            // printf("URL is %s\n", url);
            printf("IP is %s\n", ip);
            printf("port is %d\n", port);
            printf("localpath is %s\n", localpath);
            printf("filetype is %s\n", filetype);
            // connect to server
            memset(&servaddr, 0, sizeof(servaddr));
            servaddr.sin_family = htons(AF_INET);
            servaddr.sin_addr.s_addr = inet_addr(ip);
            servaddr.sin_port = htons((short int)port);
            if(connect(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){printf("Server connection failure.\n"); continue;}

        }
        else if(strcmp(cmd, "PUT") == 0)
        {
            url = strtok(NULL, ws);
            putfilename = strtok(NULL, ws);
            if(!url) {printf("Missing URL\n"); continue;}
            if(!putfilename) {printf("Missing filename\n"); continue;}
            if(strtok(NULL, ws)) printf("Command format: PUT <url> <filename>\n");
            // repetition of same stuff (sigh i write ugly code)
            urlcpy = (char*)malloc((strlen(url)+1)*sizeof(char)); strcpy(urlcpy, url);
            localpath = (char*)malloc((strlen(url)+1)*sizeof(char));
            if(strstr(urlcpy, "http://")!=urlcpy){printf("Enter a URL that begins with http:// \n"); continue;}
            urlcpy += strlen("http://");
            char* portbegin = strrchr((const char*)urlcpy, ':');
            if(portbegin)
            {
                sscanf(portbegin+1, "%d", &port);
                // TODO: invalid port handling
                *portbegin = '\0';
            }
            else port = 80;
            ip = strtok(urlcpy, ipsep); if(!ip){printf("Please enter a non-empty IP\n"); continue;}
            // TODO: invalid IP handling
            // at this point in the code, urlcpy is <IPADDR>/etc/filename.pdf (with port number)
            urlcpy += strlen(ip)+1;
            // printf("debug print %s\n", urlcpy);
            if(portbegin+1 == urlcpy) sprintf(localpath, "/"); // case handling for http://<ipaddr>:<portno>
            else sprintf(localpath, "/%s", urlcpy);
            putfilenamecpy = (char*)malloc((strlen(putfilename)+1)*sizeof(char)); strcpy(putfilenamecpy, putfilename);
            strtok(putfilenamecpy, ".");
            char* tempfiletype = strtok(NULL, ".");
            if(!tempfiletype){printf("Filename invalid\n"); continue;}
            memset(filetype, 0, 5); strcpy(filetype, tempfiletype);
            // printf("URL is %s\n", url);
            printf("IP is %s\n", ip);
            printf("port is %d\n", port);
            printf("localpath is %s\n", localpath);
            printf("filename is %s\n", putfilename);
            printf("filetype is %s\n", filetype);
            // connect to server
            memset(&servaddr, 0, sizeof(servaddr));
            servaddr.sin_family = htons(AF_INET);
            servaddr.sin_addr.s_addr = inet_addr(ip);
            servaddr.sin_port = htons((short int)port);
            if(connect(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){printf("Server connection failure.\n"); continue;}
        }


        else printf("Unrecognized command.\n");
    }
    return 0;
}
