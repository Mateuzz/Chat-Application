#pragma once

#include "chat_common.h"

#define MAX_CONNECTIONS 30
#define TIMEOUT_WARN_USER CLOCK_TO_SECONDS(30)
#define TIMEOUT_BAN_USER CLOCK_TO_SECONDS(60)

typedef struct ChatClient {
    int fd;
    struct sockaddr_in addr;
    ssize_t current_message_bytes_read;
    ChatMessage message_buffer;
    char username[USERNAME_MAX + 1];
    enum ClientStatus {
        CLIENT_STATUS_ACTIVE,
        CLIENT_STATUS_NON_RESPONDING,
        CLIENT_STATUS_TIMEOUT_WAITING,
        CLIENT_STATUS_INACTIVE
    } status;
    clock_t timeout_start;
} ChatClient;

typedef struct MessageList {
    ChatMessage *messages;
    size_t count;
    size_t max;
} MessageList;

typedef struct ChatServer {
    Socket socket; 
    ChatClient clients[MAX_CONNECTIONS];
    MessageList received;
    int clients_count;
    socklen_t addrlen;
    int port;
} ChatServer;

ChatServer *chat_server_create(int port);
void chat_server_update(ChatServer *chat);
void chat_server_delete(ChatServer *chat);

int chat_server_ban_user(ChatServer *chat, int user_index);
int chat_server_get_user_index(ChatServer *chat, const char *name);

int message_list_init(MessageList *list, int max_messages);
void message_list_deinit(MessageList *list);
int message_list_add(MessageList* list, const ChatMessage *message);
