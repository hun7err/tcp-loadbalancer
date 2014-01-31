#ifndef __NODES_H__
#define __NODES_H__

void initialize_nodes(void);
int add_new_node(char* address);    // dodaj nowy wezel po adresie ip i zwroc id w nodes[]
int add_new_socket(int id, int out_port);
int create_connection_pool(int node_id, int out_port);
int add_new_client(int client, int out_port);
int get_best_node(void);

#endif // __NODES_H__
