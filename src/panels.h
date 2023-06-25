#pragma once

#include "chat_client.h"
#include "chat_server.h"
#include "gl_common.h"
#include "nuklear_common.h"
#include <bits/pthreadtypes.h>

typedef struct WindowChatMessage {
    char username[USERNAME_MAX];
    char message[MESSAGE_MAX];
} WindowChatMessage;

typedef struct ChatUserWindow {
    ChatUser *chat_user;
    WindowChatMessage *messages;
    size_t messages_count;
    size_t messages_max;
} ChatUserWindow;

typedef struct ServerThreadArg {
    ChatServer* chat_server; 
    bool running;
    pthread_mutex_t lock;
} ServerThreadArg;

typedef struct ChatServerWindow {
    ChatServer *chat_server;
    pthread_t thread;
    ServerThreadArg arg;
} ChatServerWindow;

void user_window_init(ChatUserWindow *window, int max_messages);
void server_window_init(ChatServerWindow *window);

void user_window_deinit(ChatUserWindow *window);
void server_window_deinit(ChatServerWindow *window);

void server_window_draw(struct nk_context *ctx, ChatServerWindow *window);
void user_window_draw(struct nk_context *ctx, ChatUserWindow *window);

int user_window_add_message(ChatUserWindow* window, const ChatMessage *message);
