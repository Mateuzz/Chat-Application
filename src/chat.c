#include "chat.h"
#include "message_types.h"
#include "messages.h"
#include "network.h"

#include <stdlib.h>

static bool store_received_message(ChatClient *cc, MessageBuffer msg)
{
    if (cc->received_count == RECEIVED_MESSAGES_STORED_MAX)
        return false;
    size_t size = GET_HEADER(msg)->size + NET_DATA_HEADER_SIZE;
    memcpy(cc->received[cc->received_count++], msg, size);
    return true;
}

ChatClient *chat_client_create(void)
{
    ChatClient *cc = malloc(sizeof(*cc));
    if (!cc)
        return NULL;

    cc->username[0] = '\0';
    cc->status = CHAT_CLIENT_STATUS_NONE;
    cc->received_count = 0;
    socket_init(&cc->socket);

    return cc;
}

bool chat_client_request_join(ChatClient *cc, const char *ip, int port)
{
    if (cc->socket.status != SOCKET_DISCONNECTED)
        return false;

    if (!socket_client(&cc->socket, AF_INET, SOCK_STREAM, port, ip)) {
        cc->status = CHAT_CLIENT_STATUS_REQUEST_FAILED;
        return false;
    }

    strcpy(cc->username, "Unnamed");

    message_reader_init(&cc->reader, &cc->socket);
    cc->status = CHAT_CLIENT_STATUS_WAITING;

    return true;
}

NotifyDisconnectResult chat_client_disconnect(ChatClient *cc, enum ChatClientStatus new_status)
{
    if (cc->socket.status != SOCKET_CLIENT)
        return NOTIFY_ERROR;

    NotifyDisconnectResult result = NOTIFY_SUCESS;
    MessageBuffer out;
    MessageHeader *hd = GET_HEADER(out);

    cc->status = new_status;
    cc->received_count = 0;
    cc->username[0] = '\0';

    if (new_status == CHAT_CLIENT_STATUS_DISCONNECTED) {
        hd->size = 0;
        hd->type = MESSAGE_TYPE_CLIENT_DISCONNECT;

        if (write(cc->socket.fd, hd, NET_DATA_HEADER_SIZE) < 0)
            result = NOTIFY_ERROR;
    }

    socket_disconnect(&cc->socket);

    return result;
}

void chat_client_update(ChatClient *cc)
{
    if (cc->socket.status != SOCKET_CLIENT) 
        return;

    message_reader_process(&cc->reader);

    if (cc->reader.status == STATUS_READING_FINISHED) {
        message_reader_rewind(&cc->reader);
        MessageHeader *hd = GET_HEADER(cc->reader.buff);
        void *body = GET_BODY(cc->reader.buff);

        switch (hd->type) {
        case MESSAGE_TYPE_SERVER_ACCEPT: {
            MessageBodyUsername *msg = body;
            cc->status = CHAT_CLIENT_STATUS_SERVER_ACCEPTED;
            memcpy(cc->username, msg->username, msg->len + 1);
        } break;

        case MESSAGE_TYPE_SERVER_REFUSED:
            chat_client_disconnect(cc, CHAT_CLIENT_STATUS_SERVER_REFUSED);
            break;

        case MESSAGE_TYPE_SERVER_BAN:
            chat_client_disconnect(cc, CHAT_CLIENT_STATUS_SERVER_BANNED);
            break;

        case MESSAGE_TYPE_SERVER_CLIENT_MESSAGE:
            store_received_message(cc, cc->reader.buff);
            break;

        case MESSAGE_TYPE_SERVER_CHANGE_USERNAME: {
            MessageBodyUsername *msg = body;
            memcpy(cc->username, msg->username, msg->len + 1);
        } break;

        case MESSAGE_TYPE_SERVER_CHECK_ALIVE:
            SET_AND_SEND_HEADER(cc->socket.fd, cc->out, MESSAGE_TYPE_CLIENT_ALIVE);
            break;

        case MESSAGE_TYPE_SERVER_ENDED:
            chat_client_disconnect(cc, CHAT_CLIENT_STATUS_DISCONNECTED);
            break;

        default:
            break;
        }
    }
}

void chat_client_delete(ChatClient *cc)
{
    if (!cc)
        return;

    if (cc->socket.status == SOCKET_CLIENT) 
        socket_disconnect(&cc->socket);

    free(cc);
}
