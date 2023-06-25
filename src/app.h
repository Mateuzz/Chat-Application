#pragma once

#include "gl_common.h"
#include "nuklear_common.h"

#include "chat_client.h"
#include "chat_server.h"

typedef struct App {
    SDL_Window *window;
    SDL_GLContext gl_context;
    int win_width;
    int win_height;
    struct nk_context *ctx;
    ChatUser *chat_user;
    ChatServer *chat_server;
} App;

App* app_create();
void app_run(App *app);
void app_delete(App *app);
