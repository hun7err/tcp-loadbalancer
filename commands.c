#include "commands.h"
#include <string.h>
#include <stdio.h>

extern int client_sd, sd_in, epoll_fd, config_sd;

char *available_commands[] = { "ADD_NODE", "REMOVE_NODE" };
int command_count = 2;

int check_command(char* string)
{
    int current_command_ind = 0;
    for(; current_command_ind < command_count; current_command_ind++)
    {
        if(strstr(string, available_commands[current_command_ind]) != NULL)
        {
            return 0;
        }
    }
    return -1;
}

int execute_command(char* command)
{
    char *argument = command;
    while(*argument != ' ')
        argument++;
    argument++;

    printf("arg: '%s'\n", argument);

    return 0;
}

