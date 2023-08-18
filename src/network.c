#include "network.h"
#include <assert.h>

void socket_init(Socket *sock)
{
    sock->status = SOCKET_DISCONNECTED;
}

bool socket_client(Socket *sock, sa_family_t family, int type, int port, const char *ip)
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

    sock->fd = fd;
    sock->status = SOCKET_CLIENT;
    sock->addr = addr;

    fcntl(fd, F_SETFL, O_NONBLOCK);

    return true;
}

bool socket_server(Socket *sock,
                   sa_family_t family,
                   int type,
                   int port,
                   in_addr_t in_addr,
                   int max_connections)
{
    int fd;
    struct sockaddr_in addr;

    if ((fd = socket(family, type, 0)) < 0) {
        return false;
    }

    bzero(&addr, sizeof(addr));
    addr.sin_family = family;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = in_addr;

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return false;
    }

    if (listen(fd, max_connections) < 0) {
        close(fd);
        return false;
    }

    sock->addr = addr;
    sock->fd = fd;
    sock->status = SOCKET_SERVER;

    fcntl(fd, F_SETFL, O_NONBLOCK);

    return true;
}

Socket socket_accept(Socket *server)
{
    assert(server->status == SOCKET_SERVER);

    static socklen_t addrlen = sizeof(struct sockaddr_in);
    Socket socket;

    if ((socket.fd = accept(server->fd, (struct sockaddr *)&socket.addr, &addrlen)) > 0) { 
        socket.status = SOCKET_CLIENT;
        fcntl(socket.fd, F_SETFL, O_NONBLOCK);
    }
    return socket;
}

bool socket_disconnect(Socket *sock)
{
    assert(sock->status != SOCKET_DISCONNECTED);

    sock->status = SOCKET_DISCONNECTED;
    close(sock->fd);
    return true;
}
