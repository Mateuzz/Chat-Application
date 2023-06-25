#include "chat_server.h"

typedef enum ClientMessageResult {
    CHAT_MESSAGE_CLIENT_MESSAGE_RESULT_OK,
    CHAT_MESSAGE_CLIENT_MESSAGE_RESULT_REPEATED_NAME,
    CHAT_MESSAGE_CLIENT_MESSAGE_RESULT_ENDED,
} ClientMessageResult;

static ClientMessageResult
process_client_message(ChatClient *client, ChatClient *clients, int clients_count)
{
    switch (client->message_buffer.type) {
    case CHAT_MESSAGE_CLIENT_CHANGE_INFO:
        for (int i = 0; i < clients_count; ++i)
            if (strcmp(client->message_buffer.username, clients[i].username) == 0) {
                PRINT_DEBUG("Requisição negada: %s Tentando mudar para nome do cliente %s\n", client->username, clients[i].username);
                return CHAT_MESSAGE_CLIENT_MESSAGE_RESULT_REPEATED_NAME;
            }

        PRINT_DEBUG("uSername do cliente %s mudado para %s\n", client->username, client->message_buffer.username);
        strcpy(client->username, client->message_buffer.username);
        return CHAT_MESSAGE_CLIENT_MESSAGE_RESULT_OK;

    case CHAT_MESSAGE_CLIENT_MESSAGE:
        PRINT_DEBUG("%s enviou: %s\n",
                    client->username,
                    client->message_buffer.msg);
        strcpy(client->message_buffer.username, client->username);
        for (int i = 0; i < clients_count; ++i) {
            send(clients[i].fd, &client->message_buffer, sizeof(client->message_buffer), 0);
        }
        return CHAT_MESSAGE_CLIENT_MESSAGE_RESULT_OK;

    case CHAT_MESSAGE_CLIENT_END_CONNECTION:
        PRINT_DEBUG("Cliente %s encerrou conexao\n", client->username);
        client->status = CLIENT_STATUS_INACTIVE;
        return CHAT_MESSAGE_CLIENT_MESSAGE_RESULT_OK;

    default:
        exit(1);
        break;
    }
}

void chat_server_update(ChatServer *chat)
{
    int fd = chat->socket.fd;
    int clients_count = chat->clients_count;
    ChatClient *clients = chat->clients;
    ChatClient *cl = &chat->clients[clients_count];

    // try accept new client (async)
    if ((cl->fd = accept(fd, (struct sockaddr *)&cl->addr, &chat->addrlen)) > 0) {
        ChatMessage message = {.type = CHAT_MESSAGE_SERVER_ACCEPTED};
        sprintf(message.username, "Convidado00%d", clients_count);
        send(cl->fd, &message, sizeof(message), 0);

        fcntl(cl->fd, F_SETFL, O_NONBLOCK);
        cl->current_message_bytes_read = 0;
        cl->non_responsing = false;
        cl->status = CLIENT_STATUS_ACTIVE;
        strcpy(cl->username, message.username);
        ++chat->clients_count;

        PRINT_DEBUG("Cliente %s adicionado\n", cl->username);
    }

    // for each client, try read message or discover if client is up
    for (int i = 0; i < clients_count; ++i) {
        ChatClient *client = &clients[i];
        int error = 0;
        socklen_t len = sizeof(error);
        int retval = getsockopt(client->fd, SOL_SOCKET, SO_ERROR, &error, &len);

        if (retval == 0 && error != 0) {
            PRINT_DEBUG("Não é possivel mandar mensagem para %s, removendo\n", client->username);
            client->status = CLIENT_STATUS_INACTIVE;
        } else if (read_socket_message(client->fd,
                                       &client->message_buffer,
                                       &client->current_message_bytes_read,
                                       sizeof(client->message_buffer)) <= 0) {

            // Cant read from client, start timeout until closing socket
            if (!client->non_responsing) {
                client->non_responsing = true;
                client->timeout_start = clock();
            } else if (clock() - client->timeout_start >= TIMEOUT_MAX) {
                PRINT_DEBUG("Client %s deu timeout\n", client->username);
                client->status = CLIENT_STATUS_INACTIVE;
            }
        } else {
            // We get a message from client, process it
            client->non_responsing = false;
            if (client->current_message_bytes_read == sizeof(client->message_buffer)) {
                client->current_message_bytes_read = 0;
                process_client_message(client, clients, clients_count);
            }
        }
    }

    for (int i = 0; i < clients_count; ++i) {
        if (clients[i].status == CLIENT_STATUS_INACTIVE) {
            clients[i] = clients[clients_count - 1];
            --chat->clients_count;
        }
    }
}

int chat_server_get_user_index(ChatServer* chat, const char *name)
{
    for (int i = 0; i < chat->clients_count; ++i) {
        if (strcmp(name, chat->clients[i].username) == 0) {
            return i;
        }
    }
    return -1;
}

int chat_server_ban_user(ChatServer* chat, int user_index)
{
    if (user_index < chat->clients_count) {
        return -1;
    }

    ChatMessage out = { .type = CHAT_MESSAGE_SERVER_BAN };
    send(chat->clients[user_index].fd, &out, sizeof(out), 0);
    
    PRINT_DEBUG("Client %s has been banned\n", chat->clients[user_index].username);
    chat->clients[user_index].status = CLIENT_STATUS_INACTIVE;

    return 0;
}

ChatServer *chat_server_create(int port)
{
    ChatServer *chat = malloc(sizeof(ChatServer));
    if (!chat ||
        init_server(&chat->socket, AF_INET, SOCK_STREAM, port, INADDR_ANY, MAX_CONNECTIONS) !=
            SERVER_INIT_SUCESS) {
        free(chat);
        return NULL;
    }

    chat->addrlen = sizeof(struct sockaddr_in);
    chat->clients_count = 0;

    signal(SIGPIPE, SIG_IGN);
    fcntl(chat->socket.fd, F_SETFL, O_NONBLOCK);

    return chat;
}

void chat_server_delete(ChatServer *chat)
{
    ChatMessage out = { .type = CHAT_MESSAGE_SERVER_ENDED };
    for (int i = 0; i < chat->clients_count; ++i) {
        send(chat->clients[i].fd, &out, sizeof(out), 0);
    }
    close_socket(&chat->socket);
    free(chat);
}
