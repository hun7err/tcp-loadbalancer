#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h> // temp
#include <arpa/inet.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>

#define ISENGARD nodes[0]
#define MORDOR nodes[1]
#define KHAZADDUM nodes[2]

struct node_info {
    
};

struct out_data {
    int client_sd;
    int* out_sd;
    int out_port;
};

struct client_data {
    int client_sd;
    int* out_sd;
};

const char* servers[] = {"192.168.122.70", "192.168.122.46", "192.168.122.32"};

void *clientInHandler(void* client_data_ptr)
{
    struct client_data data = *(struct client_data*)client_data_ptr;

    char buffer[1024];
    while(*(data.out_sd) < 0)
    {
        if(*data.out_sd == -2)
        {
            return NULL;
        }
    }

    int n;
    while((n = recv(data.client_sd, buffer, 1024, 0)) != -1)
    {
        send(*(data.out_sd), buffer, n, 0);
    }
    close(data.client_sd);

    return NULL;
}

void *clientOutHandler(void* out_data_ptr)
{
    struct out_data data = *(struct out_data*)out_data_ptr;
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    char buffer[1024];

    struct sockaddr_in addr;
    addr.sin_port = htons(data.out_port);
    inet_pton(AF_INET, servers[0], &(addr.sin_addr));
    addr.sin_family = AF_INET;

    int rc = connect(sd, (struct sockaddr*)&addr, sizeof(addr));
    if(rc < 0)
    {
        *(data.out_sd) = -2;
        perror("Could not connect()\n");
        return NULL;
    }
    *(data.out_sd) = sd;

    int len;
    while((len = recv(*(data.out_sd), buffer, 1024, 0)) != -1)
    {
        send(data.client_sd, buffer, len, 0);
    }
    close(sd);

    return NULL;
}

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

    char address[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr, address, INET_ADDRSTRLEN);

    printf("Binding socket to %s:%d\n", address, in_port);
    int out_sd = -1;

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

    pthread_t in_handler, out_handler;
    int rc1, rc2;

    while(1) {
        memset(&client_addr, 0, sizeof(client_addr));
        client_sd = accept(sd_in, (struct sockaddr*)&client_addr, &client_addr_len);
        if(client_sd == -1)
        {
            close(sd_in);
            perror("Error handling client connection");
            return -1;
        }
        inet_ntop(AF_INET, &(client_addr.sin_addr), address, INET_ADDRSTRLEN);
        printf("Incoming connection from %s:%d\n", address, ntohs(client_addr.sin_port));
        struct out_data* data = malloc(sizeof(struct out_data));
        data->client_sd = client_sd;
        data->out_port = out_port;
        data->out_sd = &out_sd;

        struct client_data* client_data_ptr = malloc(sizeof(struct client_data));
        client_data_ptr->client_sd = client_sd;
        client_data_ptr->out_sd = &out_sd;

        rc1 = pthread_create(&in_handler, NULL, clientInHandler, client_data_ptr);
        rc2 = pthread_create(&out_handler, NULL, clientOutHandler, data);

        //close(client_sd);
        //return 0;

    };
    
    close(sd_in);

    return 0;
}

