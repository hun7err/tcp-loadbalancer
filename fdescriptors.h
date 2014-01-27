#ifndef __FDESCRIPTORS_H__
#define __FDESCRIPTORS_H__

extern int sd_in,
            config_sd,
            client_sd,
            epoll_fd;


void cleanup(void);
void check_incoming_config_data(int len);
int make_socket_nonblocking(int socket);

#endif // __FDESCRIPTORS_H__

