#pragma once

#include "message_types.h"
#include "messages.h"
#include "network.h"

typedef enum NotifyDisconnectResult { NOTIFY_ERROR, NOTIFY_SUCESS } NotifyDisconnectResult;

typedef struct ChatClient {
    Socket socket;
    MessageReader reader;
    MessageBuffer out;
    char username[USERNAME_MAX + 1];
    enum ChatClientStatus {
        CHAT_CLIENT_STATUS_NONE,
        CHAT_CLIENT_STATUS_REQUEST_FAILED,
        CHAT_CLIENT_STATUS_DISCONNECTED,
        CHAT_CLIENT_STATUS_SERVER_REFUSED,
        CHAT_CLIENT_STATUS_SERVER_BANNED,
        CHAT_CLIENT_STATUS_WAITING,
        CHAT_CLIENT_STATUS_SERVER_ACCEPTED,
    } status;
    MessageBuffer received[RECEIVED_MESSAGES_STORED_MAX];
    int received_count;
} ChatClient;

ChatClient *chat_client_create(void);

bool chat_client_request_join(ChatClient *cc, const char *ip, int port);
NotifyDisconnectResult chat_client_disconnect(ChatClient *cc, enum ChatClientStatus new_status);

void chat_client_update(ChatClient *cc);

void chat_client_delete(ChatClient *cc);
