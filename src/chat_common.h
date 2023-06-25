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
        CHAT_MESSAGE_SERVER_ACCEPTED,
        CHAT_MESSAGE_SERVER_REFUSED,
        CHAT_MESSAGE_SERVER_BAN,
        CHAT_MESSAGE_SERVER_ENDED,
    } type;
    char username[USERNAME_MAX];
    char msg[MESSAGE_MAX];
} ChatMessage;
