#pragma once

#include "chat_common.h"

#define CHAT_USER_ERROR_DISCONNECTED -10
#define CHAT_USER_ERROR_READ -11
#define CHAT_USER_ERROR_CONNECT -12
#define CHAT_USER_ERROR_NON_ACCEPTED -13
#define CHAT_USER_ERROR_SEND -14
#define CHAT_USER_SUCESS 0
#define CHAT_USER_MESSAGE_READY 1
#define CHAT_USER_MESSAGE_UNREADY 0

typedef struct ChatUser {
    enum ChatUserStatus {
        CHAT_USER_STATUS_DISCONNECTED,
        CHAT_USER_STATUS_CONNECTED,
    } status;
    Socket socket;
    char username[USERNAME_MAX]; 
    ChatMessage in;
    ChatMessage out;
    ssize_t bytes_read;
} ChatUser;

ChatUser *chat_user_create(void);

// return error code or sucess
int chat_user_connect(ChatUser *user, int port, const char *ip);

// return error code or sucess
int chat_user_disconnect(ChatUser *user);

// return error code or sucess
int chat_user_set_username(ChatUser *user, const char *name);

// return number of bytes send or error code 
ssize_t chat_user_send_message(ChatUser *user, const char *message);

// return number of bytes read or error code 
ssize_t chat_user_process_messages(ChatUser *user);

// return ready or unready, or error code
int chat_user_message_ready(ChatUser *user);

// Should be called only if message is ready
ChatMessage * get_last_message(ChatUser *user);

void chat_user_delete(ChatUser *user);
