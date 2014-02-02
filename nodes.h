#ifndef __NODES_H__
#define __NODES_H__

#include <arpa/inet.h>

extern int pool_size;
#define MAX_CONNECTION_COUNT 512
#define MAX_NODE_COUNT 32

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

extern struct node_info nodes[MAX_NODE_COUNT];
extern int current_node_count;

void initialize_nodes(void);
int add_new_node(char* address, int out_port);    // dodaj nowy wezel po adresie ip, stworz na nim connection pool i zwroc id w nodes[]
int remove_node(char* address);
int add_new_socket(int id, int out_port);
int create_connection_pool(int node_id, int out_port);
int add_new_client(int client, int out_port);
int remove_client(int sock);
int get_best_node(void);
int get_corresponding_socket(int sock);
int repair_node_pool(int node_sock, int out_port);
int is_node_socket(int sock);

#endif // __NODES_H__
