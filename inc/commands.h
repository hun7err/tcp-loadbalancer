#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "packet.h"

int interpret_command(int type, char* command);     // sprawdz i wywolaj polecenie
struct node_list_item_ipv4* get_node_list(void);    // pobierz liste wezlow do wyslania

#endif // __COMMANDS_H__

