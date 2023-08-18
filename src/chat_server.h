#pragma once

#include "message_types.h"
#include "messages.h"
#include "network.h"

#define MAX_CONNECTIONS 30

#define SERVER_INFO_MAX 200
#define SERVER_INFO_DATA_MAX 100

typedef struct ChatServerClient {
    Socket socket;
    MessageReader reader;
    char username[USERNAME_MAX + 1];
    enum ServerClientStatus { CLIENT_STATUS_ACTIVE, CLIENT_STATUS_INACTIVE } status;
} ChatServerClient;

typedef struct ChatServerInfoList {
    char data[SERVER_INFO_MAX][SERVER_INFO_DATA_MAX + 1];
    int count;
} ChatServerInfoList;

typedef struct ChatServer {
    Socket socket;
    struct sockaddr_in banned_clients[MAX_CONNECTIONS];
    int banned_count;
    MessageBuffer received[RECEIVED_MESSAGES_STORED_MAX];
    MessageBuffer out;
    int received_count;
    ChatServerInfoList info;
    ChatServerClient clients[MAX_CONNECTIONS];
    int clients_count;
} ChatServer;

ChatServer *chat_server_create(int port);
void chat_server_update(ChatServer *chat);
void chat_server_delete(ChatServer *chat);

bool chat_server_ban_user(ChatServer *chat, int user_index);
