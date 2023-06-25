#include "chat_server.h"
#include "chat_common.h"

static ChatMessage g_message_out;

static void
process_client_message(ChatClient *client, ChatClient *clients, int clients_count)
{
    client->status = CLIENT_STATUS_ACTIVE;

    switch (client->message_buffer.type) {
    case CHAT_MESSAGE_CLIENT_CHANGE_INFO:
        for (int i = 0; i < clients_count; ++i)
            if (strcmp(client->message_buffer.username, clients[i].username) == 0) {
                PRINT_DEBUG("Server: Requisição negada: %s Tentando mudar para nome do cliente %s\n", client->username, clients[i].username);
            }

        PRINT_DEBUG("Server: Username do cliente %s mudado para %s\n", client->username, client->message_buffer.username);
        strcpy(client->username, client->message_buffer.username);
        break;

    case CHAT_MESSAGE_CLIENT_MESSAGE:
        PRINT_DEBUG("Server: %s enviou: %s\n",
                    client->username,
                    client->message_buffer.msg);

        strcpy(client->message_buffer.username, client->username);
        for (int i = 0; i < clients_count; ++i) {
            send(clients[i].fd, &client->message_buffer, sizeof(client->message_buffer), 0);
        }
        break;

    case CHAT_MESSAGE_CLIENT_END_CONNECTION:
        PRINT_DEBUG("Server: Cliente %s encerrou conexao\n", client->username);
        client->status = CLIENT_STATUS_INACTIVE;
        break;

    case CHAT_MESSAGE_CLIENT_ALIVE:
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

    // try accept new client (async)
    if ((cl->fd = accept(fd, (struct sockaddr *)&cl->addr, &chat->addrlen)) > 0) {
        g_message_out.type = CHAT_MESSAGE_SERVER_ACCEPTED;
        sprintf(g_message_out.username, "Convidado00%d", clients_count);
        send(cl->fd, &g_message_out, sizeof(ChatMessage), 0);

        fcntl(cl->fd, F_SETFL, O_NONBLOCK);
        cl->current_message_bytes_read = 0;
        cl->status = CLIENT_STATUS_ACTIVE;
        strcpy(cl->username, g_message_out.username);
        ++chat->clients_count;

        PRINT_DEBUG("Server: Cliente %s adicionado\n", cl->username);
    }

    // for each client, try read message or discover if client is up
    for (int i = 0; i < clients_count; ++i) {
        ChatClient *client = &clients[i];
        int error = 0;
        socklen_t len = sizeof(error);
        int retval = getsockopt(client->fd, SOL_SOCKET, SO_ERROR, &error, &len);

        if (retval == 0 && error != 0) {
            PRINT_DEBUG("Server: Não é possivel mandar mensagem para %s, removendo\n", client->username);
            client->status = CLIENT_STATUS_INACTIVE;
        } else if (read_socket_message(client->fd,
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
                    g_message_out.type = CHAT_MESSAGE_SERVER_CHECK_ALIVE;
                    send(client->fd, &g_message_out, sizeof(ChatMessage), 0);
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
                process_client_message(client, clients, clients_count);
                message_list_add(&chat->received, &client->message_buffer);
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
    if (user_index >= chat->clients_count) {
        return -1;
    }

    g_message_out.type = CHAT_MESSAGE_SERVER_BAN;
    send(chat->clients[user_index].fd, &g_message_out, sizeof(ChatMessage), 0);

    PRINT_DEBUG("Server: Client %s has been banned\n", chat->clients[user_index].username);
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
    chat->port = port;

    signal(SIGPIPE, SIG_IGN);
    fcntl(chat->socket.fd, F_SETFL, O_NONBLOCK);

    message_list_init(&chat->received, 200);

    PRINT_DEBUG("Server: Server de porta %d criado\n", chat->port);

    return chat;
}

void chat_server_delete(ChatServer *chat)
{
    PRINT_DEBUG("Server: Server de porta %d encerrado\n", chat->port);
    ChatMessage out = { .type = CHAT_MESSAGE_SERVER_ENDED };
    for (int i = 0; i < chat->clients_count; ++i) {
        send(chat->clients[i].fd, &out, sizeof(out), 0);
    }
    message_list_deinit(&chat->received);
    close_socket(&chat->socket);
    free(chat);
}

int message_list_init(MessageList *list, int max_messages)
{
    if (!(list->messages = malloc(sizeof(ChatMessage) * max_messages)))
        return -1;
    list->max = max_messages;
    list->count = 0;
    return 0;
}

void message_list_deinit(MessageList *list)
{
    free(list->messages);
    list->messages = NULL;
}

int message_list_add(MessageList *list, const ChatMessage *message)
{
    if (list->count > list->max) {
        list->max *= 2;
        if (!(list->messages = realloc(list->messages, sizeof(ChatMessage) * list->max))) {
            return -1;
        }
    }

    list->messages[list->count++] = *message;

    return 0;
}
