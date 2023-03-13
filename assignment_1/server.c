#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

// #########################################
// ## Ashwani Kumar Kamal (20CS10011)     ##
// ## Networks Laboratory                 ##
// ## Assignment - 1                      ##
// #########################################
// # GCC version: gcc (GCC) 12.1.1 20220730

int PORT = 20000;
int BUF_SIZE = 6;

void remove_spaces(char *);
char *evaluate(char *, int);

int main()
{
    int sockfd, newsockfd;
    int clilen;
    struct sockaddr_in cli_addr, serv_addr;

    int i;
    char buf[100];
    char *expression = (char *)malloc(sizeof(char) * 100);

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Cannot create socket\n");
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
            perror("Accept error\n");
            exit(0);
        }

        printf("Found a client::\n\n");
        while (1)
        {
            char *result;

            while (1)
            {
                // Recieve buffer packet
                int t = recv(newsockfd, buf, BUF_SIZE, 0);
                if (t <= 0)
                    break;

                // Check for newline in a packet
                int newline_found = 0;
                for (int i = 0; i < 5; i++)
                {
                    if (buf[i] == '\n')
                        newline_found = 1;
                }

                printf("recieved string: %s\n", buf);
                // Remove spaces in buffer packet
                remove_spaces(buf);
                // Concatenate buffer packet in expression
                strcat(expression, buf);

                // If a newline was found, proceed to evaluate and send result
                if (newline_found)
                {
                    printf("complete expression: %s\n", expression);
                    result = evaluate(expression, strlen(expression));
                    printf("result = %s\n", result);
                    break;
                }
            }
            // Send result
            send(newsockfd, result, 100, 0);
            printf("Sent reponse\n");
            for (int i = 0; i < 100; i++)
                expression[i] = '\0';
        }
        close(newsockfd);
    }

    return 0;
}

// Remove any spaces, tabs or newlines
void remove_spaces(char *s)
{
    char *d = s;
    do
    {
        while (*d == ' ' || *d == '\t' || *d == '\n')
        {
            ++d;
        }
    } while (*s++ = *d++);
}

char *evaluate(char *buf, int n)
{
    char *result_string = (char *)malloc(sizeof(char) * 100);
    double res;

    // If expression starts with bracket, evaluate the bracket entirely
    if (*buf == '(')
    {
        buf++;

        if (!isdigit(*buf))
        {
            result_string = "INVALID_STRING";
            return result_string;
        }

        double res_brack = strtod(buf, &buf);
        // While closing bracket is not found
        while (*buf != ')')
        {
            char op = *buf;
            buf++;
            if (!isdigit(*buf))
            {
                result_string = "INVALID_STRING";
                return result_string;
            }

            double next_brack = strtod(buf, &buf);
            switch (op)
            {
            case '+':
                res_brack += next_brack;
                break;
            case '-':
                res_brack -= next_brack;
                break;
            case '*':
                res_brack *= next_brack;
                break;
            case '/':
                res_brack /= next_brack;
                break;
            default:
                result_string = "INVALID_STRING";
                break;
            }
        }

        buf++;
        res = res_brack;
    }
    else
    {
        // no bracket at first, get the initial number
        if (!isdigit(*buf))
        {
            result_string = "INVALID_STRING";
            return result_string;
        }
        res = strtod(buf, &buf);
    }

    // While end of string is reached
    while (*buf != '\0')
    {
        if (*buf == '+' || *buf == '-' || *buf == '*' || *buf == '/')
        {
            // Operator found, evaluate next number
            char op = *buf;
            buf++;
            double next;

            if (*buf == '(')
            {
                // Bracket evaluation
                buf++;

                if (!isdigit(*buf))
                {
                    result_string = "INVALID_STRING";
                    return result_string;
                }

                double res_brack = strtod(buf, &buf);

                while (*buf != ')')
                {
                    char op = *buf;
                    buf++;

                    if (!isdigit(*buf))
                    {
                        result_string = "INVALID_STRING";
                        return result_string;
                    }

                    double next_brack = strtod(buf, &buf);
                    switch (op)
                    {
                    case '+':
                        res_brack += next_brack;
                        break;
                    case '-':
                        res_brack -= next_brack;
                        break;
                    case '*':
                        res_brack *= next_brack;
                        break;
                    case '/':
                        res_brack /= next_brack;
                        break;
                    default:
                        result_string = "INVALID_STRING";
                        break;
                    }
                }
                buf++;

                next = res_brack;
            }
            else
            {
                // Current bracket is not bracket
                if (!isdigit(*buf))
                {
                    result_string = "INVALID_STRING";
                    return result_string;
                }
                next = strtod(buf, &buf);
            }

            switch (op)
            {
            case '+':
                res += next;
                break;
            case '-':
                res -= next;
                break;
            case '*':
                res *= next;
                break;
            case '/':
                res /= next;
                break;
            default:
                result_string = "INVALID_STRING";
                break;
            }
        }
        else
        {
            result_string = "INVALID_STRING";
            return result_string;
        }
    }
    // Convert result to string
    sprintf(result_string, "%lf", res);
    return result_string;
}