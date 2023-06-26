#pragma once

#include "network.h"

#define MESSAGE_MAX 300
#define USERNAME_MAX 50

#define LOCAL_HOST_IP "127.0.0.1"

typedef struct ChatMessage {
    enum Type {
        CHAT_MESSAGE_CLIENT_CHANGE_INFO,
        CHAT_MESSAGE_CLIENT_MESSAGE,
        CHAT_MESSAGE_CLIENT_END_CONNECTION,
        CHAT_MESSAGE_CLIENT_ALIVE,
        CHAT_MESSAGE_SERVER_ACCEPTED,
        CHAT_MESSAGE_SERVER_REFUSED,
        CHAT_MESSAGE_SERVER_BAN,
        CHAT_MESSAGE_SERVER_ENDED,
        CHAT_MESSAGE_SERVER_CHECK_ALIVE,
    } type;
    char username[USERNAME_MAX + 1];
    char msg[MESSAGE_MAX + 1];
    size_t msg_len;
} ChatMessage;

typedef struct MessageList {
    ChatMessage *messages;
    size_t count;
    size_t max;
} MessageList;

int chat_message_make(ChatMessage *msg, enum Type type, const char* value, size_t len);
ssize_t chat_message_make_and_send(Socket *socket, ChatMessage* msg, enum Type type, const char* value, size_t len);

int message_list_init(MessageList *list, int max_messages);
void message_list_deinit(MessageList *list);
int message_list_add(MessageList* list, const ChatMessage *message);
