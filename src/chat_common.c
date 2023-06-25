#include "chat_common.h"

ssize_t chat_message_send(Socket *socket, enum Type type, const char* value, size_t len)
{
    size_t max = 0;
    char *buffer = NULL;
    static ChatMessage msg;

    if (len != 0) {
        switch (type) {
        case CHAT_MESSAGE_CLIENT_CHANGE_INFO:
            buffer = msg.username;
            max = USERNAME_MAX;
            break;

        case CHAT_MESSAGE_CLIENT_MESSAGE:
            buffer = msg.msg;
            max = MESSAGE_MAX;
            break;

        default:
            break;
        }

        if (len > max)
            return -2;

        memcpy(buffer, value, len);
        buffer[len] = '\0';
    }

    msg.type = type;
    msg.msg_len = len;

    return send(socket->fd, &msg, sizeof(msg), 0);
}
