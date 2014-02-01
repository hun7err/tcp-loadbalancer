#include "nodes.h"
#include "sockets.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>

extern int epoll_fd;
// ewentualnie tutaj extern sd_in, config_sd itp.

#define MAX_CONNECTION_COUNT 512
#define MAX_NODE_COUNT 32

int pool_size = 3;

struct fd_pair {
    int in;
    int out;
    int used;
};

struct node_info {
    struct fd_pair conn_map[MAX_CONNECTION_COUNT];
    int conn_count;
    int pool_usage;
    char ip[INET_ADDRSTRLEN];
    int id;
    int used;
};

int current_node_count;
struct node_info nodes[MAX_NODE_COUNT];

void initialize_nodes(void)
{
    printf("[ ] Initializing node info...\n");
    memset(nodes, 0, sizeof(struct node_info)*MAX_NODE_COUNT);
    int i, j;
    for(i = 0; i < MAX_NODE_COUNT; i++)
    {
        for(j = 0; j < MAX_CONNECTION_COUNT; j++)
        {
            nodes[i].conn_map[j].in = nodes[i].conn_map[j].out = -1;
            nodes[i].conn_map[j].used = 0;
        }
        nodes[i].conn_count = 0;
        nodes[i].id = i;
        nodes[i].used = 0;
        nodes[i].pool_usage = 0;
    }
    printf("[+] Node info filled in successfully\n");
}

int add_new_node(char* address, int out_port) // zwroc id node'a
{
    int i;
    for(i = 0; i < MAX_NODE_COUNT; i++)
    {
        if(nodes[i].used == 0)
        {
            nodes[i].used = 1;
            current_node_count++;
            strncpy(nodes[i].ip, address, INET_ADDRSTRLEN);
            if(create_connection_pool(i, out_port) == -1)
            {
                perror("[!] Connection pool creation failed");
            }

            return nodes[i].id;
        }
    }
    printf("[!] Could not add the node\n");
    return -1;
}

int add_new_node_socket(int id, int out_port)
{
    int i, found = -1;

    for(i = 0; i < MAX_CONNECTION_COUNT; i++)
    {
        if(nodes[id].conn_map[i].in == -1   // nie jest wykorzystany socket wejsciowy
        && nodes[id].conn_map[i].out == -1)  // nie jest wykorzystany socket wyjsciowy
        {
            found = i;
            break;
        }
    }

    if(found == -1)
    {
        return -1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1)
    {
        perror("[!] Could not create socket");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(out_port);

    inet_pton(AF_INET, nodes[id].ip, &(addr.sin_addr));

    if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("[!] Could not connect to node");
        return -1;
    }

    if(make_socket_nonblocking(sock) == -1)
    {
        perror("[!] Could not make the socket non-blocking");
        close(sock);
        return -1;
    }

    nodes[id].conn_map[found].out = sock;
    return sock;
}

int create_connection_pool(int node_id, int out_port)
{
    int i;

    for(i = 0; i < pool_size; i++)
    {
        if(add_new_node_socket(node_id, out_port) == -1)
        {
            perror("[!] Could not create socket on the node");
            return -1;
        }
        nodes[node_id].pool_usage--;
    }

    return 0;
}

int add_new_client(int client, int out_port)  // gotowe
{
    int current_connection,
        best_node_id = get_best_node();

    if(nodes[best_node_id].pool_usage == pool_size)
    {
        if(create_connection_pool(best_node_id, out_port) == -1)
        {
            return -1;
        }
    }

    int backup_id = -1, backup_found = 0;

    static struct fd_pair current_mapping;

    for(current_connection = 0; current_connection < MAX_CONNECTION_COUNT; current_connection++)
    {
        current_mapping = nodes[best_node_id].conn_map[current_connection];
        if(current_mapping.used == 0)
        {
            if(!backup_found && current_mapping.out == -1)
            {
                backup_id = current_connection;
            }
            else if (current_mapping.out != 0)
            {
                int node_sd = nodes[best_node_id].conn_map[current_connection].out;

                struct epoll_event event;
                event.data.fd = node_sd;
                event.events = EPOLLIN | EPOLLET;

                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, node_sd, &event) == -1)
                {
                    perror("[!] epoll_ctl node_fd");
                    return -1;
                }

                nodes[best_node_id].conn_map[current_connection].in = client;
                nodes[best_node_id].conn_map[current_connection].used = 1;
                nodes[best_node_id].conn_count++;
                nodes[best_node_id].pool_usage++;

                return node_sd;
            }
        }
    }

    if(backup_id == -1)
    {
        return -1;
    }
    
    int out_socket = add_new_node_socket(best_node_id, out_port);

    if(out_socket == -1)
    {
        return -1;
    }

    nodes[best_node_id].conn_map[backup_id].in = client;
    nodes[best_node_id].conn_map[backup_id].out = out_socket;
    nodes[best_node_id].conn_count++;

    return 0;
}

int get_best_node(void) // wybierz wezel o najmniejszym obciazeniu
{
    int id_of_min_conns = 0, i;

    for(i = 0; i < MAX_NODE_COUNT; i++)
    {
        if(nodes[id_of_min_conns].used == 0)
        {
            id_of_min_conns = i;
        }

        if(nodes[i].used == 1)
        {
            if(nodes[i].conn_count < nodes[id_of_min_conns].conn_count)
            {
                id_of_min_conns = i;
            }
        }
    } 

    return id_of_min_conns;
}

int get_corresponding_socket(int sock)
{
    int i, j;
    for(i = 0; i < MAX_NODE_COUNT; i++)
    {
        if(nodes[i].used != 0)
        {
            for(j = 0; j < MAX_CONNECTION_COUNT; j++)
            {
                if(nodes[i].conn_map[j].in == sock)
                {
                    return nodes[i].conn_map[j].out;
                }
                else if(nodes[i].conn_map[j].out == sock && nodes[i].conn_map[j].in != -1)
                {
                    return nodes[i].conn_map[j].in;
                }
            }
        }
    }

    return -1;
}

int is_node_socket(int sock)
{
    int i, j;
    for(i = 0; i < MAX_NODE_COUNT; i++)
    {
        for(j = 0; j < MAX_CONNECTION_COUNT; j++)
        {
            if(nodes[i].conn_map[j].out == sock)
            {
                return 0;
            }
        }
    }

    return -1;
}

int remove_node(char* address)
{
    return 0;
}

int repair_node_pool(int node_sock, int out_port) // nic nie daje
{
    //int flags;
    //fcntl(node_sock, F_GETFD, &flags);

    //if(flags & EBADF)
    //{
        printf("Found a bad FD\n");
        int i, j, id;
        for(i = 0; i < MAX_NODE_COUNT; i++)
        {
            for(j = 0; j < MAX_CONNECTION_COUNT; j++)
            {
                if(nodes[i].conn_map[j].out == node_sock)
                {
                    id = i;
                    break;
                }
            }
        }

        close(node_sock);
        // ustal na ktorym node jest ten socket
        if(add_new_node_socket(id, out_port) == -1)
        {
            return -1;
        }

        struct epoll_event event;
        event.data.fd = node_sock;
        event.events = EPOLLIN | EPOLLET;

        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, node_sock, &event) == -1)
        {
            close(node_sock);
            perror("[!] epoll_ctl node_sock");
            return -1;
        }
    //}

    return 0;
}

int remove_client(int sock)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);

    close(sock);
    return 0;
}

