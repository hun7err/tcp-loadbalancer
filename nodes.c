
int pool_size = 3;

struct fd_pair {
    int in;
    int out;
};

struct node_info {
    struct fd_pair conn_map[512];
    int conn_count;
    int poll_usage;
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
        nodes[i].poll_usage = 0;
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

int get_new_node_socket(int id, int out_port)
{
    int i, found = 0;

    for(i = 0; i < 512; i++)
    {
        if(nodes[id].conn_map[i].in == -1 && nodes[id].conn_map[i].out == -1)
        {
            found = id;
        }
    }

    if(!found)
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
    nodes[found].conn_count++;

    return socket;
}

int add_new_socket_mapping(int client, int node, int id)
{
}

int get_corresponding_socket(int socket) // zwroc odpowiadajacy socket z conn_map
{
}

int get_next_socket(int node_id) // ??
{
}



