
int pool_size = 3;

struct fd_pair {
    int in;
    int out;
};

struct node_info {
    struct fd_pair conn_map[512];
    int conn_count;
    int poll_usage;
    char name[64];
};

const int max_host_count = 32;
struct node_info nodes[max_host_count];

void initialize_nodes(void)
{
    memset(nodes, 0, sizeof(struct node_info)*max_host_count);
    int i, j;
    for(i = 0; i < max_host_count; i++)
    {
        for(j = 0; j < 512; j++)
        {
            nodes[i].conn_map[j].in = nodes[i].conn_map[j].out = -1;
        }
        nodes[i].conn_count = 0;
    }
}

int get_new_node(void)
{
}

