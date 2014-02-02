#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "packet.h"

int interpret_command(int type, char* command);
struct node_list_item_ipv4* get_node_list(void);

#endif // __COMMANDS_H__

