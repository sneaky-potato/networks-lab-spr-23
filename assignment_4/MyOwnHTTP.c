/*
    CS39006 - Networks Laboratory, Spring Semester 2022-2023
    Assignment No: 4
    Group No: 16
    Members: Ashwani Kumar Kamal (20CS10011), Kartik Pontula (20CS10031)
    Program Synopsis: Concurrent HTTP server program to parse GET and PUT requests and respond to clients.
    Usage:
        gcc MyOwnHTTP.c -o serv && ./serv
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>

#define MAX_REQ_SIZE 1024

const unsigned PORT = 20000;
const unsigned BUF_SIZE = 50;
const unsigned LOCAL_BUF_SIZE = 1024;

typedef enum Method
{
    GET,
    PUT,
    UNSUPPORTED
} Method;

typedef struct Header
{
    char *name;
    char *value;
    struct Header *next;
} Header;

typedef struct Request
{
    enum Method method;
    char *url;
    char *version;
    struct Header *headers;
} Request;

// recv and store string function prototype
void recv_str(int, char *, char *, int, int *, char *);
char *getDate(time_t);
struct Request *parse_request_headers(const char *);
void free_header(struct Header *);
void free_request(struct Request *);
char *getHeader(struct Request *, char *);
char *processGetRequest(struct Request *req, int sockfd, int *status);
char *processPutRequest(struct Request *req, int sockfd);
void processUnsupportedRequest(struct Request *req, int sockfd);

int main()
{
    int sockfd, newsockfd;
    int clilen;
    struct sockaddr_in cli_addr, serv_addr;

    // 1 byte extra for null string
    char *buf = (char *)malloc(sizeof(char) * (BUF_SIZE + 1));

    // local buf for recv and storing successively
    char *local_buf = (char *)malloc(sizeof(char) * LOCAL_BUF_SIZE);

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
    memset(&cli_addr, 0, sizeof(cli_addr));
    while (1)
    {
        // Accept connection from client
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

            // recv http request
            int body_len = 0;
            char *partial_body = (char *)malloc(sizeof(char) * BUF_SIZE);
            recv_str(newsockfd, local_buf, buf, BUF_SIZE, &body_len, partial_body);
            time_t recvtime = time(NULL);
            struct tm* recvtm = localtime(&recvtime); // recording the time of receiving response
            printf("\nRequest received from client:\n%s\n", local_buf);
            struct Request *req = parse_request_headers(local_buf);
            char *response = NULL;
            int status_code = 0;
            // TODO: append to AccessLog.txt
            // format:- ddmmyy:hhmmss:clientip:clientport:GET/PUT:URL
            char* cli_ip = inet_ntoa(cli_addr.sin_addr);
            int cli_port = (int)ntohs(cli_addr.sin_port);
            FILE* flog = fopen("AccessLog.txt", "a");
            fprintf(flog, "%02d%02d%02d:%02d%02d%02d:%s:%d:", 
                    recvtm->tm_mday, recvtm->tm_mon+1, recvtm->tm_year-100,
                    recvtm->tm_hour, recvtm->tm_min, recvtm->tm_sec,
                    cli_ip, cli_port
                    );
            if (req->method == GET)
            {
                response = processGetRequest(req, newsockfd, &status_code);
                printf("\nResponse sent to client: \n%s\n", response);
                send(newsockfd, response, strlen(response), 0);

                char *filename = req->url;
                if (filename[0] == '/')
                    filename++;
                // complete log entry for GET
                fprintf(flog, "GET:/%s\n", filename);
                fclose(flog);

                FILE *fp = fopen(filename, "rb");
                int nread;
                while ((nread = fread(buf, sizeof(char), BUF_SIZE, fp)) > 0)
                {
                    send(newsockfd, buf, nread, 0);
                }
                fclose(fp);
            }
            else if (req->method == PUT)
            {
                char* filedir = req->url;
                if(filedir[0]=='/') filedir++;
                // long entry completion for PUT
                fprintf(flog, "PUT:/%s\n", filedir);
                fclose(flog);

                char *value = getHeader(req, "Content-Length");
                if (!value)
                {
                    printf("No content length\n");
                    close(newsockfd);
                    continue;
                }
                int content_len = atoi(value);
                char *content_type = getHeader(req, "Content-Type");
                if (!content_type)
                {
                    printf("No content type\n");
                    close(newsockfd);
                    continue;
                }

                FILE *fp = fopen(filedir, "wb");
                if (!fp)
                {
                    printf("File creation failure.\n");
                    close(newsockfd);
                    continue;
                }
                fwrite(partial_body, sizeof(char), body_len, fp);
                int bytes_left = content_len - body_len;
                while (bytes_left > 0)
                {
                    int bytes_read = recv(newsockfd, buf, BUF_SIZE, 0);
                    if (bytes_read == -1)
                    {
                        printf("Error reading from socket.\n");
                        close(newsockfd);
                        continue;
                    }
                    fwrite(buf, bytes_read, sizeof(char), fp);
                    bytes_left -= bytes_read;
                }
                fclose(fp);
                response = processPutRequest(req, newsockfd);
                send(newsockfd, response, strlen(response) + 1, 0);
                printf("\nResponse sent to client:\n%s\n", response);
            }
            free_request(req);
            close(newsockfd);
            exit(0);
        }
        close(newsockfd);
    }
    return 0;
}

// recv until \r\n\r\n
void recv_str(int sockfd, char *local_buf, char *buf, int buf_size, int *body_len, char *partial_body)
{
    // recv until one of recved packets contains \r\n\r\n
    // store everything in Request struct
    // printf("recv starts\n");
    int t;
    int found = 0;
    int cnt = 0;
    char pattern[] = {'\r', '\n', '\r', '\n'};
    char *p;
    int i;
    while ((t = recv(sockfd, buf, BUF_SIZE, 0)) > 0)
    {
        memcpy(local_buf + cnt, buf, t);
        // printf(">%s\n", local_buf);
        cnt += t;
        // printf("cnt: %d\n", cnt);
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
    // printf("recv ends\n");
    if (found)
    {
        *body_len = cnt - i - 4;
        memcpy(partial_body, p + 4, *body_len);
    }
    *(p + 4) = '\0';
}

char *getDate(time_t t)
{
    char *s = (char *)malloc(100 * sizeof(char));
    struct tm *tm = gmtime(&t);
    strftime(s, 100, "%a, %d %b %Y %H:%M:%S %Z", tm);
    return s;
}

struct Request *parse_request_headers(const char *raw)
{
    struct Request *req = (struct Request *)malloc(sizeof(struct Request));
    memset(req, 0, sizeof(struct Request));

    // Method
    size_t meth_len = strcspn(raw, " ");
    if (memcmp(raw, "GET", strlen("GET")) == 0)
        req->method = GET;
    else if (memcmp(raw, "PUT", strlen("PUT")) == 0)
        req->method = PUT;
    else
        req->method = UNSUPPORTED;
    raw += meth_len + 1; // move past <SP>

    // Request-URI
    size_t url_len = strcspn(raw, " ");
    req->url = (char *)malloc((url_len + 1) * sizeof(char));
    memcpy(req->url, raw, url_len);
    req->url[url_len] = '\0';
    raw += url_len + 1; // move past <SP>

    // HTTP-Version
    size_t ver_len = strcspn(raw, "\r\n");
    req->version = (char *)malloc((ver_len + 1) * sizeof(char));
    memcpy(req->version, raw, ver_len);
    req->version[ver_len] = '\0';
    raw += ver_len + 2; // move past <CR><LF>

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
    req->headers = header;

    return req;
}

char *processGetRequest(struct Request *request, int sockfd, int *status_code)
{
    char *filename = request->url;
    if (filename[0] == '/')
        filename++;
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        char *response = "HTTP/1.1 404 Not Found\r\n\r\n";
        *status_code = 404;
        return response;
    }
    char *expires = getDate(time(NULL) + 3 * 24 * 60 * 60);

    int file_size = 0;
    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    fclose(fp);

    char *response = (char *)malloc((MAX_REQ_SIZE) * sizeof(char));
    char *content_type = getHeader(request, "Accept");

    if (!content_type)
        content_type = "text/html";

    sprintf(
        response,
        "HTTP/1.1 200 OK\r\nExpires: %s\r\nCache-Control: no-store\r\nContent-Language: en-us\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n",
        expires, file_size, content_type);
    // printf("HTTP response: %s\n", response);
    *status_code = 200;
    return response;
}

char *processPutRequest(struct Request *request, int sockfd)
{
    char *filename = request->url;
    if (filename[0] == '/')
        filename++;
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        char *response = "HTTP/1.1 404 Not Found\r\n";
        return response;
    }
    char *expires = getDate(time(NULL) + 3 * 24 * 60 * 60);

    int file_size = 0;
    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    fclose(fp);

    char *response = (char *)malloc((MAX_REQ_SIZE) * sizeof(char));
    char *content_type = getHeader(request, "Accept");

    if (!content_type)
        content_type = "text/html";

    sprintf(
        response,
        "HTTP/1.1 200 OK\r\nExpires: %s\r\nCache-Control: no-store\r\nContent-Language: en-us\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n",
        expires, file_size, content_type);
    // *status_code = 200;
    return response;
}

char *getHeader(struct Request *request, char *name)
{
    struct Header *header = request->headers;
    while (header != NULL)
    {
        if (!strcmp(header->name, name))
            return header->value;
        header = header->next;
    }
    return NULL;
}

void free_request(struct Request *req)
{
    free(req->url);
    free(req->version);
    free(req->headers->name);
    free(req->headers->value);
    free(req->headers->next);
    free(req->headers);
    free(req);
}
