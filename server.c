#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h> // temp
#include <arpa/inet.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>

int main(int argc, char ** argv)
{
    if (argc < 3)
    {
        printf("Usage: %s <in_port> <in iface> [out_port] [config_port]\n", argv[0]);
        return -1;
    }
    int in_port = atoi(argv[1]),
        out_port,
        config_port = 1337,
        sd_in = socket(AF_INET, SOCK_STREAM, 0);
    
    if(argc < 4)
    {
        out_port = in_port;
    }
    else
    {
        out_port = atoi(argv[3]);
        if(argc == 5)
        {
            config_port = atoi(argv[4]);
        }
    }

    if(sd_in == -1)
    {
        perror("Could not create socket");
        return -1;
    }

    struct ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;
    memset(ifr.ifr_name, 0, sizeof(ifr.ifr_name));
    strncpy(ifr.ifr_name, argv[2], strlen(argv[2]));
    ioctl(sd_in, SIOCGIFADDR, &ifr);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr; // .s_addr ?
    server_addr.sin_port = htons(in_port);

    printf("Binding socket to %s:%d\n", inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr), in_port);

    if(bind(sd_in, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        close(sd_in);
        perror("Could not bind socket");
        return -1;
    }

    if(listen(sd_in, 10) == -1)
    {
        close(sd_in);
        perror("Could not start listening on the provided interface\n");
        return -1;
    }

    printf("Listening for connections\n");
    int client_sd;
    char data_buffer[1024];
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    do {
        memset(&client_addr, 0, sizeof(client_addr));
        client_sd = accept(sd_in, (struct sockaddr*)&client_addr, &client_addr_len);
        if(client_sd == -1)
        {
            close(sd_in);
            perror("Error handling client connection");
            return -1;
        }
        printf("Incoming connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        close(client_sd);
        return 0;

    } while(0);
    
    close(sd_in);

    return 0;
}

