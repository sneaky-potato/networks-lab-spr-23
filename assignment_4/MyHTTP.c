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
#include <dirent.h>

// #########################################
// ## Ashwani Kumar Kamal (20CS10011)     ##
// ## Networks Laboratory                 ##
// ## Assignment - 4                      ##
// #########################################
// # GCC version: gcc (GCC) 12.1.1 20220730

const unsigned PORT = 20001;
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

// send results in batches function prototype
void send_results(int, char *, char *, int);
// recv and store string function prototype
void recv_str(int, char *, char *, int);

struct Request *parse_request(const char *raw);
void free_header(struct Header *h);
void free_request(struct Request *req);

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
    while (raw[0] != '\r' || raw[1] != '\n')
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
        {
            raw++;
        }

        // value
        size_t value_len = strcspn(raw, "\r\n");
        header->value = malloc(value_len + 1);
        if (!header->value)
        {
            free_request(req);
            return NULL;
        }
        memcpy(header->value, raw, value_len);
        header->value[value_len] = '\0';
        raw += value_len + 2; // move past <CR><LF>

        // next
        header->next = last;
    }
    req->headers = header;
    raw += 2; // move past <CR><LF>

    size_t body_len = strlen(raw);
    req->body = malloc(body_len + 1);
    if (!req->body)
    {
        free_request(req);
        return NULL;
    }
    memcpy(req->body, raw, body_len);
    req->body[body_len] = '\0';

    return req;
}

void free_header(struct Header *h)
{
    if (h)
    {
        free(h->name);
        free(h->value);
        free_header(h->next);
        free(h);
    }
}

void free_request(struct Request *req)
{
    free(req->url);
    free(req->version);
    free_header(req->headers);
    free(req->body);
    free(req);
}