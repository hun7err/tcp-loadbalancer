#include "commands.h"
#include "nodes.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern int client_sd, sd_in, epoll_fd, config_sd;

int interpret_command(int type, char* command)
{
    if(type == ADD_NODE)
    {
        struct node_to_add_ipv4 node = *(struct node_to_add_ipv4*)command;

        add_new_node(node.address, node.port);

        return 0;
    }
    else if(type == REMOVE_NODE)
    {
        mark_for_removal(command);
        remove_nodes();

        return 0;
    }
    else if(type == LIST_NODES)
    {
        struct node_list_item_ipv4* node_list = get_node_list();

        char* packet = node_list_packet(current_node_count, pool_size, 4, node_list);

        int len = send(client_sd, packet, node_list_packet_size(current_node_count), 0);

        if(len < 0)
        {
            perror("[!] Error sending response to client: ");
            close(client_sd);
            client_sd = -1;

            free(packet);
            return -1;
        }

        free(packet);

        return 0;
    }
    else
    {
        printf("[!] Unrecognized configuration command\n");
        struct header h;
        h.type = COMMAND_NOT_IMPLEMENTED;
        h.length = 0;

        if(send(client_sd, (char*)&h, sizeof(h), 0) < 0)
        {
            perror("[!] Could not send response");
            close(client_sd);
            client_sd = -1;
        }

        return -1;
    }
}

struct node_list_item_ipv4* get_node_list(void)
{
    int i, current_node = 0;
    struct node_list_item_ipv4* node_list = calloc(current_node_count * sizeof(struct node_list_item_ipv4), 1);

    for(i = 0; i < MAX_NODE_COUNT; i++)
    {
        if(nodes[i].used != 0)
        {
            node_list[current_node].connection_count = nodes[i].conn_count;
            node_list[current_node].pool_usage = nodes[i].pool_usage;
            strncpy(node_list[current_node].address, nodes[i].ip, strlen(nodes[i].ip)+1);

            current_node++;
        }
    }

    return node_list;
}

