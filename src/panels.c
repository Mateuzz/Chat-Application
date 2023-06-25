#include "panels.h"
#include "chat_client.h"
#include "chat_server.h"
#include <pthread.h>
#include <signal.h>

// preciso da porta em que vai criar

void user_window_init(ChatUserWindow *window, int max_messages)
{
    window->chat_user = chat_user_create();
    window->messages_max = max_messages;
    window->messages_count = 0;
    window->messages = malloc(sizeof(WindowChatMessage) * max_messages);
}

void server_window_init(ChatServerWindow *window)
{
    window->chat_server = NULL;
    pthread_mutex_init(&window->arg.lock, NULL);
}

void user_window_deinit(ChatUserWindow *window)
{
    window->messages_max = 0;
    window->messages_count = 0;
    chat_user_delete(window->chat_user);
    free(window->messages);
}

void server_window_deinit(ChatServerWindow *window)
{
    pthread_mutex_destroy(&window->arg.lock);
    if (window->chat_server) {
        chat_server_delete(window->chat_server);
        window->chat_server = NULL;
    }
}

static void *thread_server_run(void *arg)
{
    ServerThreadArg *data = arg;
    ChatServer *chat_server = data->chat_server;

    while (true) {
        pthread_mutex_lock(&data->lock);
        if (!data->running)
            break;
        chat_server_update(chat_server);
        pthread_mutex_unlock(&data->lock);
    }

    return NULL;
}

void server_window_draw(struct nk_context *ctx, ChatServerWindow *window)
{
    static struct nk_colorf bg = {0.05f, 0.05f, 0.09f, 1.0f};
    static int port = 0;
    static char buffer_label[500];
    int w_width = 300;
    int w_height = 120;

    if (nk_begin(ctx,
                 "Servidor",
                 nk_rect(0, 0, w_width, w_height),
                 NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_MOVABLE |
                     NK_WINDOW_SCALABLE)) {
        if (window->chat_server) {
            nk_layout_row_dynamic(ctx, 30, 1);
            sprintf(buffer_label, "Server running at port %d", port);
            nk_label(ctx, buffer_label, NK_TEXT_CENTERED);

            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_button_label(ctx, "Stop server")) {
                pthread_mutex_lock(&window->arg.lock);
                window->arg.running = false;
                chat_server_delete(window->chat_server);
                window->chat_server = NULL;
                pthread_mutex_unlock(&window->arg.lock);
                pthread_join(window->thread, NULL);
            }
        } else {
            float port_row_ratio[] = {0.5f, 0.5f};

            nk_layout_row(ctx, NK_DYNAMIC, 35, 2, port_row_ratio);
            nk_property_int(ctx, "Port", 0, &port, 65535, 1, 1);
            if (nk_button_label(ctx, "Create chat group")) {
                if ((window->chat_server = chat_server_create(port))) {
                    window->arg.running = true;
                    window->arg.chat_server = window->chat_server;
                    pthread_mutex_unlock(&window->arg.lock);
                    pthread_create(&window->thread, NULL, thread_server_run, &window->arg);
                }
            }
        }
    }
    nk_end(ctx);

    glClearColor(bg.r, bg.g, bg.b, bg.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

int user_window_add_message(ChatUserWindow *window, const ChatMessage *message)
{
    if (window->messages_count > window->messages_max) {
        return -1;
    }

    int i = window->messages_count;

    strcpy(window->messages[i].message, message->msg);
    strcpy(window->messages[i].username, message->username);
    ++window->messages_count;

    return 0;
}

void user_window_draw(struct nk_context *ctx, ChatUserWindow *window)
{
    static char ip_address[20] = "";
    static int ip_address_len = 0;
    static int port = 0;

    if (nk_begin(ctx,
                 "Chat",
                 nk_rect(400, 0, 500, 200),
                 NK_WINDOW_MINIMIZABLE | NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE |
                     NK_WINDOW_SCALABLE)) {
        if (window->chat_user->status != CHAT_USER_STATUS_CONNECTED) {
            float ratio[] = {0.2f, 0.5f, 0.3f};
            nk_layout_row(ctx, NK_DYNAMIC, 30, 3, ratio);
            nk_label(ctx, "IP Address", NK_TEXT_LEFT);
            nk_edit_string(ctx, NK_EDIT_SIMPLE, ip_address, &ip_address_len, 20, nk_filter_ascii);
            nk_property_int(ctx, "Port", 0, &port, 65535, 1, 1);

            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_button_label(ctx, "Join Chat")) {
                if (chat_user_connect(window->chat_user, port, ip_address) != CHAT_USER_SUCESS) {
                }
            }
        } else {
        }
    }
    nk_end(ctx);
}
