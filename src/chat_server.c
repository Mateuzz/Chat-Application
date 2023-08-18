#include "chat_server.h"
#include "chat.h"
#include "common.h"
#include "message_types.h"
#include "messages.h"
#include "network.h"

#include <signal.h>
#include <stdlib.h>

static int update_info(ChatServer *chat, const char* format, ...)
{
    if (chat->info.count == SERVER_INFO_MAX)
        return -1;

    va_list ap;
    va_start(ap, format);
    int n = vsprintf(chat->info.data[chat->info.count++], format, ap);
    va_end(ap);

    return n;
}

static bool is_banned(ChatServer *chat, ChatServerClient *cl)
{
    for (int i = 0; i < chat->banned_count; ++i) {
        if (cl->socket.addr.sin_addr.s_addr == chat->banned_clients[i].sin_addr.s_addr) {
            return true;
        }
    }
    return false;
}

static void process_client_message(ChatServer *chat, ChatServerClient *cl)
{
    void *msg = cl->reader.buff;
    ChatServerClient *clients = chat->clients;
    int clients_count = chat->clients_count;
    MessageHeader *hd = GET_HEADER(msg);
    void *body = GET_BODY(msg);

    cl->status = CLIENT_STATUS_ACTIVE;

    switch (hd->type) {
    case MESSAGE_TYPE_CLIENT_DISCONNECT:
        cl->status = CLIENT_STATUS_INACTIVE;
        break;

    case MESSAGE_TYPE_CLIENT_REQUEST_USERNAME: {
        MessageBodyUsername *msg_body = body;

        msg_body->len = MIN(USERNAME_MAX, msg_body->len);
        msg_body->username[msg_body->len] = '\0';

        for (int i = 0; i < clients_count; ++i) {
            if (strcmp(msg_body->username, clients[i].username) == 0) {
                return;
            }
        }

        update_info(chat, "Client %s changed username to %s", cl->username, msg_body->username);

        memcpy(cl->username, msg_body->username, msg_body->len + 1);

        hd->type = MESSAGE_TYPE_SERVER_CHANGE_USERNAME;
        hd->size = sizeof(MessageBodyUsername);

        SEND_MESSAGE(cl->socket.fd, msg);
    } break;

    case MESSAGE_TYPE_CLIENT_MSG: {
        MessageBodyMessage *msg_body = body;
        msg_body->content_len = MIN(MESSAGE_CONTENT_MAX, msg_body->content_len);
        msg_body->content[msg_body->content_len] = '\0';

        size_t username_len = strlen(cl->username);
        memcpy(msg_body->username, cl->username, username_len + 1);
        msg_body->user_len = username_len;

        hd->size = sizeof(MessageBodyMessage);
        hd->type = MESSAGE_TYPE_SERVER_CLIENT_MESSAGE;

        for (int i = 0; i < clients_count; ++i)
            SEND_MESSAGE(clients[i].socket.fd, msg);

        if (chat->received_count < RECEIVED_MESSAGES_STORED_MAX)
            memcpy(chat->received[chat->received_count++], msg, NET_DATA_HEADER_SIZE + hd->size);

    } break;

    case MESSAGE_TYPE_CLIENT_ALIVE:
        update_info(chat, "Client %s confirmed alive", cl->username);
        break;

    case MESSAGE_TYPE_CLIENT_END_CONNECTION:
        cl->status = CLIENT_STATUS_INACTIVE;
        break;
    }
}

ChatServer *chat_server_create(int port)
{
    ChatServer *chat = malloc(sizeof(ChatServer));
    if (!chat ||
            !socket_server(&chat->socket, AF_INET, SOCK_STREAM, port, INADDR_ANY, MAX_CONNECTIONS)) {
        free(chat);
        return NULL;
    }

    chat->clients_count = 0;
    chat->info.count = 0;
    chat->banned_count = 0;
    chat->received_count = 0;

    signal(SIGPIPE, SIG_IGN);
    fcntl(chat->socket.fd, F_SETFL, O_NONBLOCK);

    return chat;
}

void chat_server_update(ChatServer *chat)
{
    int clients_count = chat->clients_count;
    ChatServerClient *clients = chat->clients;
    ChatServerClient *new_client = &chat->clients[clients_count];

    new_client->socket = socket_accept(&chat->socket);
    if (new_client->socket.status == SOCKET_CLIENT) {
        if (is_banned(chat, new_client)) {
            SET_AND_SEND_HEADER(new_client->socket.fd, chat->out, MESSAGE_TYPE_SERVER_REFUSED);
            socket_disconnect(&new_client->socket);
        } else {
            sprintf(new_client->username, "Convidado%03d", clients_count);
            message_reader_init(&new_client->reader, &new_client->socket);

            size_t user_len = strlen(new_client->username);
            MessageBodyUsername *body = GET_BODY(chat->out);

            SET_HEADER(chat->out, MESSAGE_TYPE_SERVER_ACCEPT, sizeof(MessageBodyUsername));
            memcpy(body->username, new_client->username, user_len + 1);
            body->len = user_len;

            SEND_MESSAGE(new_client->socket.fd, chat->out);

            update_info(chat ,"Client %s connected", new_client->username);

            for (int i = 0; i < chat->received_count; ++i) {
                SEND_MESSAGE(new_client->socket.fd, chat->received[i]);
            }

            ++chat->clients_count;
        }
    }

    for (int i = 0; i < chat->clients_count; ++i) {
        ChatServerClient *client = &clients[i];
        int error = 0;
        socklen_t len = sizeof(error);
        int retval = getsockopt(client->socket.fd, SOL_SOCKET, SO_ERROR, &error, &len);

        if (retval == 0 && error != 0) {
            client->status = CLIENT_STATUS_INACTIVE;
        } else {
            message_reader_process(&client->reader);
            if (client->reader.status == STATUS_READING_FINISHED) {
                message_reader_rewind(&client->reader);
                process_client_message(chat, client);
            }
        }
    }

    for (int i = 0; i < clients_count; ++i) {
        if (clients[i].status == CLIENT_STATUS_INACTIVE) {
            update_info(chat, "Client %s was disconnected", clients[i].username);
            socket_disconnect(&clients[i].socket);
            clients[i] = clients[clients_count - 1];
            --chat->clients_count;
        }
    }
}

void chat_server_delete(ChatServer *chat)
{
    if (!chat)
        return;

    SET_HEADER(chat->out, MESSAGE_TYPE_SERVER_ENDED, 0);

    for (int i = 0; i < chat->clients_count; ++i) {
        SEND_MESSAGE(chat->clients[i].socket.fd, chat->out);
        socket_disconnect(&chat->clients[i].socket);
    }

    free(chat);
}

bool chat_server_ban_user(ChatServer *chat, int index)
{
    if (index >= chat->clients_count)
        return false;

    ChatServerClient *cl = &chat->clients[index];

    SET_AND_SEND_HEADER(cl->socket.fd, chat->out, MESSAGE_TYPE_SERVER_BAN);

    update_info(chat, "Client %s was banned", cl->username);

    cl->status = CLIENT_STATUS_INACTIVE;
    chat->banned_clients[chat->banned_count++] = cl->socket.addr;

    return true;
}
