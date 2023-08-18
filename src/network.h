#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#define RECEIVED_MESSAGES_STORED_MAX 200

typedef struct Socket {
    int fd;
    struct sockaddr_in addr;
    enum SocketStatus {
        SOCKET_DISCONNECTED,
        SOCKET_CLIENT,
        SOCKET_SERVER,
    } status;
} Socket;

void socket_init(Socket *sock);

bool socket_client(Socket *sock, sa_family_t family, int type, int port, const char *ip);

bool socket_server(Socket *sock,
                   sa_family_t family,
                   int type,
                   int port,
                   in_addr_t in_addr,
                   int max_connections);

Socket socket_accept(Socket *server);

bool socket_disconnect(Socket *sock);
