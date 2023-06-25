#pragma once

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#include <nuklear/nuklear.h>
#include <nuklear/nuklear_sdl_gl3.h>

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
