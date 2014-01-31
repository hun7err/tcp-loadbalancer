
int pool_size = 3;

struct fd_pair {
    int in;
    int out;
};

struct node_info {
    struct fd_pair conn_map[512];
    int conn_count;
    int pool_usage;
    char ip[INET_ADDRSTRLEN];
    int id;
    int used;
};

const int max_node_count = 32;
struct node_info nodes[max_node_count];

void initialize_nodes(void)
{
    memset(nodes, 0, sizeof(struct node_info)*max_node_count);
    int i, j;
    for(i = 0; i < max_host_count; i++)
    {
        for(j = 0; j < 512; j++)
        {
            nodes[i].conn_map[j].in = nodes[i].conn_map[j].out = -1;
        }
        nodes[i].conn_count = 0;
        nodes[i].id = i;
        nodes[i].used = 0;
        nodes[i].pool_usage = 0;
    }
}

int add_new_node(char* address) // zwroc id node'a
{
    int i;
    for(i = 0; i < max_node_count; i++)
    {
        if(nodes[i].used == 0)
        {
            nodes[i].used = 1;
            strcpy(nodes[i].ip, address);
            return nodes[i].id;
        }
    }
    return -1;
}

int add_new_node_socket(int id, int out_port)
{
    int i, found = -1;

    for(i = 0; i < 512; i++)
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

    int socket = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0);
    if(socket == -1)
    {
        perror("[!] Could not create socket");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(out_port);

    inet_pton(AF_INET, nodes[found].ip, &(addr.sin_addr));

    if(connect(socket, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("[!] Could not connect to node");
        return -1;
    }

    nodes[id].conn_map[found].out = socket;
    return socket;
}

int create_connection_pool(int node_id, int out_port)
{
    int i;

    for(i = 0; i < pool_size; i++)
    {
        if(add_new_node_socket(node_id, out_port) == -1)
        {
            perror("[!] Could not create socket on the node!");
            return -1;
        }
        nodes[best_id].pool_usage--;
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

    for(current_connection = 0; current_connection < 512; current_connection++)
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
                nodes[best_node_id].conn_map[current_connection].in = client;
                nodes[best_node_id].conn_map[current_connection].used = 1;
                nodes[best_node_id].conn_count++;
                nodes[best_node_id].pool_usage++;

                return 0;
            }
        }
    }

    if(backup_id == -1)
    {
        return -1;
    }
    
    int out_socket = get_new_node_socket(best_node_id, out_port);

    if(out_socket == -1)
    {
        return -1;
    }

    nodes[best_node_id].conn_map[backup_id].in = client;
    nodes[best_node_id].conn_map[backup_id].out = out_socket;
    nodes[best_node_id].conn_count++;
}

int get_best_node(void) // wybierz wezel o najmniejszym obciazeniu
{
    int id_of_min_conns = nodes[0].conn_count, i;

    for(i = 0; i < max_node_count; i++)
    {
        if(nodes[id_of_min_conns].conn_count == 0 && nodes[i].conn_count != 0)
        {
            id_of_min_conns = i;
        }
        else if (nodes[id_of_min_conns].conn_count != 0)
        {
            if(nodes[i].conn_count < nodes[id_of_min_conns].conn_count)
            {
                id_of_min_conns = i;
            }
        }
    } 

    return id_of_min_conns;
}



