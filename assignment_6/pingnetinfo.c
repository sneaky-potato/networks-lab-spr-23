/*
    CS39006 - Networks Laboratory, Spring Semester 2022-2023
    Assignment No: 6
    Group No: 16
    Members: Ashwani Kumar Kamal (20CS10011), Kartik Pontula (20CS10031)
    Program Synopsis: Estimation of route, latency and bandwidth of an IP address
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <signal.h>

#define PACKET_SIZE 64
#define MAX_NO_PACKETS 5
#define MAX_NO_HOPS 30
#define RECV_TIMEOUT 1

int program_exit;

// idea behind this handler:
// send ICMP packets in a loop until program_exit is set to 1
// CTRL-C to unset program_exit and print statitics of ping packets
void exitHandler(int signum)
{
    program_exit = 1;
}

uint16_t csum(uint16_t *addr, int len)
{
    int nleft = len;
    uint32_t sum = 0;
    uint16_t *buf = addr;
    uint16_t res = 0;

    while (nleft > 1)
    {
        sum += *buf++;
        nleft -= 2;
    }
    if (nleft == 1)
    {
        *(unsigned char *)(&res) = *(unsigned char *)buf;
        sum += res;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    res = ~sum;
    return res;
}

int main(int argc, char **argv)
{
    // takes DNS name or IP
    // 1 <= n <= INT_MAX
    // 1 <= T <= LONG_MAX
    // signal(SIGINT, exitHandler);

    char *hostname_or_ip;
    void *ip;
    char str[INET_ADDRSTRLEN];
    struct hostent *hptr;
    int n;
    unsigned long T;
    // Input validation
    if (argc < 4)
    {
        printf("Usage: ./a.out <hostname/IP> <n> <T>");
        return EXIT_FAILURE;
    }

    hostname_or_ip = argv[1];
    n = atoi(argv[2]);
    if (n <= 0)
    {
        printf("n must be a positive integer");
        return EXIT_FAILURE;
    }
    T = strtoul((const char *)argv[3], NULL, 10);
    if (T <= 0 || !T || T == ULONG_MAX)
    {
        printf("T must be a positive integer");
        return EXIT_FAILURE;
    }
    // Obtaining IP from hostname (note: handle IP input separately)
    hptr = gethostbyname(hostname_or_ip);
    if (!hptr)
    {
        printf("Error in gethostbyname: %s\n", hstrerror(h_errno));
        return EXIT_FAILURE;
    }
    if (hptr->h_addrtype != AF_INET)
    {
        printf("Error: Only AF_INET supported");
        return EXIT_FAILURE;
    }

    printf("IP Address(es):\n");
    for (char **ptr = hptr->h_addr_list; *(ptr) != NULL; ptr++)
    {
        if (inet_ntop(hptr->h_addrtype, *ptr, str, sizeof(str)) == NULL)
        {
            printf("Error in h_addr_list: %s\n", hstrerror(h_errno));
            return EXIT_FAILURE;
        }
        printf("%s\n", str);
    }
    printf("n = %d, T = %lu\n", n, T);

    // first IP address is used
    ip = hptr->h_addr_list[0];

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd == -1)
    {
        perror("ERROR: Socket creation failure");
        exit(EXIT_FAILURE);
    }

    struct icmphdr *icmp_header;
    struct icmphdr *icmp_reply;
    struct timeval start, end, diff;

    int seq_num = 0;
    struct sockaddr_in server_addr;
    unsigned int addr_len = sizeof(server_addr);
    double rtt;

    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1025);
    inet_ntop(hptr->h_addrtype, hptr->h_addr_list[0], str, sizeof(str));

    if (inet_aton(str, &server_addr.sin_addr) < 0)
    {
        printf("inet_aton");
        return -1;
    }

    int status;
    int err;

    struct timeval recv_timeout;
    recv_timeout.tv_sec = RECV_TIMEOUT;
    recv_timeout.tv_usec = 0;
    int ttl;

    unsigned char *packet = (unsigned char *)malloc(PACKET_SIZE * sizeof(unsigned char));

    for (ttl = 1; ttl < MAX_NO_HOPS; ttl++)
    {
        if ((status = setsockopt(sockfd, SOL_IP, IP_TTL, &ttl, sizeof(ttl))) < 0)
        {
            printf("setsockopt ttl");
            exit(EXIT_FAILURE);
        }

        if ((err = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&recv_timeout, sizeof(recv_timeout))) < 0)
        {
            printf("setsockopt recv timeout");
            exit(EXIT_FAILURE);
        }

        double min_rrt = INT_MAX;

        for (int seq = 0; seq < MAX_NO_PACKETS; seq++)
        {
            memset(packet, 0, PACKET_SIZE);
            icmp_header = (struct icmphdr *)packet;

            icmp_header->type = ICMP_ECHO;
            icmp_header->code = 0;
            icmp_header->un.echo.id = getpid();
            icmp_header->un.echo.sequence = htons(seq);
            icmp_header->checksum = 0;
            icmp_header->checksum = csum((uint16_t *)icmp_header, PACKET_SIZE);

            gettimeofday(&start, NULL);
            if ((status = sendto(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr *)&server_addr, addr_len)) < 0)
            {
                printf("sendto");
                return -1;
            }
            unsigned char *reply = (unsigned char *)malloc(PACKET_SIZE * sizeof(unsigned char));

            if ((status = recvfrom(sockfd, reply, PACKET_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len)) < 0)
            {
                printf("recvfrom");
                return -1;
            }

            gettimeofday(&end, NULL);

            struct iphdr *ip_header = (struct iphdr *)reply;
            int ip_header_len = ip_header->ihl * 4;

            icmp_reply = (struct icmphdr *)(reply + ip_header_len);

            timersub(&end, &start, &diff);
            rtt = (double)diff.tv_sec * 1000.0 + (double)diff.tv_usec / 1000.0;

            if (rtt < min_rrt)
                min_rrt = rtt;

            if (icmp_reply->type == ICMP_TIME_EXCEEDED)
            {
                struct in_addr ip_addr;
                ip_addr.s_addr = ip_header->saddr;
                printf("%d: %s\n", seq, inet_ntoa(server_addr.sin_addr));
            }
            else if (icmp_reply->type == ICMP_ECHOREPLY)
            {
                printf("%d: %s\n", seq, inet_ntoa(server_addr.sin_addr));
                break;
            }
            else
            {
                printf("* * * *\n");
            }
            sleep(1);
        }
        printf("rrt: %f\n\n", min_rrt);
    }

    close(sockfd);

    // print statistics

    return 0;
}

void input_validator()
{
    return;
}