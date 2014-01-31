#ifndef __SOCKETS_H__
#define __SOCKETS_H__

int make_socket_nonblocking(int socket);
int create_and_bind_sockets(char *iface, int in_port, int config_port);
int accept_client(void);

#endif // __SOCKETS_H__

