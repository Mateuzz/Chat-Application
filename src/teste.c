#include "network.h"

int main()
{
    Socket socket;
    init_server(&socket, AF_INET, SOCK_STREAM, 8080, INADDR_ANY, 10);

    struct sockaddr addr;
    socklen_t len = sizeof(addr);

    getsockname(socket.fd, &addr, &len);
    printf("Socket addr = %s\n", addr.sa_family);
}
