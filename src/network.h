#pragma once

#include "pchheader.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <netdb.h>

#include <fcntl.h>

typedef struct Socket {
    int fd;
    struct sockaddr_in addr;
} Socket;

typedef enum ServerInitStatus {
    SERVER_INIT_SUCESS,
    SERVER_INIT_SOCKET_ERROR,
    SERVER_INIT_BIND_ERROR,
    SERVER_INIT_LISTEN_ERROR,
} ServerInitStatus;

ServerInitStatus init_server(Socket *server,
                             sa_family_t family,
                             int socket_type,
                             int port,
                             in_addr_t in_addr,
                             int max_connections);

bool init_client(Socket *client, sa_family_t family, int type, int port, const char *ip);

void close_socket(Socket *socket);

ssize_t read_socket_message(int fd, void *buffer, ssize_t *bytes_read, size_t size);
