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
#include <errno.h>
#include <ifaddrs.h>

#define PACKET_SIZE 64
#define MAX_NO_PACKETS 5
#define MAX_NO_HOPS 30
#define RECV_TIMEOUT 10
#define MAX_NO_DATA_SIZES 2

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

void set_ip_header(struct iphdr *ip_header, char *dest, int ttl, int packet_size)
{
    ip_header->ihl = 5;
    ip_header->version = 4;
    ip_header->tos = 0;
    ip_header->tot_len = packet_size;
    ip_header->id = htons(0);
    ip_header->frag_off = 0;
    ip_header->ttl = ttl;
    ip_header->protocol = IPPROTO_ICMP;
    ip_header->check = 0;

    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[INET_ADDRSTRLEN];
    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;
        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET)
        {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
            if (s != 0)
            {
                printf("getnameinfo() failed: %s", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            if (strcmp(ifa->ifa_name, "eth0") == 0)
            {
                break;
            }
        }
    }
    freeifaddrs(ifaddr);

    ip_header->saddr = inet_addr(host); // source IP address
    ip_header->daddr = inet_addr(dest); // destination IP address
}

void set_icmp_header(struct icmphdr *icmp_header)
{
    icmp_header->type = ICMP_ECHO;
    icmp_header->code = 0;
    icmp_header->un.echo.id = getpid();
    icmp_header->un.echo.sequence = 0;
    icmp_header->checksum = 0;
    icmp_header->checksum = csum((uint16_t *)icmp_header, PACKET_SIZE - sizeof(struct iphdr));
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

    int *data_sizes = (int *)malloc(MAX_NO_DATA_SIZES * sizeof(int));
    data_sizes[0] = 0;
    data_sizes[1] = 16;

    double *prev_rtt = (double *)malloc(MAX_NO_DATA_SIZES * sizeof(double));
    for (int i = 0; i < MAX_NO_DATA_SIZES; i++)
        prev_rtt[i] = 0;
    double *curr_rtt = (double *)malloc(MAX_NO_DATA_SIZES * sizeof(double));

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

    struct iphdr *ip_header;
    struct icmphdr *icmp_header;
    struct icmphdr *icmp_reply;
    struct timeval start, end, diff;

    int seq_num = 0;
    struct sockaddr_in server_addr;
    unsigned int addr_len = sizeof(server_addr);
    double rtt;

    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

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

    int reached = 0;
    for (ttl = 1; ttl < MAX_NO_HOPS && !reached; ttl++)
    {
        const int on = 1;
        if ((status = setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on))) < 0)
        {
            printf("setsockopt IP_HDRINCL");
            exit(EXIT_FAILURE);
        }
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

        for (int seq = 0; seq < MAX_NO_PACKETS && !reached; seq++)
        {
            memset(packet, 0, PACKET_SIZE);
            ip_header = (struct iphdr *)packet;
            set_ip_header(ip_header, str, ttl, PACKET_SIZE);

            icmp_header = (struct icmphdr *)(packet + ip_header->ihl * 4);
            set_icmp_header(icmp_header);

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
                reached = 1;
                break;
            }
            else
            {
                printf("* * * *\n");
            }
            sleep(1);
        }

        double bandwidth = 0;
        double latency = 0;

        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < MAX_NO_DATA_SIZES; j++)
            {
                memset(packet, 0, PACKET_SIZE);
                ip_header = (struct iphdr *)packet;
                set_ip_header(ip_header, str, ttl, sizeof(struct iphdr) + sizeof(struct icmphdr) + data_sizes[j]);

                icmp_header = (struct icmphdr *)(packet + ip_header->ihl * 4);
                set_icmp_header(icmp_header);

                gettimeofday(&start, NULL);
                if ((status = sendto(sockfd, packet, ip_header->tot_len, 0, (struct sockaddr *)&server_addr, addr_len)) < 0)
                {
                    printf("sendto");
                    return -1;
                }
                unsigned char *reply = (unsigned char *)malloc(PACKET_SIZE * sizeof(unsigned char));

                if ((status = recvfrom(sockfd, reply, ip_header->tot_len, 0, (struct sockaddr *)&server_addr, &addr_len)) < 0)
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
                printf("DEBUG: %lf\n", rtt);
                curr_rtt[j] = rtt;
                if (j == 0)
                    latency += rtt / 2;

                sleep(1);
            }

            double temp = (2.0 * (data_sizes[1] - data_sizes[0])) / (curr_rtt[1] - prev_rtt[1] - curr_rtt[0] + prev_rtt[0]);
            printf("bandwidth: %lf Bps\n", temp);
            bandwidth += temp;

            for (int i = 0; i < MAX_NO_DATA_SIZES; i++)
                prev_rtt[i] = curr_rtt[i];

            sleep(T);
        }
        bandwidth /= n;
        latency /= n;

        printf("avg bandwidth: %lf Bps\n", bandwidth);
        printf("avg latency: %lf ms\n", latency);
        bandwidth = 0;

        printf("rtt: %f\n\n", min_rrt);
    }

    close(sockfd);

    // print statistics

    return 0;
}

void input_validator()
{
    return;
}