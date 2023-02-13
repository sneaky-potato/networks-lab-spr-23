#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>

// #########################################
// ## Ashwani Kumar Kamal (20CS10011)     ##
// ## Networks Laboratory                 ##
// ## Assignment - 4                      ##
// #########################################
// # GCC version: gcc (GCC) 12.1.1 20220730

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
    char *body;
} Request;

// recv and store string function prototype
void recv_str(int, char *, char *, int);
char *getDate(time_t);
struct Request *parse_request(const char *);
void free_header(struct Header *h);
void free_request(struct Request *req);
char *processGetRequest(struct Request *req, int sockfd, int *status);
void processPutRequest(struct Request *req, int sockfd);
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

            // recv http request
            recv_str(newsockfd, local_buf, buf, BUF_SIZE);
            printf("HTTP request:\n%s", local_buf);
            struct Request *req = parse_request(local_buf);
            if (req)
            {
                printf("Method: %d\n", req->method);
                printf("Request-URI: %s\n", req->url);
                printf("HTTP-Version: %s\n", req->version);
                puts("Headers:");
                struct Header *h;
                for (h = req->headers; h; h = h->next)
                    printf("%32s: %s\n", h->name, h->value);
            }
            char *response = NULL;
            int status_code = 0;
            if (req->method == GET)
            {
                response = processGetRequest(req, newsockfd, &status_code);
                send(newsockfd, response, strlen(response), 0);
            }
            // else if (req->method == PUT)
            //     processPutRequest(req, newsockfd);
            // else
            //     processUnsupportedRequest(req, newsockfd);
            // free_request(req);

            // close connection
            close(newsockfd);
            exit(0);
        }

        close(newsockfd);
    }
    return 0;
}

void recv_str(int sockfd, char *local_buf, char *buf, int buf_size)
{
    int t;
    local_buf[0] = '\0';
    while ((t = recv(sockfd, buf, buf_size, 0)) > 0)
    {
        int i;
        for (i = 0; i < t; i++)
        {
            if (buf[i] == '\0')
                break;
        }
        if (i < t)
        {
            strcat(local_buf, buf);
            return;
        }
        buf[buf_size] = '\0';
        strcat(local_buf, buf);
    }
}

char *getDate(time_t t)
{
    char *s = (char *)malloc(100 * sizeof(char));
    struct tm *tm = gmtime(&t);
    strftime(s, 100, "%a, %d %b %Y %H:%M:%S %Z", tm);
    return s;
}

struct Request *parse_request(const char *raw)
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
    req->headers = NULL;
    // raw += 2; // move past <CR><LF>

    // size_t body_len = strlen(raw);
    // req->body = malloc(body_len + 1);
    // memcpy(req->body, raw, body_len);
    // req->body[body_len] = '\0';

    return req;
}

char *processGetRequest(struct Request *request, int sockfd, int *status_code)
{
    char *filename = request->url;
    if (filename[0] == '/')
        filename++;
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        char *response = "HTTP/1.1 404 Not Found\r\n";
        *status_code = 404;
        return response;
    }
    char *expires = getDate(time(NULL) + 3 * 24 * 60 * 60);

    int file_size = 0;
    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    char *response = (char *)malloc((MAX_REQ_SIZE + file_size) * sizeof(char));
    char *file_content = (char *)malloc(file_size * sizeof(char));
    fread(file_content, file_size, 1, fp);
    fclose(fp);

    sprintf(
        response,
        "HTTP/1.1 200 OK\r\nExpires: %s\r\nCache-control: no-store\r\nContent-language: en-us\r\nContent-length: %d\r\nContent-Type: text/html\r\n\r\n%s\r\n",
        expires, file_size, file_content);
    printf("HTTP response: %s\n", response);
    *status_code = 200;
    return response;
}

void free_header(struct Header *h)
{
    free(h->name);
    free(h->value);
    free_header(h->next);
    free(h);
}

void free_request(struct Request *req)
{
    free(req->url);
    free(req->version);
    free_header(req->headers);
    free(req->body);
    free(req);
}