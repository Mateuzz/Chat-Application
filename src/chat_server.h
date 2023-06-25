#pragma once

#include "network.h"

#define MAX_CONNECTIONS 30

#define MESSAGE_MAX 300
#define USERNAME_MAX 50

#define CLOCK_TO_MS(c) ((c)*1000)
#define CLOCK_TO_SECONDS(c) ((c)*1000000)
#define TIMEOUT_MAX CLOCK_TO_SECONDS(60)

typedef struct ChatMessage {
    enum Type {
        CLIENT_INFO,
        CLIENT_MESSAGE,
        ACCEPTED_CLIENT,
        REFUSED_CLIENT,
        CHECK_STATUS,
        ENDED_CONNECTION
    } type;
    char username[USERNAME_MAX];
    char msg[MESSAGE_MAX];
} ChatMessage;

typedef struct ChatClient {
    int fd;
    struct sockaddr_in addr;
    ssize_t current_message_bytes_read;
    ChatMessage message_buffer;
    char username[USERNAME_MAX];
    enum ClientStatus { CLIENT_STATUS_ACTIVE, CLIENT_STATUS_INACTIVE } status;
    bool non_responsing;
    clock_t timeout_start;
} ChatClient;

typedef struct ChatServer {
    Socket socket;
    ChatClient clients[MAX_CONNECTIONS];
    int clients_count;
    socklen_t addrlen;
    int port;
} ChatServer;

ChatServer *chat_server_create(int port);
void chat_server_update(ChatServer *chat);
void chat_server_delete(ChatServer* chat);
