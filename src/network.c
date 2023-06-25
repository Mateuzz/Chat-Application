#include "network.h"

ServerInitStatus init_server(Socket *server,
                             sa_family_t family,
                             int type,
                             int port,
                             in_addr_t in_addr,
                             int max_connections)
{
    int fd;
    struct sockaddr_in addr;

    if ((fd = socket(family, type, 0)) < 0) {
        return SERVER_INIT_SOCKET_ERROR;
    }

    bzero(&addr, sizeof(addr));
    addr.sin_family = family;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = in_addr;

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return SERVER_INIT_BIND_ERROR;
    }

    if (listen(fd, max_connections) < 0) {
        close(fd);
        return SERVER_INIT_LISTEN_ERROR;
    }

    server->addr = addr;
    server->fd = fd;

    return SERVER_INIT_SUCESS;
}

bool init_client(Socket *client, sa_family_t family, int type, int port, const char *ip)
{
    int fd;
    struct sockaddr_in addr = {
        .sin_family = family,
        .sin_port = htons(port),
        .sin_zero = {0},
    };

    if ((fd = socket(family, type, 0)) < 0) {
        return false;
    }

    if (inet_pton(family, ip, &addr.sin_addr) < 0) {
        return false;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        return false;
    }

    client->fd = fd;
    client->addr = addr;

    return true;
}

void close_socket(Socket *socket)
{
    close(socket->fd);
}

ssize_t read_socket_message(int fd, void *buffer, ssize_t *bytes_read, size_t size)
{
    uint8_t *read_pos = (uint8_t *)buffer + *bytes_read;
    ssize_t n = read(fd, read_pos, size - *bytes_read);
    if (n > 0)
        *bytes_read += n;
    return n;
}

