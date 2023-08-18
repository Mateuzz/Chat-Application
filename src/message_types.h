#pragma once

#include <stddef.h>

#define USERNAME_MAX 50
#define MESSAGE_CONTENT_MAX 300

typedef enum MessageType {
    MESSAGE_TYPE_CLIENT_DISCONNECT,
    MESSAGE_TYPE_CLIENT_REQUEST_USERNAME,
    MESSAGE_TYPE_CLIENT_MSG,
    MESSAGE_TYPE_CLIENT_ALIVE,
    MESSAGE_TYPE_CLIENT_END_CONNECTION,

    MESSAGE_TYPE_SERVER_ACCEPT,
    MESSAGE_TYPE_SERVER_REFUSED,
    MESSAGE_TYPE_SERVER_ENDED,
    MESSAGE_TYPE_SERVER_BAN,
    MESSAGE_TYPE_SERVER_CLIENT_MESSAGE,
    MESSAGE_TYPE_SERVER_CHANGE_USERNAME,
    MESSAGE_TYPE_SERVER_CHECK_ALIVE,
} MessageType;

typedef struct MessageBodyMessage {
    char username[USERNAME_MAX + 1];
    size_t user_len;
    char content[MESSAGE_CONTENT_MAX + 1];
    size_t content_len;
} MessageBodyMessage;

typedef struct MessageBodyUsername {
    char username[USERNAME_MAX + 1];
    size_t len;
} MessageBodyUsername;
