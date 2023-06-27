#include "app.h"
#include "chat_client.h"
#include "chat_common.h"
#include "panels.h"
#include "pchheader.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 700

static ChatMessage g_message_buffer;

struct App {
    SDL_Window *window;
    SDL_GLContext gl_context;
    int win_width;
    int win_height;
    struct nk_context *ctx;
    ChatUserWindow chat_user_window;
    ChatServerWindow chat_server_window;
};

typedef enum InputResult {
    INPUT_RESULT_NONE,
    INPUT_RESULT_QUIT,
} InputResult;

static InputResult handle_input(App *app)
{
    SDL_Event evt;

    nk_input_begin(app->ctx);
    while (SDL_PollEvent(&evt)) {
        if (evt.type == SDL_QUIT)
            return INPUT_RESULT_QUIT;
        nk_sdl_handle_event(&evt);
    }
    nk_input_end(app->ctx);

    SDL_GetWindowSize(app->window, &app->win_width, &app->win_height);
    glViewport(0, 0, app->win_width, app->win_height);

    return INPUT_RESULT_NONE;
}

static void render_buffer(App *app)
{
    static struct nk_colorf bg = {0.05f, 0.05f, 0.09f, 1.0f};

    glClearColor(bg.r, bg.g, bg.b, bg.a);
    glClear(GL_COLOR_BUFFER_BIT);

    nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
    SDL_GL_SwapWindow(app->window);
}

static void process_server(App *app)
{
    ChatServerWindow *window = &app->chat_server_window;
    server_window_draw(app->ctx, window, app->win_width, app->win_height);
}

static void process_user(App *app)
{
    ChatUserWindow *window = &app->chat_user_window;
    ChatUser *user = window->chat_user;

    chat_user_process_messages(user);

    if (chat_user_message_ready(user) != CHAT_USER_MESSAGE_READY) {
        user_window_draw(app->ctx, &app->chat_user_window, app->win_width, app->win_height);
        return;
    }

    ChatMessage *msg = get_next_message(user);

    switch (msg->type) {
    case CHAT_MESSAGE_CLIENT_MESSAGE:
        PRINT_DEBUG("AppUser: Mensagem do servidor recebida\n");
        message_list_add(&window->messages, msg);
        break;

    case CHAT_MESSAGE_SERVER_ACCEPTED:
        PRINT_DEBUG("AppUser: Cliente %s foi aceito pelo server\n", msg->username);
        strcpy(user->username, msg->username);
        user->status = CHAT_USER_STATUS_CONNECTED;
        break;

    case CHAT_MESSAGE_SERVER_REFUSED:
        PRINT_DEBUG("AppUser: Cliente foi negado pelo server\n", msg->username);
        chat_user_disconnect(user, CHAT_USER_STATUS_REFUSED);
        break;

    case CHAT_MESSAGE_SERVER_CHECK_ALIVE:
        chat_message_make_and_send(&user->socket,
                                   &g_message_buffer,
                                   CHAT_MESSAGE_CLIENT_ALIVE,
                                   NULL,
                                   0);
        break;

    case CHAT_MESSAGE_SERVER_BAN:
        chat_user_disconnect(user, CHAT_USER_STATUS_BANNED);
        break;

    case CHAT_MESSAGE_SERVER_ENDED:
        PRINT_DEBUG("Appuser: Cliente foi terminado\n");
        chat_user_disconnect(user, CHAT_USER_STATUS_DISCONNECTED);
        break;
    }

    user_window_draw(app->ctx, &app->chat_user_window, app->win_width, app->win_height);
}

App *app_create()
{
    App *app = malloc(sizeof(App));
    if (!app)
        return NULL;

    app->window = NULL;
    app->gl_context = NULL;
    app->chat_user_window.chat_user = NULL;

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    app->window = SDL_CreateWindow("Chat Application",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   WINDOW_WIDTH,
                                   WINDOW_HEIGHT,
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI |
                                       SDL_WINDOW_RESIZABLE);
    if (!app->window)
        goto cleanup;

    app->gl_context = SDL_GL_CreateContext(app->window);
    if (!app->gl_context)
        goto cleanup;

    SDL_GetWindowSize(app->window, &app->win_width, &app->win_height);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glewExperimental = 1;
    if (glewInit() != GLEW_OK)
        goto cleanup;

    app->ctx = nk_sdl_init(app->window);
    if (!app->ctx)
        goto cleanup;

    {
        struct nk_font_atlas *atlas;
        nk_sdl_font_stash_begin(&atlas);
        nk_sdl_font_stash_end();
    }

    set_style(app->ctx, THEME_DARK);

    server_window_init(&app->chat_server_window);
    user_window_init(&app->chat_user_window, 200);

    return app;

cleanup:
    SDL_DestroyWindow(app->window);
    SDL_GL_DeleteContext(app->gl_context);
    SDL_Quit();
    return NULL;
}

void app_run(App *app)
{
    while (true) {
        switch (handle_input(app)) {
        case INPUT_RESULT_QUIT:
            return;

        case INPUT_RESULT_NONE:
            break;
        }

        process_server(app);
        process_user(app);

        render_buffer(app);
    }
}

void app_delete(App *app)
{
    user_window_deinit(&app->chat_user_window);
    server_window_deinit(&app->chat_server_window);
    nk_sdl_shutdown();
    SDL_GL_DeleteContext(app->gl_context);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
    free(app);
}
