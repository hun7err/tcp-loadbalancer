#ifndef __NODES_H__
#define __NODES_H__

#include <arpa/inet.h>  // INET_ADDRSTRLEN

extern int pool_size;   // wielkosc connection pool
#define MAX_CONNECTION_COUNT 512    // maksymalna liczba polaczen per wezel
#define MAX_NODE_COUNT 32       // maksymalna liczba wezlow

struct fd_pair {
    int in;     // socket wejsciowy (od klienta)
    int out;    // socket wyjsciowy (do wezla)
    int used;   // zajety?
};

struct node_info {
    struct fd_pair conn_map[MAX_CONNECTION_COUNT];  // mapa polaczen (klient <-> wezel)
    int conn_count; // aktualna liczba polaczen
    int pool_usage; // zuzycie connection pool, >= 0, <= pool_size
    char ip[INET_ADDRSTRLEN];   // adres ip wezla w postaci dziesietnej kropkowanej
    int id;     // numer identyfikacyjny, indeks w nodes[]
    int used;   // uzywany/zajety?
    int to_remove;  // aby oznaczyc do usuniecia
};

extern struct node_info nodes[MAX_NODE_COUNT];  // lista wezlow
extern int current_node_count;  // aktualna liczba wezlow

void initialize_nodes(void);    // inicjalizuj tablice i zmienne zwiazane z wezlami/klientami wezlow
int add_new_node(char* address, int out_port);    // dodaj nowy wezel po adresie ip, stworz na nim connection pool i zwroc id w nodes[]
int mark_for_removal(char* address);    // oznacz wezel do usuniecia
void remove_nodes(void);    // usun wezly oznaczone do usuniecia
int add_new_socket(int id, int out_port);   // dodaj nowy socket do wezla
int create_connection_pool(int node_id, int out_port);  // utworz connection pool
int add_new_client(int client, int out_port);   // dodaj nowego klienta
int remove_client(int sock);    // usun klienta
int get_best_node(void);    // wybierz najmniej obciazony wezel
int get_corresponding_socket(int sock); // wybierz odpowiadajacy socket, in dla out a out dla in
int repair_node_pool(int node_sock, int out_port);  // odtworz socket po zamknieciu polaczenia 
int is_node_socket(int sock);   // czy socket nalezy do wezla?

#endif // __NODES_H__
