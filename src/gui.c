#include "gui.h"
#include "chat.h"
#include "chat_server.h"
#include "message_types.h"
#include "messages.h"

#define WINDOW_FLAGS (NK_WINDOW_BORDER | NK_WINDOW_TITLE)

void chat_client_window(struct nk_context *ctx, ChatClient *client, int ww, int wh)
{
    static char buffer[200];
    static char msg_buffer[MESSAGE_CONTENT_MAX];
    static int msg_len;
    static char username[USERNAME_MAX];
    static int username_len;
    static char ip_address[21];
    static int ip_address_len;
    static int port;
    static int w_width;
    static enum Flags {
        FLAG_USERNAME_EMPTY = 0x01,
    } flags;

    int width = ww - 600;
    if (width <= 0)
        width = 0;
    int height = wh;

    if (client->status != CHAT_CLIENT_STATUS_SERVER_ACCEPTED) {
        if (nk_begin(ctx, "Join Chat", nk_rect(600, 0, width, height), WINDOW_FLAGS)) {
            float ratio[] = {0.2f, 0.3f};

            nk_layout_row(ctx, NK_DYNAMIC, 30, 2, ratio);
            nk_label(ctx, "IP Address", NK_TEXT_LEFT);
            nk_edit_string(ctx, NK_EDIT_SIMPLE, ip_address, &ip_address_len, 20, nk_filter_ascii);

            nk_layout_row_dynamic(ctx, 30, 2);
            nk_property_int(ctx, "Port", 0, &port, 65535, 1, 1);

            nk_layout_row(ctx, NK_DYNAMIC, 30, 2, ratio);
            nk_label(ctx, "Username", NK_TEXT_LEFT);
            nk_edit_string(ctx,
                           NK_EDIT_SIMPLE,
                           username,
                           &username_len,
                           USERNAME_MAX,
                           nk_filter_ascii);

            nk_layout_row_dynamic(ctx, 30, 1);
            bool enter_pressed =
                nk_window_has_focus(ctx) && ctx->input.keyboard.keys[NK_KEY_ENTER].clicked;

            if (nk_button_label(ctx, "Join Chat") || enter_pressed) {
                if (username_len == 0) {
                    flags |= FLAG_USERNAME_EMPTY;
                } else {
                    flags &= ~FLAG_USERNAME_EMPTY;

                    chat_client_request_join(client, ip_address, port);

                    MessageBodyUsername *new_username = GET_BODY(client->out);
                    memcpy(new_username->username, username, username_len);
                    new_username->len = username_len;

                    SET_HEADER(client->out, MESSAGE_TYPE_CLIENT_REQUEST_USERNAME, sizeof(MessageBodyUsername));
                    SEND_MESSAGE(client->socket.fd, client->out);
                }
            }

            if (flags & FLAG_USERNAME_EMPTY) {
                nk_layout_row_dynamic(ctx, 30, 1);
                nk_label(ctx, "Username Field is empty", NK_TEXT_LEFT);
            }

            switch (client->status) {
            case CHAT_CLIENT_STATUS_WAITING:
                nk_layout_row_dynamic(ctx, 30, 1);
                nk_label(ctx, "Wanting Server accept", NK_TEXT_LEFT);
                break;

            case CHAT_CLIENT_STATUS_SERVER_REFUSED:
                nk_layout_row_dynamic(ctx, 30, 1);
                nk_label(ctx, "Server Refused connection", NK_TEXT_LEFT);
                break;

            case CHAT_CLIENT_STATUS_DISCONNECTED:
                nk_layout_row_dynamic(ctx, 30, 1);
                nk_label(ctx, "You were disconnected from the server", NK_TEXT_LEFT);
                break;

            case CHAT_CLIENT_STATUS_SERVER_BANNED:
                nk_layout_row_dynamic(ctx, 30, 1);
                nk_label(ctx, "You were banned from the server", NK_TEXT_LEFT);
                break;

            case CHAT_CLIENT_STATUS_REQUEST_FAILED:
                nk_layout_row_dynamic(ctx, 30, 1);
                nk_label(ctx, "Request to chat failed", NK_TEXT_LEFT);
                break;

            case CHAT_CLIENT_STATUS_SERVER_ACCEPTED:
            case CHAT_CLIENT_STATUS_NONE:
                break;
            }
        }
        nk_end(ctx);
    } else {
        sprintf(buffer,
                "Messages - IP %u - Port %d",
                client->socket.addr.sin_addr.s_addr,
                (int)client->socket.addr.sin_port);

        if (nk_begin(ctx, "Chat Prompt", nk_rect(600, 0, width, height), WINDOW_FLAGS)) {
            nk_layout_row_dynamic(ctx, height - 150, 1);
            if (nk_group_begin(ctx, "Messages", WINDOW_FLAGS)) {
                w_width = nk_window_get_width(ctx);

                float char_per_line = w_width * 0.8f / 8.3f;
                MessageBuffer *received = client->received;

                for (int i = 0; i < client->received_count; ++i) {
                    MessageBodyMessage *msg = GET_BODY(received[i]);
                    int lines = ceilf(msg->content_len / char_per_line);
                    int row_height = lines * 20;

                    nk_layout_row(ctx, NK_DYNAMIC, row_height, 3, (float[]) {0.15f, 0.05f, 0.75f});
                    nk_label(ctx,
                            msg->username,
                             NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_LEFT);
                    nk_label(ctx, " : ", NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_TOP);
                    nk_label_wrap(ctx, msg->content);
                }
                nk_group_end(ctx);
            }

            nk_layout_row(ctx, NK_DYNAMIC, 30, 2, (float[]) {0.8f, 0.2f});
            nk_edit_string(ctx,
                           NK_EDIT_SIMPLE,
                           msg_buffer,
                           &msg_len,
                           sizeof(msg_buffer),
                           nk_filter_default);

            bool enter_pressed =
                nk_window_has_focus(ctx) && ctx->input.keyboard.keys[NK_KEY_ENTER].clicked;

            if ((nk_button_label(ctx, "Send Message") || enter_pressed) && msg_len > 0) {
                MessageBodyMessage *out = GET_BODY(client->out);
                memcpy(out->content, msg_buffer, msg_len);
                out->content_len = msg_len;

                SET_HEADER(client->out, MESSAGE_TYPE_CLIENT_MSG, sizeof(MessageBodyMessage));
                SEND_MESSAGE(client->socket.fd, client->out);

                msg_len = 0;
            }

            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_button_label(ctx, "Disconnect from chat"))
                chat_client_disconnect(client, CHAT_CLIENT_STATUS_DISCONNECTED);
        }
        nk_end(ctx);
    }
}

void chat_server_window(struct nk_context *ctx, ChatServer **server, int ww, int wh)
{
    static int port = 0;
    static char buffer_label[SERVER_INFO_DATA_MAX + 1];

    int height = wh;

    if (!*server) {
        if (nk_begin(ctx, "Server", nk_rect(0, 0, 600, 120), WINDOW_FLAGS)) {
            float port_row_ratio[] = {0.5f, 0.5f};

            nk_layout_row(ctx, NK_DYNAMIC, 35, 2, port_row_ratio);
            nk_property_int(ctx, "Port", 0, &port, 65535, 1, 1);

            bool enter_pressed =
                nk_window_has_focus(ctx) && ctx->input.keyboard.keys[NK_KEY_ENTER].clicked;

            if (nk_button_label(ctx, "Create chat group") || enter_pressed)
                *server = chat_server_create(port);
        }
        nk_end(ctx);
    } else {
        if (nk_begin(ctx, "Server Manager", nk_rect(0, 0, 600, height), WINDOW_FLAGS)) {
            nk_layout_row_dynamic(ctx, 30, 1);
            sprintf(buffer_label, "Server running at port %d", port);
            nk_label(ctx, buffer_label, NK_TEXT_CENTERED);

            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_button_label(ctx, "Stop server")) {
                chat_server_delete(*server);
                *server = NULL;
                nk_end(ctx);
                return;
            }

            static char *tabs[3] = {"Messages", "Server info", "Members"};
            static enum Tab { TAB_MESSAGES, TAB_INFO, TAB_MEMBERS, TAB_COUNT } tab_selected;

            nk_layout_row_dynamic(ctx, 35, 1);
            if (nk_group_begin(ctx, "Tab", NK_WINDOW_NO_SCROLLBAR)) {
                nk_layout_row_static(ctx, 25, 100, TAB_COUNT);
                for (int i = 0; i < TAB_COUNT; ++i) {
                    int non_selected = 0;
                    int selected = 1;
                    int *select_status = (i == tab_selected) ? &selected : &non_selected;
                    if (nk_selectable_label(ctx, tabs[i], NK_TEXT_CENTERED, select_status)) {
                        tab_selected = i;
                    }
                }

                nk_group_end(ctx);
            }

            switch (tab_selected) {
            case TAB_MESSAGES:
                nk_layout_row_dynamic(ctx, height - 165, 1);
                if (nk_group_begin(ctx, "User Messages", WINDOW_FLAGS)) {
                    float w_width = nk_window_get_width(ctx);
                    float char_per_line = w_width * 0.8f / 8.3f;
                    MessageBuffer *msgs = (*server)->received;
                    int received_count = (*server)->received_count;

                    for (size_t i = 0; i < received_count; ++i) {
                        MessageBodyMessage *msg = GET_BODY(msgs[i]);
                        int lines = ceilf(msg->content_len / char_per_line);
                        int row_height = lines * 20;
                        nk_layout_row(ctx,
                                      NK_DYNAMIC,
                                      row_height,
                                      3,
                                      (float[]) {0.15f, 0.05f, 0.75f});

                        nk_label(ctx,
                                 msg->username,
                                 NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_LEFT);
                        nk_label(ctx, " : ", NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_TOP);
                        nk_label_wrap(ctx, msg->content);
                    }

                    nk_group_end(ctx);
                }
                break;

            case TAB_INFO: {
                int info_count = (*server)->info.count;
                char (*info_data)[SERVER_INFO_DATA_MAX + 1] = (*server)->info.data;

                for (int i = 0; i < info_count; ++i) {
                    strcpy(buffer_label, info_data[i]); 
                    nk_layout_row_dynamic(ctx, 30, 1);
                    nk_label_wrap(ctx, buffer_label);
                } 
            } break;

            case TAB_MEMBERS: {
                int clients_count = (*server)->clients_count;
                ChatServerClient *clients = (*server)->clients;

                for (int i = 0; i < clients_count; ++i) {
                    nk_layout_row_dynamic(ctx, 30, 2);
                    nk_label(ctx, clients[i].username, NK_TEXT_CENTERED);
                    if (nk_button_label(ctx, "Ban"))
                        chat_server_ban_user(*server, i);
                }
            } break;

            default:
                break;
            }
        }
        nk_end(ctx);
    }
}
