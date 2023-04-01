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

int main(int argc, char** argv)
{
    // takes DNS name or IP
    // 1 <= n <= INT_MAX
    // 1 <= T <= LONG_MAX
    char* hostname_or_ip;
    void* ip;
    char str[INET_ADDRSTRLEN];
    struct hostent* hptr;
    int n;
    unsigned long T;
    // Input validation
    if(argc < 4) { printf("Usage: ./a.out <hostname/IP> <n> <T>"); return EXIT_FAILURE; }

    hostname_or_ip = argv[1];
    n = atoi(argv[2]); if( n<=0 ) { printf("n must be a positive integer"); return EXIT_FAILURE; }
    T = strtoul((const char*)argv[3], NULL, 10); if ( T<=0 || !T || T == ULONG_MAX) { printf("T must be a positive integer"); return EXIT_FAILURE; }
    // Obtaining IP from hostname (note: handle IP input separately)
    hptr = gethostbyname(hostname_or_ip);
    if(!hptr) { printf("Error in gethostbyname: %s\n", hstrerror(h_errno)); return EXIT_FAILURE; }
    if(hptr->h_addrtype != AF_INET) { printf("Error: Only AF_INET supported"); return EXIT_FAILURE; }
    
    printf("IP Address(es):\n");
    for(char** ptr = hptr->h_addr_list; *(ptr) != NULL; ptr++)
    {
        if(inet_ntop(hptr->h_addrtype, *ptr, str, sizeof(str)) == NULL) { printf("Error in h_addr_list: %s\n", hstrerror(h_errno)); return EXIT_FAILURE; }
        printf("%s\n", str);
    }
    printf("n = %d, T = %lu\n", n, T);

    // first IP address is used
    ip = hptr->h_addr_list[0];

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(sockfd == -1) { perror("ERROR: Socket creation failure"); exit(EXIT_FAILURE); }

    
    return 0;
}

void input_validator()
{
    return;
}