#pragma once

#include "chat_common.h"

#define MAX_CONNECTIONS 30
#define TIMEOUT_MAX CLOCK_TO_SECONDS(60)

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

int chat_server_ban_user(ChatServer* chat, int user_index);
int chat_server_get_user_index(ChatServer* chat, const char *name);
