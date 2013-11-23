#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <net/if.h>

#define PKT_LEN 65535

unsigned short crc(unsigned short *buf, int len)
{
    unsigned long sum;
    for(sum = 0; len > 0; len--)
    {
        sum += *buf++;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return (unsigned short)(~sum);
}

int main(int argc, char *argv[])
{
    int sd_in, sd_out;
    char buffer[PKT_LEN];
    struct sockaddr_in sin, din;
    int one = 1;
    const int *val = &one;

    memset(buffer, 0, PKT_LEN);

    if(argc != 3)
    {
        printf("invalid parameters!\n");
        printf("usage: %s <iface in> <iface out>\n", argv[0]);
        exit(-1);
    }

    sd_in = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if(sd_in < 0)
    {
        perror("in socket() error");
        return -1;
    }
    sd_out = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
    if(sd_out < 0)
    {
        perror("out socket() error");
        return -1;
    }
    struct ifreq ifr;
    ifr.ifr_addr.sa_family = PF_INET;
    strncpy(ifr.ifr_name, argv[2], strlen(argv[2]));
    ioctl(sd_out, SIOCGIFADDR, &ifr);
    printf("sending as %s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

    // ustawienie nasluchiwania tylko na wybranym interfejsie sieciowym
    setsockopt(sd_in, SOL_SOCKET, SO_BINDTODEVICE, argv[1], strlen(argv[1]));
    setsockopt(sd_out, SOL_SOCKET, SO_BINDTODEVICE, argv[2], strlen(argv[2]));    

    char buf[PKT_LEN];
    int n;
    while(1)
    {
        n = recv(sd_in, buf, PKT_LEN, 0);
    //{
        struct iphdr *ip_header = (struct iphdr*)buf;
        struct tcphdr* tcp_header = (struct tcphdr*)(buf + ip_header->ihl * 4);

        //printf("protocol = %d\n", ip_header->protocol);
        
        struct sockaddr_in source, dest;
        
        memset(&source, 0, sizeof(source));
        source.sin_addr.s_addr = ip_header->saddr;
        memset(&dest, 0, sizeof(dest));
        dest.sin_addr.s_addr = ip_header->daddr;
        
        printf("src = %s", inet_ntoa(source.sin_addr));
        printf(":%d", ntohs(tcp_header->source));
        printf(", dst = %s", inet_ntoa(dest.sin_addr));
        printf(":%d\n", ntohs(tcp_header->dest));
    }
    
    close(sd_in);
    close(sd_out);

    return 0;
}

