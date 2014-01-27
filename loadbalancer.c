#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/epoll.h> // new
#include <sys/types.h> // new
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <signal.h>

#include "fdescriptors.h"
#include "commands.h"
#include "sockets.h"
#include "pack.h"
#include "hex.h"

int sd_in, config_sd, client_sd, epoll_fd;
void sighandler(int signum);

int main(int argc, char ** argv)
{
    if(argc < 3)
    {
        printf("Usage: %s <in port> <in iface> [out port] [config port]\n", argv[0]);
        printf("example: %s 22 eth0 22 1337 will start SSH loadbalancing with configuration on port 1337 TCP\n", argv[0]);
        return -1;
    }

    printf("[ ] Creating sockets for incoming client connections and configuration...\n");

    int in_port = atoi(argv[1]),// port wejsciowy (gdzie udajemy usluge ktorej ruch rozkladamy i przyjmujemy klientow)
        out_port,               // port wyjsciowy dla nas, a wejsciowy dla wezlow (mozemy udawac 22 dla SSH a przekierowywac pod 33 dla wezla)
        config_port = 1337;

    if(argc < 4)    // podane 3 argumenty: nazwa programu, port wejsciowy, interfejs wejsciowy
    {
        out_port = in_port;
    }
    else    // 4 lub wiecej argumentow: podane opcjonalne dane jak port wejsciowy i port do konfiguracji
    {
        out_port = atoi(argv[3]);

        if(argc == 5)
        {
            config_port = atoi(argv[4]);
        }
    }

    signal(SIGINT, sighandler);

    if(create_and_bind_sockets(argv[2], in_port, config_port) == -1)
    {
        cleanup();
        return -1;
    }

    /*if(make_socket_nonblocking(sd_in) == -1)
    {
        cleanup();
        return -1;
    }

    if(make_socket_nonblocking(config_sd) == -1)
    {
        cleanup();
        return -1;
    }*/

    if(listen(sd_in, 100) == -1)
    {
        perror("[!] Could not start listening for client connections");
        cleanup();
        return -1;
    }
    printf("[+] Listening for client connections\n");

    if(listen(config_sd, 1) == -1)
    {
        perror("[!] Could not start listening on the configuration socket\n");
        cleanup();
        return -1;
    }
    printf("[+] Listening for configuration commands\n");

    struct epoll_event event;
    struct epoll_event *events;

    epoll_fd = epoll_create1(0);
    if(epoll_fd == -1)
    {
        perror("[!] Epoll_create1");
        cleanup();
        return -1;
    }

    event.data.fd = sd_in;
    event.events = EPOLLIN | EPOLLET;

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sd_in, &event) == -1)
    {
        perror("[!] epoll_ctl sd_in");
        cleanup();
        return -1;
    }

    event.data.fd = config_sd;

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, config_sd, &event) == -1)
    {
        perror("[!] epoll_ctl config_sd");
        cleanup();
        return -1;
    }

    events = calloc(64, sizeof(event));
    client_sd = -1;

    while(1)
    {
        static int current_event, event_count;

        event_count = epoll_wait(epoll_fd, events, 64, -1);
        for(current_event = 0; current_event < event_count; current_event++)
        {
            if((events[current_event].events & EPOLLERR) ||
                    (events[current_event].events & EPOLLHUP) ||
                    (!(events[current_event].events & EPOLLIN)))
            {
                fprintf(stderr, "[!] Epoll error\n");
                close(events[current_event].data.fd);
                continue;
            }
            else if (sd_in == events[current_event].data.fd) // przychodzace polaczenie do przekierowania
            {
                // debug
                printf("Data on main socket\n");
            }
            else if(sd_in == events[current_event].data.fd)
            {
                /*struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);

                static int new_client_sd;
                new_client_sd = accept(sd_in, (struct sockaddr*)&client_addr, &client_addr_len);

                if(new_client_sd == -1)
                {
                    perror("[!] Could not accept client connection");
                }

                char address_string[INET_ADDRSTRLEN];
                memset(address_string, 0, INET_ADDRSTRLEN);

                inet_ntop(AF_INET, &(client_addr.sin_addr), address_string, INET_ADDRSTRLEN);
                printf("[+] Accepted new client: %s:%d is now on sd %d\n", address_string, ntohs(client_addr.sin_port), new_client_sd);

                if(make_socket_nonblocking(new_client_sd) == -1)
                {
                    printf("[!] Closing client connection\n");
                    close(new_client_sd);
                }

                event.data.fd = new_client_sd;
                event.events = EPOLLIN | EPOLLET;

                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_client_sd, &event) == -1)
                {
                    perror("[!] epoll_ctl");
                    close(new_client_sd);
                }*/

                // tutaj wybierz odpowiednia maszyne do przydzielenia dla polaczenia i tam przypisz fd
            }
            else if (config_sd == events[current_event].data.fd) // podlaczony klient do konfiguracji
            {
                while(client_sd == -1)  // tylko jeden klient w danym momencie moze odpowiadac za konfiguracje
                {
                    printf("[ ] Incoming client connection\n");
                    struct sockaddr_in client_addr;
                    socklen_t client_addr_len;

                    client_addr_len = sizeof(client_addr);
                    client_sd = accept(config_sd, (struct sockaddr*)&client_addr, &client_addr_len);

                    if(client_sd == -1)
                    {
                        if((errno == EAGAIN) || (errno == EWOULDBLOCK))
                        {
                            break;
                        }
                        perror("[!] Could not accept connection");
                    } 
                    char address_string[INET_ADDRSTRLEN];
                    memset(address_string, 0, INET_ADDRSTRLEN);

                    inet_ntop(AF_INET, &(client_addr.sin_addr), address_string, INET_ADDRSTRLEN); 

                    printf("[+] Accepted config. client %s:%d on socket %d\n", address_string, ntohs(client_addr.sin_port), client_sd);

                    if(make_socket_nonblocking(client_sd) == -1)
                    {
                        printf("[!] Could not make the config. client socket nonblocking, closing connection\n");
                        close(client_sd);
                        client_sd = -1;
                    }

                    event.data.fd = client_sd;
                    event.events = EPOLLIN | EPOLLET;

                    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sd, &event) == -1)
                    {
                        perror("[!] epoll_ctl");
                        close(client_sd);
                        client_sd = -1;
                    }
                }
            }

            if(client_sd == events[current_event].data.fd && client_sd != -1)
            {
               char buf[128];
               static int data_len, count;

               ioctl(client_sd, FIONREAD, &count);

               if(count > 128)
               {
                   char msg[] = "Error 001: Too much data sent";
                   send(client_sd, msg, strlen(msg), 0);
                   printf("[!] Error 001 on client\n");
                   close(client_sd);
                   client_sd = -1;
               }

               errno = 0;
               data_len = recv(client_sd, buf, count, 0); // zmienic na 64

               if(data_len < 0)
               {
                   perror("[!] Error receiving data");

                   close(client_sd);
                   client_sd = -1;
               }
               else if (data_len == 0)
               {
                   printf("[i] Client disconnected\n");
                   close(client_sd);
                   client_sd = -1;
               }
               else // prawidlowo odebrane dane
               {
                   if(check_command(buf) != -1)
                   {
                       printf("[+] Valid command sent\n");
                       execute_command(buf);
                   }
               }
            }
            else
            {
            // ruch ktory mamy rownowazyc: klient lub serwer
            }
        }

    }

    return 0;
}

void sighandler(int signum)
{
    if(signum == SIGINT)
    {
        printf(" [!] SIGINT caught, quitting...\n");
        cleanup();
        exit(0);
    }
}
