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
    char *body;
} Response;

void showError(char *s)
{
    fprintf(stderr, "ERROR: %s\n", s);
    exit(EXIT_FAILURE);
}

char *getDate(time_t);
void recv_str(int, char *, char *, int, int *, char *);
char *getRequest(char *, int, char *, char *);
char *putRequest(char *, int, char *, char *, int);
struct Response *parse_response(const char *);

int main()
{
    int sockfd, port;
    char buf[BUF_SIZE], *cmd, *url, *putfilename, *putfilenamecpy, *urlcpy, *localpath, *ip, filetype[5];
    char *local_buf = (char *)malloc(sizeof(char) * LOCAL_BUF_SIZE);
    char *ws = " \n\r\a\t", *ipsep = "/:";
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        showError("Socket creation failure.");
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;

    for (; 1;)
    {
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
            sprintf(localpath, "/%s", urlcpy);
            strtok(urlcpy, ".");
            char *tempfiletype = strtok(NULL, ".");
            if (!tempfiletype)
            {
                printf("Address must contain a filetype and not a directory\n");
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
            send(sockfd, request, strlen(request) + 1, 0);
            // poll for response
            // int pollret = poll(&pfd, 1, 3000);
            // if (pollret == 0)
            // {
            //     printf("No response from server.\n");
            //     close(sockfd);
            //     continue;
            // }
            // else if (pollret == -1)
            // {
            //     printf("Polling error.\n");
            //     continue;
            // }
            // else
            // {
            // receive response
            char *response = (char *)malloc(MAX_REQ_SIZE * sizeof(char));
            int body_len;
            char *partial_body = (char *)malloc(BUF_SIZE * sizeof(char));
            recv_str(sockfd, local_buf, buf, BUF_SIZE, &body_len, partial_body);
            printf("local_buf: %s\n", local_buf);
            printf("partial_body: %s\n", partial_body);
            close(sockfd);
            // }
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
                sprintf(localpath, "/%s", urlcpy);
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
            if (connect(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
            {
                printf("Server connection failure.\n");
                continue;
            }
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
    if (found)
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
        "GET %s HTTP/1.1\r\nHost: %s:%d\r\nConnection: close\r\nDate: % s\r\nAccept: %s\r\nAccept-Language: %s\r\nIf-Modified-Since: %s\r\n\r\n",
        localpath,
        ip,
        port,
        date,
        accept,
        accept_lang,
        previous_two_date);

    return request;
}

char *putRequest(char *ip, int port, char *localpath, char *filetype, int content_length)
{
    char *request = (char *)malloc(MAX_REQ_SIZE * sizeof(char));
    char *date = getDate(time(NULL));
    char *previous_two_date = getDate(time(NULL) - 2 * 24 * 60 * 60);

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
        "PUT %s HTTP/1.1\nHost: %s:%d\nConnection: close;\nDate: % s\nContent-type: %s\nContent-language: %s\n",
        localpath,
        ip,
        port,
        date,
        content_type,
        content_lang);

    return request;
}

struct Response *parse_response(const char *raw)
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

    raw += 2; // move past <CR><LF>

    size_t body_len = strlen(raw);
    res->body = malloc(body_len + 1);
    memcpy(res->body, raw, body_len);
    res->body[body_len] = '\0';

    return res;
}
