#ifndef __FDESCRIPTORS_H__
#define __FDESCRIPTORS_H__

extern int sd_in,
            config_sd,
            client_sd,
            epoll_fd;


void cleanup(void); // posprzataj deskryptory
void check_incoming_config_data(int len);   // sprawdz przychodzace dane, jak niepoprawne to zakoncz polaczenie

#endif // __FDESCRIPTORS_H__

