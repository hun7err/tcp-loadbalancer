#ifndef __PACKET_H__
#define __PACKET_H__

#define COMMAND_NOT_IMPLEMENTED 0x00
#define ADD_NODE    0x01
#define LIST_NODES  0x02
#define LIST_NODES_RESPONSE 0x03
#define REMOVE_NODE 0x04

struct header {
    int type;
    int length;
} __attribute__ ((packed));

struct node_list_header {
    int host_count;
    int pool_size;
    int ip_version;
} __attribute__ ((packed));

struct node_list_item_ipv4 {
    char address[16];
    int connection_count;
    int pool_usage;
} __attribute__ ((packed));

char* node_list_packet(int host_count, int pool_size, int ip_version, struct node_list_item_ipv4* nodes);
int node_list_packet_size(int host_count);

struct node_to_add_ipv4 {
    char address[16];
    int port;
} __attribute__ ((packed));

struct node_to_remove_ipv4 {
    char address[16];
} __attribute__ ((packed));

#endif // __PACKET_H__
