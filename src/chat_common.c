#include "chat_common.h"

int chat_message_make(ChatMessage* msg, enum Type type, const char* value, size_t len)
{
    size_t max = 0;
    char *buffer = NULL;

    if (len != 0) {
        switch (type) {
        case CHAT_MESSAGE_CLIENT_CHANGE_INFO:
            buffer = msg->username;
            max = USERNAME_MAX;
            break;

        case CHAT_MESSAGE_CLIENT_MESSAGE:
            buffer = msg->msg;
            max = MESSAGE_MAX;
            break;

        default:
            break;
        }

        if (len > max)
            return -1;

        memcpy(buffer, value, len);
        buffer[len] = '\0';
    }

    msg->type = type;
    msg->msg_len = len;
    
    return 0;
}

ssize_t chat_message_make_and_send(Socket *socket, ChatMessage* msg, enum Type type, const char* value, size_t len)
{
    chat_message_make(msg, type, value, len);
    return send(socket->fd, msg, sizeof(ChatMessage), 0);
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
    list->count = list->max = 0;
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
