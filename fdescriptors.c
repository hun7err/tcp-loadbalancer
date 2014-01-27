#include "fdescriptors.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void cleanup(void)
{
    close(client_sd);
    close(config_sd);
    close(sd_in);
    close(epoll_fd);
}

void check_incoming_config_data(int len)
{
    if(len <= 0)
    {
        if(len < 0)
        {
            perror("[!] Error on client connection");
        }
        else
        {
            printf("[i] Client disconnected\n");
        }
        close(client_sd);
        client_sd = -1;
    }
}

