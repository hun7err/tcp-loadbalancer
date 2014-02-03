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
#include "packet.h"
#include "nodes.h"
#include "pack.h"
#include "hex.h"

int sd_in,      // socket for accepting incoming client connections
    config_sd,  // listening socket descriptor for accepting config. clients
    client_sd,  // configuration client socket descriptor
    epoll_fd;   // epoll main file descriptor

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

    if(create_and_bind_sockets(argv[2], in_port, config_port) == -1)    // utworz sockety zbinduj pod odpowiednie porty
    {
        cleanup();
        return -1;
    }

    if(listen(sd_in, 100) == -1)    // zakladamy wielkosc backloga = 100
    {
        perror("[!] Could not start listening for client connections");
        cleanup();
        return -1;
    }
    printf("[+] Listening for client connections\n");

    if(listen(config_sd, 1) == -1)  // tylko 1 klient moze oczekiwac w kolejce
    {
        perror("[!] Could not start listening on the configuration socket\n");
        cleanup();
        return -1;
    }
    printf("[+] Listening for configuration commands\n");

    struct epoll_event event;   // do dodawania fd do epolla
    struct epoll_event *events;

    epoll_fd = epoll_create1(0);    // utworz glowny file descriptor epolla
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

    // zakladamy przetrzymywanie 64 eventow w epollu
    events = calloc(64, sizeof(event));
    client_sd = -1;

    initialize_nodes();

    // isengard, mordor, khazad-dum
    char *nodes_to_add[] = {"192.168.122.70", "192.168.122.46", "192.168.122.32"};
    int cur_node;
    // dodajemy wezly
    for(cur_node = 0; cur_node < 3; cur_node++)
    {
        if(add_new_node(nodes_to_add[cur_node], out_port) == -1)
        {
            perror("[!] Could not add node");
            cleanup();
            return -1;
        }
    }

    while(1)
    {
        static int current_event, event_count;

        errno = 0;
        // czekaj na wydarzenie na dowolnym z obserwowanych deskryptorow
        event_count = epoll_wait(epoll_fd, events, 64, -1);
        for(current_event = 0; current_event < event_count; current_event++)
        {
            if((events[current_event].events & EPOLLERR) ||
                    (events[current_event].events & EPOLLHUP) ||
                    (!(events[current_event].events & EPOLLIN)))    // w skrocie: blad epolla, np. brak danych mimo ze zostalismy powiadomieni o ich obecnosci
            {
                perror("Epoll error");
                close(events[current_event].data.fd);
                continue;
            }
            else if(sd_in == events[current_event].data.fd)
            {
                // na glownym sockecie nasluchujacym mamy dane, czyli ktos chce sie podlaczyc
                printf("[+] Incoming client connection\n");
                int socket = accept_client(sd_in);
                if(socket != -1)
                {
                    event.data.fd = socket;
                    // czekamy na dane na wejsciu, edge-triggered (wyzwalane zboczem)
                    event.events = EPOLLIN | EPOLLET;

                    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket, &event) == -1)
                    {
                        perror("[!] epoll_ctl");
                        close(socket);
                    }

                    if(add_new_client(socket, out_port) == -1)
                    {
                        printf("[!] Could not add client! Closing connection...");
                        close(socket);
                    }
                }
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

                    // pobierz ze struktury do stringa (address_string) ip klienta
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
                struct header h;    // naglowek pakietu
                int len = recv(client_sd, (char*)&h, sizeof(struct header), 0);

                if(len < 0)
                {
                    printf("[!] Reading error on client; closing connection...\n");
                    close(client_sd);
                    client_sd = -1;
                }
                else
                {
                    if(len == 0)
                    {
                        printf("[ ] Conf. client disconnected.\n");
                        close(client_sd);
                        client_sd = -1;
                    }
                    else
                    {
                        char* packet = calloc(h.length, 1);
                        len = recv(client_sd, packet, h.length, 0);
                        if(interpret_command(h.type, packet) == -1)
                        {
                            printf("[!] Unrecognized command from the conf. client\n");
                        }
                        free(packet);
                    }
                }
            }
            else
            {
                int in_sock = events[current_event].data.fd,
                    n_in,
                    n_out;

                int out_sock = get_corresponding_socket(in_sock);

                if(out_sock != -1)
                {
                    char buf[128];
                    while((n_in = recv(in_sock, buf, 128, 0)) > 0)
                    {
                        n_out = send(out_sock, buf, n_in, 0);
                        if(n_out == -1)
                        {
                            perror("[!] Could not send the received data");
                            printf("Closing connection with %d\n", out_sock);
                            close(out_sock);
                        }
                    }

                    if(n_in == 0 && is_node_socket(in_sock) == -1) // klient rozlaczyl sie...
                    {
                        printf("[ ] Client on sd %d disconnected\n", in_sock);
                        remove_client(in_sock);
                        remove_nodes();
                        // usun klienta
                    }
                    else if (n_in == 0 && is_node_socket(in_sock) != -1)    // ...a wraz z klientem padl socket wezla wiec musimy go odtworzyc
                    {
                        repair_node_pool(in_sock, out_port);
                    }
                }
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
