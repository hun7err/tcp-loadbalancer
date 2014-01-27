#ifndef __PACK_H__
#define __PACK_H__

char* pack(int type, int len, char* msg);
int unpack(char *msg, int *type, int *len, char **body);

#endif // __PACK_H__

