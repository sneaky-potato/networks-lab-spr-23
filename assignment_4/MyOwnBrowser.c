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
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#define MAX_REQ_SIZE 1024
const unsigned BUF_SIZE = 50;
const unsigned LOCAL_BUF_SIZE = 1024;

typedef struct Header
{
    char *name;
    char *value;
    struct Header *next;
} Header;

typedef struct Response
{
    char *version;
    int status;
    char *status_msg;
    struct Header *headers;
} Response;

void showError(char *s)
{
    fprintf(stderr, "ERROR: %s\n", s);
    exit(EXIT_FAILURE);
}

char *getDate(time_t);
void recv_str(int, char *, char *, int, int *, char *);
char *getRequest(char *, int, char *, char *);
char *putRequest(char *, int, char *, char *, char *);
struct Response *parse_response_headers(const char *);
char *getHeader(struct Response *, char *);

int main()
{
    int sockfd, port;
    char buf[BUF_SIZE], *cmd, *url, *putfilename, *putfilenamecpy, *urlcpy, *localpath, *ip, filetype[5];
    char *local_buf = (char *)malloc(sizeof(char) * LOCAL_BUF_SIZE);
    char *ws = " \n\r\a\t", *ipsep = "/:";
    struct sockaddr_in servaddr;
    struct pollfd pfd;

    for (; 1;)
    {
        // create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
            showError("Socket creation failure.");
        struct pollfd pfd;
        pfd.fd = sockfd;
        pfd.events = POLLIN;

        fflush(stdin);
        printf("MyOwnBrowser> ");
        // accept user input
        memset(buf, 0, BUF_SIZE);
        fgets(buf, BUF_SIZE, stdin);
        buf[strlen(buf) - 1] = '\0';
        // parse user input
        cmd = strtok(buf, ws);
        if (!cmd)
        {
            printf("Please enter a non-empty command\n");
            close(sockfd);
            continue;
        }
        // perform appropriate operation
        // remember to poll for max 3 seconds, retry if no response
        if (strcmp(cmd, "QUIT") == 0)
        {
            if (strtok(NULL, ws))
                printf("Command format: QUIT\n");
            printf("Client terminated.\n");
            break;
        }
        else if (strcmp(cmd, "GET") == 0)
        {
            url = strtok(NULL, ws);
            if (!url)
            {
                printf("Missing URL\n");
                close(sockfd);
                continue;
            }
            if (strtok(NULL, ws))
                printf("Command format: GET <url>\n");
            // TODO: date

            // code to parse IP and port
            // NOTE: Don't use urlcpy outside this else-if block
            urlcpy = (char *)malloc((strlen(url) + 1) * sizeof(char));
            strcpy(urlcpy, url);
            localpath = (char *)malloc((strlen(url) + 1) * sizeof(char));
            if (strstr(urlcpy, "http://") != urlcpy)
            {
                printf("Enter a URL that begins with http:// \n");
                close(sockfd);
                continue;
            }
            urlcpy += strlen("http://");
            char *portbegin = strrchr((const char *)urlcpy, ':');
            if (portbegin)
            {
                sscanf(portbegin + 1, "%d", &port);
                // TODO: invalid port handling
                *portbegin = '\0';
            }
            else
                port = 80;
            ip = strtok(urlcpy, ipsep);
            if (!ip)
            {
                printf("Please enter a non-empty IP\n");
                close(sockfd);
                continue;
            }

            urlcpy += strlen(ip) + 1;
            sprintf(localpath, "/%s", urlcpy);
            strtok(urlcpy, ".");
            char *tempfiletype = strtok(NULL, ".");
            if (!tempfiletype)
            {
                printf("Address must contain a filetype and not a directory\n");
                close(sockfd);
                continue;
            }
            memset(filetype, 0, 5);
            strcpy(filetype, tempfiletype);
            memset(&servaddr, 0, sizeof(servaddr));

            servaddr.sin_family = AF_INET;
            inet_aton(ip, &servaddr.sin_addr);
            servaddr.sin_port = htons((short int)port);

            if (connect(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
            {
                printf("Server connection failure.\n");
                continue;
            }
            // prepare request
            char *request = (char *)malloc(MAX_REQ_SIZE * sizeof(char));
            request = getRequest(ip, port, localpath, filetype);
            printf("\nRequest sent to server:\n%s\n", request);
            send(sockfd, request, strlen(request) + 1, 0);
            // poll for response
            int pollret = poll(&pfd, 1, 3000);
            if (pollret == 0)
            {
                printf("No response from server.\n");
                close(sockfd);
                continue;
            }
            else if (pollret == -1)
            {
                printf("Polling error.\n");
                close(sockfd);
                continue;
            }
            char *response = (char *)malloc(MAX_REQ_SIZE * sizeof(char));
            int body_len;
            char *partial_body = (char *)malloc(BUF_SIZE * sizeof(char));
            recv_str(sockfd, local_buf, buf, BUF_SIZE, &body_len, partial_body);
            printf("\nResponse from server:\n%s\n", local_buf);
            struct Response *response_headers = parse_response_headers(local_buf);
            // TODO: status code (400, 403, 404, 4xx, 5xx)
            // printf("%d %s\n", response_headers->status, response_headers->status_msg);
            if (response_headers->status == 200)
            {
            	printf("Status code: 200\nOK\n");
                char *value = getHeader(response_headers, "Content-Length");
                if (!value)
                {
                    printf("No content length\n");
                    close(sockfd);
                    continue;
                }
                int content_len = atoi(value);
                char *content_type = getHeader(response_headers, "Content-Type");
                if (!content_type)
                {
                    printf("No content type\n");
                    close(sockfd);
                    continue;
                }
                
                char* filedir = 1+strrchr(localpath, '/');

                FILE *fp = fopen(filedir, "wb");
                if (!fp)
                {
                    printf("File creation failure.\n");
                    close(sockfd);
                    continue;
                }
                int fd = fileno(fp);
                fwrite(partial_body, sizeof(char), body_len, fp);
                int bytes_left = content_len - body_len;
                while (bytes_left > 0)
                {
                    int bytes_read = recv(sockfd, buf, BUF_SIZE, 0);
                    if (bytes_read == -1)
                    {
                        printf("Error reading from socket.\n");
                        close(sockfd);
                        continue;
                    }
                    fwrite(buf, bytes_read, sizeof(char), fp);
                    bytes_left -= bytes_read;
                }
                fclose(fp);
                // this is where we open the document using filedir
                if(fork() == 0)
                {
                    // exec call
                    // if html
                    int filelen = strlen(filedir);
                    if(strcmp(filedir+filelen-5, ".html") == 0)
	                    execlp("google-chrome", "google-chrome", filedir, NULL);
	                else if(strcmp(filedir+filelen-4, ".pdf") == 0)
	                    execlp("acroread", "acroread", filedir, NULL);
	                else if((strcmp(filedir+filelen-4, ".jpg") == 0) || (strcmp(filedir+filelen-5, ".jpeg")))
	                    execlp("eog", "eog", filedir, NULL);
	                else
		                execlp("gedit", "gedit", filedir, NULL);
					
                    printf("Error in opening file with appropriate application.\n");
                    exit(0);
                }
                wait(NULL);
            }
            else if(response_headers->status == 400)
            	printf("Status code: 400\nBad request\n");
            else if(response_headers->status == 403)
            	printf("Status code: 403\nForbidden resource\n");
            else if(response_headers->status == 404)
            	printf("Status code: 404\nResource not found\n");
            else
            	printf("Status code: %d\nUnknown error\n", response_headers->status);
            close(sockfd);
        }
        else if (strcmp(cmd, "PUT") == 0)
        {
            url = strtok(NULL, ws);
            putfilename = strtok(NULL, ws);
            if (!url)
            {
                printf("Missing URL\n");
                continue;
            }
            if (!putfilename)
            {
                printf("Missing filename\n");
                continue;
            }
            if (strtok(NULL, ws))
                printf("Command format: PUT <url> <filename>\n");
            // repetition of same stuff (sigh i write ugly code)
            urlcpy = (char *)malloc((strlen(url) + 1) * sizeof(char));
            strcpy(urlcpy, url);
            localpath = (char *)malloc((strlen(url) + 1) * sizeof(char));
            if (strstr(urlcpy, "http://") != urlcpy)
            {
                printf("Enter a URL that begins with http:// \n");
                continue;
            }
            urlcpy += strlen("http://");
            char *portbegin = strrchr((const char *)urlcpy, ':');
            if (portbegin)
            {
                sscanf(portbegin + 1, "%d", &port);
                // TODO: invalid port handling
                *portbegin = '\0';
            }
            else
                port = 80;
            ip = strtok(urlcpy, ipsep);
            if (!ip)
            {
                printf("Please enter a non-empty IP\n");
                continue;
            }
            // TODO: invalid IP handling
            // at this point in the code, urlcpy is <IPADDR>/etc/filename.pdf (with port number)
            urlcpy += strlen(ip) + 1;
            // printf("debug print %s\n", urlcpy);
            if (portbegin + 1 == urlcpy)
                sprintf(localpath, "/"); // case handling for http://<ipaddr>:<portno>
            else
                sprintf(localpath, "/%s/", urlcpy);
            putfilenamecpy = (char *)malloc((strlen(putfilename) + 1) * sizeof(char));
            strcpy(putfilenamecpy, putfilename);
            strtok(putfilenamecpy, ".");
            char *tempfiletype = strtok(NULL, ".");
            if (!tempfiletype)
            {
                printf("Filename invalid\n");
                continue;
            }
            memset(filetype, 0, 5);
            strcpy(filetype, tempfiletype);
            // printf("URL is %s\n", url);
            // printf("IP is %s\n", ip);
            // printf("port is %d\n", port);
            // printf("localpath is %s\n", localpath);
            // printf("filename is %s\n", putfilename);
            // printf("filetype is %s\n", filetype);
            // char *finalpath = (char *)malloc((1 + strlen(localpath) + strlen(putfilename)) * sizeof(char));
            // sprintf(finalpath, "%s%s", localpath, putfilename);
            
            // connect to server
            memset(&servaddr, 0, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_addr.s_addr = inet_addr(ip);
            servaddr.sin_port = htons((short int)port);
            if (connect(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
            {
                printf("Server connection failure.\n");
                continue;
            }
            if (putfilename[0] == '/')
                putfilename++;

            char *request = (char *)malloc(MAX_REQ_SIZE * sizeof(char));
            request = putRequest(ip, port, localpath, putfilename, filetype);
            if (!request)
            {
                printf("Error creating request.\n");
                continue;
            }
            printf("\nRequest sent to server:\n%s\n", request);
            send(sockfd, request, strlen(request), 0);

            FILE *fp = fopen(putfilename, "rb");
            int nread;
            while ((nread = fread(buf, sizeof(char), BUF_SIZE, fp)) > 0)
            {
                send(sockfd, buf, nread, 0);
            }
            fclose(fp);
            // poll for response
            int pollret = poll(&pfd, 1, 3000);
            if (pollret == 0)
            {
                printf("No response from server.\n");
                close(sockfd);
                continue;
            }
            else if (pollret == -1)
            {
                printf("Polling error.\n");
                close(sockfd);
                continue;
            }

            // receive response
            // char *response = (char *)malloc(MAX_REQ_SIZE * sizeof(char));
            recv_str(sockfd, local_buf, buf, BUF_SIZE, NULL, NULL);
            printf("Response from server: \n%s\n", local_buf);
            struct Response *response_headers = parse_response_headers(local_buf);
            if (response_headers->status == 200)
                printf("Request successful: %s\n", response_headers->status_msg);
            else if (response_headers->status == 400)
                printf("Bad Request: %s\n", response_headers->status_msg);
            else if (response_headers->status == 403)
                printf("Forbidden Resource: %s\n", response_headers->status_msg);
            else if (response_headers->status == 404)
                printf("Resource Not Found: %s\n", response_headers->status_msg);
            else
                printf("Unknown error: %s\n", response_headers->status_msg);
            close(sockfd);
            continue;
        }

        else
            printf("Unrecognized command.\n");
    }
    return 0;
}

char *getDate(time_t t)
{
    char *s = (char *)malloc(100 * sizeof(char));
    struct tm *tm = gmtime(&t);
    strftime(s, 100, "%a, %d %b %Y %H:%M:%S %Z", tm);
    return s;
}

void recv_str(int sockfd, char *local_buf, char *buf, int buf_size, int *body_len, char *partial_body)
{
    int t;
    int found = 0;
    int cnt = 0;
    char pattern[] = {'\r', '\n', '\r', '\n'};
    char *p;
    int i;
    while ((t = recv(sockfd, buf, BUF_SIZE, 0)) > 0)
    {
        memcpy(local_buf + cnt, buf, t);
        cnt += t;
        for (i = 0; i < cnt - 3; i++)
        {
            if (local_buf[i] == '\r' && local_buf[i + 1] == '\n' && local_buf[i + 2] == '\r' && local_buf[i + 3] == '\n')
            {
                found = 1;
                p = local_buf + i;
                break;
            }
        }
        if (found)
            break;
    }
    // printf("%s\n", local_buf);
    if (found && body_len && partial_body)
    {
        *body_len = cnt - i - 4;
        memcpy(partial_body, p + 4, *body_len);
    }
    *(p + 4) = '\0';
}

char *getRequest(char *ip, int port, char *localpath, char *filetype)
{
    char *request = (char *)malloc(MAX_REQ_SIZE * sizeof(char));
    char *date = getDate(time(NULL));
    char *previous_two_date = getDate(time(NULL) - 2 * 24 * 60 * 60);

    char *accept = (char *)malloc(100 * sizeof(char));
    accept = "text/*";
    if (!strcmp(filetype, "html"))
        accept = "text/html";
    else if (!strcmp(filetype, "pdf"))
        accept = "application/pdf";
    else if (!strcmp(filetype, "jpg"))
        accept = "image/jpeg";

    char *accept_lang = "en-us, en;q=0.9";

    sprintf(
        request,
        "GET %s HTTP/1.1\r\nHost: %s:%d\r\nConnection: close\r\nDate: %s\r\nAccept: %s\r\nAccept-Language: %s\r\nIf-Modified-Since: %s\r\n\r\n",
        localpath,
        ip,
        port,
        date,
        accept,
        accept_lang,
        previous_two_date);

    return request;
}

char *putRequest(char *ip, int port, char *localpath, char* putfilename, char *filetype)
{
    char *request = (char *)malloc(MAX_REQ_SIZE * sizeof(char));
    char *date = getDate(time(NULL));
    char *finalpath = (char *)malloc((1 + strlen(localpath) + strlen(putfilename)) * sizeof(char));
    sprintf(finalpath, "%s%s", localpath, putfilename);
	if(finalpath[0] == '/' && finalpath[1] == '/') finalpath++;
    if (putfilename[0] == '/')
        putfilename++;

    FILE *fp = fopen(putfilename, "rb");
    if (fp == NULL)
    {
        printf("File not found.\n");
        return NULL;
    }

    int file_size = 0;
    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    fclose(fp);
    // printf("debug print filesize %d\n", file_size);
    char *content_type = (char *)malloc(100 * sizeof(char));
    content_type = "text/*";
    if (!strcmp(filetype, "html"))
        content_type = "text/html";
    else if (!strcmp(filetype, "pdf"))
        content_type = "application/pdf";
    else if (!strcmp(filetype, "jpg"))
        content_type = "image/jpeg";

    char *content_lang = "en-us, en;q=0.9";

    sprintf(
        request,
        "PUT %s HTTP/1.1\r\nHost: %s:%d\r\nConnection: close\r\nDate: %s\r\nContent-Type: %s\r\nContent-Language: %s\r\nContent-Length: %d\r\n\r\n",
        finalpath,
        ip,
        port,
        date,
        content_type,
        content_lang,
        file_size);
    // printf("debug print request %s\n", request);
    return request;
}

struct Response *parse_response_headers(const char *raw)
{
    Response *res = (Response *)malloc(sizeof(Response));
    memset(res, 0, sizeof(struct Response));

    // HTTP-Version
    size_t ver_len = strcspn(raw, " ");
    res->version = (char *)malloc((ver_len + 1) * sizeof(char));
    memcpy(res->version, raw, ver_len);
    res->version[ver_len] = '\0';
    raw += ver_len + 1; // move past <SP>

    // Status code
    size_t status_len = strcspn(raw, " ");
    char *status_str = (char *)malloc((status_len + 1) * sizeof(char));
    memcpy(status_str, raw, status_len);
    res->status = atoi(status_str);
    raw += status_len + 1; // move past <SP>

    // Ststus message
    size_t msg_len = strcspn(raw, "\r\n");
    res->status_msg = (char *)malloc((msg_len + 1) * sizeof(char));
    memcpy(res->status_msg, raw, msg_len);
    res->status_msg[msg_len] = '\0';
    raw += msg_len + 2; // move past <CR><LF>

    struct Header *header = NULL, *last = NULL;

    while (!((raw[0] == '\r' && raw[1] == '\n') || (raw[0] == '\0')))
    {
        last = header;
        header = malloc(sizeof(Header));

        // name
        size_t name_len = strcspn(raw, ":");
        header->name = malloc(name_len + 1);
        memcpy(header->name, raw, name_len);
        header->name[name_len] = '\0';
        raw += name_len + 1; // move past :

        while (*raw == ' ')
            raw++;

        // value
        size_t value_len = strcspn(raw, "\r\n");
        header->value = malloc(value_len + 1);
        memcpy(header->value, raw, value_len);
        header->value[value_len] = '\0';
        raw += value_len + 2; // move past <CR><LF>

        // next
        header->next = last;
    }
    res->headers = header;

    return res;
}

char *getHeader(struct Response *response, char *name)
{
    struct Header *header = response->headers;
    while (header != NULL)
    {
        if (!strcmp(header->name, name))
            return header->value;
        header = header->next;
    }
    return NULL;
}
