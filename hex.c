#include "hex.h"
#include <stdio.h>

int print_hex(void *ptr, int len)
{
    if(ptr == NULL)
    {
        return -1;
    }
    char *text = (char *)ptr;

    int i;
    for(i = 0; i < len; i++)
    {
        printf("%02x ", text[i]);
    }
    printf("\n");
    return 0;
}

