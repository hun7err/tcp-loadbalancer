#include "pack.h"
#include <string.h>
#include <stdlib.h>

char* pack(int type, int len, char* msg)
{
    if(msg == NULL)
    {
        return NULL;
    }

    char* packed = calloc(2*sizeof(int)+strlen(msg)+1, 1);
    memcpy(packed, &type, sizeof(int));
    memcpy(packed+sizeof(int), &len, sizeof(int));
    memcpy(packed+2*sizeof(int), msg, strlen(msg)+1);
    return packed;
}

int unpack(char *msg, int *type, int *len, char **body)
{
    if(msg == NULL || type == NULL || len == NULL || *body == NULL)
    {
        return -1;
    }
    memcpy(type, msg, sizeof(int));
    memcpy(len, msg+sizeof(int), sizeof(int));
    *body = calloc(*len, 1);
    memcpy(*body, msg+2*sizeof(int), *len);

    return 0;
}
