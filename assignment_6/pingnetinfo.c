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

unsigned short checksum(unsigned short *buf, int nwords)
{
    unsigned long sum;
    for (sum = 0; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return ~sum;
}

int main(int argc, char **argv)
{
    // takes DNS name or IP
    // 1 <= n <= INT_MAX
    // 1 <= T <= LONG_MAX
    signal(SIGINT, exitHandler);

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

    char packet[PACKET_SIZE];
    char empty_packet[sizeof(struct icmphdr)];

    int seq_num = 0;
    struct sockaddr_in server_addr;
    unsigned int addr_len = sizeof(server_addr);
    double rtt;

    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) < 0)
    {
        printf("inet_pton");
        return -1;
    }

    icmp_header = (struct icmphdr *)empty_packet;

    icmp_header->type = ICMP_ECHO;
    icmp_header->code = 0;
    icmp_header->un.echo.sequence = seq_num;
    icmp_header->checksum = checksum(icmp_header, sizeof(struct icmphdr));

    // printf("icmp_header->type = %d\n", icmp_header->type);
    // printf("icmp_header->checksum = %d\n", icmp_header->checksum);
    // printf("icmp_header->code = %d\n", icmp_header->code);
    // printf("icmp_header->un.echo.sequence = %d\n", icmp_header->un.echo.sequence);

    gettimeofday(&start, NULL);

    int status;
    if ((status = sendto(sockfd, icmp_header, sizeof(struct icmphdr), 0, (struct sockaddr *)&server_addr, addr_len)) < 0)
    {
        printf("sendto");
        return -1;
    }

    if ((status = recvfrom(sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, &addr_len)) < 0)
    {
        printf("recvfrom");
        return -1;
    }

    gettimeofday(&end, NULL);

    struct iphdr *ip_header = (struct iphdr *)packet;
    int ip_header_len = ip_header->ihl * 4;

    icmp_reply = (struct icmphdr *)(packet + ip_header_len);

    printf("icmp_header->type = %d\n", icmp_header->type);
    printf("icmp_header->checksum = %d\n", icmp_header->checksum);
    printf("icmp_header->code = %d\n", icmp_header->code);
    printf("icmp_header->un.echo.sequence = %d\n", icmp_header->un.echo.sequence);

    if (icmp_header->type == ICMP_ECHOREPLY)
    {
        printf("Received ICMP ECHO REPLY\n");
        timersub(&end, &start, &diff);
        rtt = (double)diff.tv_sec * 1000.0 + (double)diff.tv_usec / 1000.0;

        printf("Received ping response from %s: seq_num=%d rtt=%.3fms\n", argv[1], seq_num, rtt);
    }
    else
    {
        printf("Received ICMP ECHO REPLY: %d\n", icmp_header->type);
    }

    close(sockfd);

    // print statistics

    return 0;
}

void input_validator()
{
    return;
}