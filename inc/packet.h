#ifndef __PACKET_H__
#define __PACKET_H__

#define COMMAND_NOT_IMPLEMENTED 0x00    // komenda nie jest obslugiwana
#define ADD_NODE    0x01                // dodaj wezel
#define LIST_NODES  0x02                // listuj wezly
#define LIST_NODES_RESPONSE 0x03        // odpowiedz z lista wezlow
#define REMOVE_NODE 0x04                // usun wezel

struct header { // naglowek pakietu
    int type;       // typ pakietu
    int length;     // dlugosc ciala pakietu
} __attribute__ ((packed));

struct node_list_header {   // naglowek listy wezlow
    int host_count;     // liczba wezlow ktore beda przeslane
    int pool_size;      // wielkosc connection pool (globalna)
    int ip_version;     // wersja protokolu IP
} __attribute__ ((packed));

struct node_list_item_ipv4 {    // informacje o wezle ipv4
    char address[16];       // adres
    int connection_count;   // liczba otwartych polaczen
    int pool_usage;         // zuzycie connection pool
} __attribute__ ((packed));

char* node_list_packet(int host_count, int pool_size, int ip_version, struct node_list_item_ipv4* nodes);
int node_list_packet_size(int host_count);

struct node_to_add_ipv4 {   // wezel ipv4 do dodania
    char address[16];
    int port;
} __attribute__ ((packed));

struct node_to_remove_ipv4 {    // wezel ipv4 do usuniecia
    char address[16];
} __attribute__ ((packed));

#endif // __PACKET_H__
