#pragma once

#include "chat_common.h"

#define MAX_CONNECTIONS 30
#define TIMEOUT_WARN_USER CLOCK_TO_SECONDS(120)
#define TIMEOUT_BAN_USER CLOCK_TO_SECONDS(240)

#define SERVER_INFO_BUFFER_SIZE 200
#define SERVER_INFO_BUFFER_DATA_SIZE USERNAME_MAX

typedef struct ChatClient {
    Socket socket;
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

typedef struct ChatServerInfo {
    enum ServerInfoType {
        INFO_TYPE_NONE,
        INFO_CLIENT_CHANGE_NAME,
        INFO_CLIENT_BANNED,
        INFO_CLIENT_ENTERED,
        INFO_CLIENT_ACCEPTED,
        INFO_CLIENT_DISCONNECTED,
        INFO_CLIENT_REFUSED,
        INFO_CLIENT_CHECKING_ALIVE,
        INFO_CLIENT_CONFIRMED_ALIVE,
    } type;
    int flag;
    char data1[SERVER_INFO_BUFFER_DATA_SIZE]; // used for username mostly
    char data2[SERVER_INFO_BUFFER_DATA_SIZE];
} ChatServerInfo;

typedef struct ChatServerInfoList {
    ChatServerInfo data[SERVER_INFO_BUFFER_SIZE];
    int count;
} ChatServerInfoList;

typedef struct ChatServer {
    Socket socket; 
    ChatClient clients[MAX_CONNECTIONS];
    struct sockaddr_in banned_clients[MAX_CONNECTIONS];
    int clients_banned_count;
    ChatServerInfoList info;
    MessageList received;
    ChatMessage message_buffer;
    int clients_count;
    socklen_t addrlen;
    int port;
} ChatServer;

ChatServer *chat_server_create(int port);
void chat_server_update(ChatServer *chat);
void chat_server_delete(ChatServer *chat);

int chat_server_ban_user(ChatServer *chat, int user_index);
int chat_server_get_user_index(ChatServer *chat, const char *name);
