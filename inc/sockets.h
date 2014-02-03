#ifndef __SOCKETS_H__
#define __SOCKETS_H__

int make_socket_nonblocking(int socket);    // ustaw socket jako nieblokujacy
int create_and_bind_sockets(char *iface, int in_port, int config_port); // stworz sockety i zbinduj pod odpowiednie porty
int accept_client(int socket);  // akceptuj klienta na glownym sockecie

#endif // __SOCKETS_H__

