#include "../inc/packet.h"
#include <stdlib.h>
#include <string.h>

int node_list_packet_size(int host_count)
{
    return sizeof(struct header) + sizeof(struct node_list_header) + host_count * sizeof(struct node_list_item_ipv4);
}

char* node_list_packet(int host_count, int pool_size, int ip_version, struct node_list_item_ipv4* nodes)
{
    // magic, do not touch.

    int length = sizeof(struct node_list_header) + host_count * sizeof(struct node_list_item_ipv4);
    char* packet = calloc(sizeof(struct header)+length, 1);
    struct header h = {LIST_NODES, length};
    struct node_list_header h_nodes = {host_count, pool_size, ip_version};

    memcpy(packet, (char*)&h, sizeof(struct header));
    memcpy(packet+sizeof(struct header), (char*)&h_nodes, sizeof(struct node_list_header));
    int i;

    for(i = 0; i < host_count; i++)
    {
        memcpy(packet+sizeof(struct header)+sizeof(struct node_list_header)+i*sizeof(struct node_list_item_ipv4), (char*)&nodes[i], sizeof(struct node_list_item_ipv4));
    }

    return packet;
}

