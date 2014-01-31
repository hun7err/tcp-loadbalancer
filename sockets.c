#include "sockets.h"
#include <fcntl.h>      // fcntl
#include <stdio.h>      // perror
#include <unistd.h>     // close
#include <sys/types.h>  // socket, bind
#include <sys/socket.h> // socket, bind
#include <net/if.h>     // ifreq
#include <sys/ioctl.h>  // ioctl
#include <arpa/inet.h>  // inet_ntop
#include <string.h>     // memset

extern int sd_in, config_sd;

int make_socket_nonblocking(int socket)
{
    int flags;

    flags = fcntl(socket, F_GETFL, 0);
    if(flags == -1)
    {
        perror("[!] Fcntl failed");
        return -1;
    }

    flags |= O_NONBLOCK;

    if(fcntl(socket, F_SETFL, flags) == -1)
    {
        perror("[!] Fcntl failed");
        return -1;
    }

    return 0;
}

int create_and_bind_sockets(char *iface, int in_port, int config_port)
{
    sd_in = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0);
    config_sd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0);

    if(sd_in == -1)
    {
        perror("[!] Could not create the listening socket\n");
        return -1;
    }
    printf("[+] Listening socket created\n");

    if(config_sd == -1)
    {
        perror("[!] Could not create the configuration socket\n");
        return -1;
    }

    printf("[+] Configuration socket created\n");

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, iface, strlen(iface));
    ioctl(sd_in, SIOCGIFADDR, &ifr);

    struct sockaddr_in server_addr, server_config_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&server_config_addr, 0, sizeof(server_config_addr));

    server_addr.sin_family = server_config_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = server_config_addr.sin_addr.s_addr = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr;
    server_addr.sin_port = htons(in_port);
    server_config_addr.sin_port = htons(config_port);

    char address_string[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr, address_string, INET_ADDRSTRLEN);

    printf("[ ] Binding listening socket to %s:%d\n", address_string, in_port);

    if(bind(sd_in, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("[!] Could not bind the listening socket");
        return -1;
    }

    printf("[+] Listening socket binded\n");

    printf("[ ] Binding configuration socket to %s:%d\n", address_string, config_port);

    if(bind(config_sd, (struct sockaddr*)&server_config_addr, sizeof(server_config_addr)) == -1)
    {
        perror("[!] Could not bind the configuration socket");
        return -1;
    }

    return 0;
}

int accept_client(int socket)
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    static int new_client_sd;
    new_client_sd = accept(socket, (struct sockaddr*)&client_addr, &client_addr_len);

    if(new_client_sd == -1)
    {
        perror("[!] Could not accept client connection");
        return -1;
    }

    char address_string[INET_ADDRSTRLEN];
    memset(address_string, 0, INET_ADDRSTRLEN);

    inet_ntop(AF_INET, &(client_addr.sin_addr), address_string, INET_ADDRSTRLEN);
    printf("[+] Accepted new client: %s:%d on socket %d\n", address_string, ntohs(client_addr.sin_port), new_client_sd);

    if(make_socket_nonblocking(new_client_sd) == -1)
    {
        printf("[!] Could not make the socket non-blocking; closing connection");
        close(new_client_sd);
        return -1;
    }

    return new_client_sd;
}

