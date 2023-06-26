#include "chat_server.h"
#include "chat_common.h"

static bool is_user_banned(ChatServer *chat, ChatClient *cl)
{
    for (int i = 0; i < chat->clients_banned_count; ++i) {
        if (cl->socket.addr.sin_addr.s_addr == chat->banned_clients[i].sin_addr.s_addr) {
            return true;
        }
    }
    return false;
}

static void send_message(ChatServer *chat, ChatMessage *message, ChatClient *client)
{
    send(client->socket.fd, message, sizeof(ChatMessage), 0);

    ChatServerInfo *info = &chat->info.data[chat->info.count];
    info->type = INFO_TYPE_NONE;

    switch (message->type) {
    case CHAT_MESSAGE_CLIENT_END_CONNECTION:
        info->type = INFO_CLIENT_DISCONNECTED;
        break;

    case CHAT_MESSAGE_SERVER_ACCEPTED:
        info->type = INFO_CLIENT_ACCEPTED;
        break;

    case CHAT_MESSAGE_SERVER_REFUSED:
        info->type = INFO_CLIENT_REFUSED;
        break;

    case CHAT_MESSAGE_SERVER_BAN:
        info->type = INFO_CLIENT_BANNED;
        break;

    case CHAT_MESSAGE_SERVER_CHECK_ALIVE:
        info->type = INFO_CLIENT_CHECKING_ALIVE;
        break;
    }

    if (info->type != INFO_TYPE_NONE) {
        ++chat->info.count;
        strcpy(info->data1, client->username);
    }
}

static int update_server_info(ChatServer* chat, enum ServerInfoType type, const char *data1, const char *data2, int flag)
{
    if (chat->info.count == SERVER_INFO_BUFFER_SIZE) {
        return -1;
    }

    ChatServerInfo *info = &chat->info.data[chat->info.count];
    if (data1) {
        strcpy(info->data1, data1);
    }
    if (data2) {
        strcpy(info->data2, data2);
    }
    info->flag = flag;
    info->type = type;

    ++chat->info.count;

    return 0;
}

static void process_client_message(ChatServer* chat, ChatClient *client)
{
    client->status = CLIENT_STATUS_ACTIVE;
    ChatClient *clients = chat->clients;
    int clients_count = chat->clients_count;

    switch (client->message_buffer.type) {
    case CHAT_MESSAGE_CLIENT_CHANGE_INFO: {
        PRINT_DEBUG("Servidor recebeu Message::CLIENT_CHANGE_INFO");
        const char *new_username = client->message_buffer.username;

        for (int i = 0; i < clients_count; ++i) {
            if (strcmp(new_username, clients[i].username) == 0) {
                PRINT_DEBUG(
                    "Server: Requisição negada: %s Tentando mudar para nome do cliente %s\n",
                    client->username,
                    clients[i].username);
                return;
            }
        }

        update_server_info(chat, INFO_CLIENT_CHANGE_NAME, client->username, new_username, 0);

        PRINT_DEBUG("Server: Username do cliente %s mudado para %s\n",
                    client->username,
                    client->message_buffer.username);
        strcpy(client->username, client->message_buffer.username);
    } break;

    case CHAT_MESSAGE_CLIENT_MESSAGE:
        PRINT_DEBUG("Server: %s enviou: %s\n", client->username, client->message_buffer.msg);

        strcpy(client->message_buffer.username, client->username);
        for (int i = 0; i < clients_count; ++i) {
            send_message(chat, &client->message_buffer, &clients[i]);
        }
        message_list_add(&chat->received, &client->message_buffer);
        break;

    case CHAT_MESSAGE_CLIENT_END_CONNECTION:
        PRINT_DEBUG("Server: Cliente %s encerrou conexao\n", client->username);
        client->status = CLIENT_STATUS_INACTIVE;
        break;

    case CHAT_MESSAGE_CLIENT_ALIVE:
        update_server_info(chat, INFO_CLIENT_CONFIRMED_ALIVE, client->username, NULL, 0);
        PRINT_DEBUG("Server: Client %s ainda esta vivo\n", client->username);
        break;

    default:
        break;
    }
}

void chat_server_update(ChatServer *chat)
{
    int fd = chat->socket.fd;
    int clients_count = chat->clients_count;
    ChatClient *clients = chat->clients;
    ChatClient *cl = &chat->clients[clients_count];
    static char buffer[USERNAME_MAX + 1];

    // try accept new client (async)
    if ((cl->socket.fd = accept(fd, (struct sockaddr *)&cl->socket.addr, &chat->addrlen)) > 0) {
        if (is_user_banned(chat, cl)) {
            chat_message_make(&chat->message_buffer, CHAT_MESSAGE_SERVER_REFUSED, NULL, 0);
            send_message(chat, &chat->message_buffer, cl);
        } else {
            sprintf(buffer, "Convidado00%d", clients_count);
            strcpy(cl->username, buffer);
            fcntl(cl->socket.fd, F_SETFL, O_NONBLOCK);
            cl->current_message_bytes_read = 0;
            cl->status = CLIENT_STATUS_ACTIVE;

            chat_message_make( &chat->message_buffer, CHAT_MESSAGE_SERVER_ACCEPTED, NULL, 0);
            send_message(chat, &chat->message_buffer, cl);

            PRINT_DEBUG("Server: Cliente %s adicionado\n", cl->username);

            for (size_t i = 0; i < chat->received.count; ++i) {
                send(cl->socket.fd, &chat->received.messages[i], sizeof(ChatMessage), 0);
            }

            ++chat->clients_count;
        }
    }

    // for each client, try read message or discover if client is up
    for (int i = 0; i < clients_count; ++i) {
        ChatClient *client = &clients[i];
        int error = 0;
        socklen_t len = sizeof(error);
        int retval = getsockopt(client->socket.fd, SOL_SOCKET, SO_ERROR, &error, &len);

        if (retval == 0 && error != 0) {
            PRINT_DEBUG("Server: Não é possivel mandar mensagem para %s, removendo\n",
                    client->username);
            client->status = CLIENT_STATUS_INACTIVE;
        } else if (read_socket_message(client->socket.fd,
                    &client->message_buffer,
                    &client->current_message_bytes_read,
                    sizeof(client->message_buffer)) <= 0) {

            // can't read message from client, start timeout until warning or disconecct
            switch (client->status) {
            case CLIENT_STATUS_ACTIVE:
                client->status = CLIENT_STATUS_NON_RESPONDING;
                client->timeout_start = clock();
                break;

            case CLIENT_STATUS_NON_RESPONDING:
                if (clock() - client->timeout_start >= TIMEOUT_WARN_USER) {
                    PRINT_DEBUG("Server: Client %s deu timeout, mandando aviso\n", client->username);
                    chat_message_make(&chat->message_buffer, CHAT_MESSAGE_SERVER_CHECK_ALIVE, NULL, 0);
                    send_message(chat, &chat->message_buffer, client);
                    client->status = CLIENT_STATUS_TIMEOUT_WAITING;
                }
                break;
            case CLIENT_STATUS_TIMEOUT_WAITING:
                if (clock() - client->timeout_start >= TIMEOUT_BAN_USER) {
                    PRINT_DEBUG("Server: Client %s deu timeout final, banindo\n", client->username);
                    chat_server_ban_user(chat, i);
                }
                break;

            case CLIENT_STATUS_INACTIVE:
                break;
            }

        } else {
            // We get a message from client, process it
            if (client->current_message_bytes_read == sizeof(client->message_buffer)) {
                client->current_message_bytes_read = 0;
                process_client_message(chat, client);
            }
        }
    }

    for (int i = 0; i < clients_count; ++i) {
        if (clients[i].status == CLIENT_STATUS_INACTIVE) {
            update_server_info(chat, INFO_CLIENT_DISCONNECTED, clients[i].username, NULL, 0);
            close_socket(&clients[i].socket);
            clients[i] = clients[clients_count - 1];
            --chat->clients_count;
        }
    }
}

int chat_server_get_user_index(ChatServer *chat, const char *name)
{
    for (int i = 0; i < chat->clients_count; ++i) {
        if (strcmp(name, chat->clients[i].username) == 0) {
            return i;
        }
    }
    return -1;
}

int chat_server_ban_user(ChatServer *chat, int user_index)
{
    ChatClient *cl = &chat->clients[user_index];
    if (user_index >= chat->clients_count) {
        return -1;
    }

    chat_message_make(&chat->message_buffer, CHAT_MESSAGE_SERVER_BAN, NULL, 0);
    send_message(chat, &chat->message_buffer, cl);

    PRINT_DEBUG("Server: Client %s has been banned\n", cl->username);
    cl->status = CLIENT_STATUS_INACTIVE;

    chat->banned_clients[chat->clients_banned_count++] = cl->socket.addr;

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
    chat->port = port;
    chat->info.count = 0;
    chat->clients_banned_count = 0;

    signal(SIGPIPE, SIG_IGN);
    fcntl(chat->socket.fd, F_SETFL, O_NONBLOCK);

    message_list_init(&chat->received, 200);

    PRINT_DEBUG("Server: Server de porta %d criado\n", chat->port);

    return chat;
}

void chat_server_delete(ChatServer *chat)
{
    PRINT_DEBUG("Server: Server de porta %d encerrado\n", chat->port);
    ChatMessage out = {.type = CHAT_MESSAGE_SERVER_ENDED};
    for (int i = 0; i < chat->clients_count; ++i) {
        send(chat->clients[i].socket.fd, &out, sizeof(out), 0);
        close_socket(&chat->clients[i].socket);
    }
    message_list_deinit(&chat->received);
    close_socket(&chat->socket);
    free(chat);
}
