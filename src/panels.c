#include "panels.h"
#include "chat_client.h"
#include "chat_common.h"
#include "chat_server.h"
#include <pthread.h>
#include <signal.h>

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

void user_window_init(ChatUserWindow *window, int max_messages)
{
    window->chat_user = chat_user_create();
    message_list_init(&window->messages, max_messages);
}

void user_window_deinit(ChatUserWindow *window)
{
    message_list_deinit(&window->messages);
    chat_user_delete(window->chat_user);
    window->chat_user = NULL;
}

void user_window_draw(struct nk_context *ctx, ChatUserWindow *window)
{
    static char buffer[200];
    static char username[USERNAME_MAX];
    static int username_len = 0;
    static char ip_address[21] = "";
    static int ip_address_len = 0;
    static int port = 0;
    static int w_width;
    static char msg_buffer[MESSAGE_MAX];
    static int msg_len = 0;
    ChatUser *user = window->chat_user;

    if (window->chat_user->status != CHAT_USER_STATUS_CONNECTED) {
        if (nk_begin(ctx,
                     "Chat",
                     nk_rect(300, 0, 500, 200),
                     NK_WINDOW_MINIMIZABLE | NK_WINDOW_BORDER | NK_WINDOW_TITLE |
                         NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {
            float ratio[] = {0.2f, 0.3f};

            nk_layout_row(ctx, NK_DYNAMIC, 30, 2, ratio);
            nk_label(ctx, "IP Address", NK_TEXT_LEFT);
            nk_edit_string(ctx, NK_EDIT_SIMPLE, ip_address, &ip_address_len, 20, nk_filter_ascii);

            nk_layout_row_dynamic(ctx, 30, 2);
            nk_property_int(ctx, "Port", 0, &port, 65535, 1, 1);

            nk_layout_row(ctx, NK_DYNAMIC, 30, 2, ratio);
            nk_label(ctx, "Username", NK_TEXT_LEFT);
            nk_edit_string(ctx, NK_EDIT_SIMPLE, username, &username_len, USERNAME_MAX, nk_filter_ascii);

            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_button_label(ctx, "Join Chat")) {
                ip_address[ip_address_len] = '\0';
                chat_user_connect(user, port, ip_address);
                chat_message_send(&user->socket, CHAT_MESSAGE_CLIENT_CHANGE_INFO, username, username_len);
            }
        }
        nk_end(ctx);
    } else {
        sprintf(buffer,
                "Messages - IP %s - Port %d",
                user->chat_ip_address,
                (int)user->chat_port);

        if (nk_begin(ctx,
                     buffer,
                     nk_rect(300, 0, 900, 680),
                     NK_WINDOW_MINIMIZABLE | NK_WINDOW_BORDER | NK_WINDOW_TITLE |
                         NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {
            w_width = nk_window_get_width(ctx);

            float char_per_line = w_width * 0.8f / 8.3f;
            MessageList *msgs = &window->messages;

            for (size_t i = 0; i < msgs->count; ++i) {
                int lines = ceilf(msgs->messages[i].msg_len / char_per_line);
                int row_height = lines * 20;
                nk_layout_row(ctx, NK_DYNAMIC, row_height, 2, (float[]) {0.18f, 0.76f});
                nk_label(ctx, msgs->messages[i].username, NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_TOP);
                nk_label_wrap(ctx, msgs->messages[i].msg);
            }
            nk_end(ctx);
        }

        if (nk_begin(ctx, "Chat Prompt", nk_rect(300, 680, 900, 120), NK_WINDOW_MINIMIZABLE | NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {
            nk_layout_row(ctx, NK_DYNAMIC, 30, 2, (float[]) {0.8f, 0.2f});
            nk_edit_string(ctx, NK_EDIT_SIMPLE, msg_buffer, &msg_len, MESSAGE_MAX, nk_filter_default);

            if (nk_button_label(ctx, "Send Message") && msg_len > 0) {
                PRINT_DEBUG("Message size is %d\n", msg_len);
                chat_message_send(&user->socket, CHAT_MESSAGE_CLIENT_MESSAGE, msg_buffer, msg_len);
                msg_len = 0;
            }


            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_button_label(ctx, "Disconnect from chat")) {
                chat_user_disconnect(user);
            }

            nk_end(ctx);
        }

    }
}

void server_window_init(ChatServerWindow *window)
{
    window->chat_server = NULL;
    pthread_mutex_init(&window->arg.lock, NULL);
}

void server_window_deinit(ChatServerWindow *window)
{
    window->arg.running = false;
    pthread_join(window->thread, NULL);
    pthread_mutex_destroy(&window->arg.lock);
    if (window->chat_server) {
        chat_server_delete(window->chat_server);
        window->chat_server = NULL;
    }
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
            }
        } else {
            // TODO -> adicionar janela de mensagens do servidor
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
