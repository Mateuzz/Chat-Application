#include "chat_server.h"

int main()
{
    ChatServer *server = chat_server_create(8080);
    if (!server) {
        printf("Erro em criar server\n");
        return 1;
    }

    for (;;) {
        chat_server_update(server);
    }

    chat_server_delete(server);
}
