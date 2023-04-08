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
    unsigned char *packet = (unsigned char *)malloc(PACKET_SIZE * sizeof(unsigned char));
    icmp_header = (struct icmphdr *)packet;

    icmp_header->type = ICMP_ECHO;
    icmp_header->code = 0;
    icmp_header->un.echo.id = getpid();
    icmp_header->un.echo.sequence = htons(3);
    icmp_header->checksum = 0;
    icmp_header->checksum = csum((uint16_t *)icmp_header, PACKET_SIZE);

    printf("icmp_header->type = %d\n", icmp_header->type);
    printf("icmp_header->code = %d\n", icmp_header->code);
    printf("icmp_header->un.echo.sequence = %d\n", icmp_header->un.echo.sequence);
    printf("icmp_header->checksum = %d\n", icmp_header->checksum);

    gettimeofday(&start, NULL);

    int status;
    if ((status = sendto(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr *)&server_addr, addr_len)) < 0)
    {
        perror("sendto");
        printf("sendto");
        return -1;
    }
    printf("waiting for reply\n");

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

    printf("icmp_reply->type = %d\n", icmp_reply->type);
    printf("icmp_reply->checksum = %d\n", icmp_reply->checksum);
    printf("icmp_reply->code = %d\n", icmp_reply->code);
    printf("icmp_reply->un.echo.sequence = %d\n", icmp_reply->un.echo.sequence);

    if (icmp_reply->type == ICMP_ECHOREPLY)
    {
        printf("Received ICMP ECHO REPLY\n");
        timersub(&end, &start, &diff);
        rtt = (double)diff.tv_sec * 1000.0 + (double)diff.tv_usec / 1000.0;

        printf("Received ping response from %s: seq_num=%d rtt=%.3fms\n", argv[1], seq_num, rtt);
    }
    else
    {
        printf("Received ICMP ECHO REPLY: %d\n", icmp_reply->type);
    }

    close(sockfd);

    // print statistics

    return 0;
}

void input_validator()
{
    return;
}