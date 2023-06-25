#include "Interface.h"
#include "chat_server.h"

// preciso da porta em que vai criar

void draw_server_window(struct nk_context *ctx, ChatServer *chat_server)
{
    static struct nk_colorf bg = { 0.05f, 0.05f, 0.09f, 1.0f };
    static int port = 0;
    
    if (nk_begin(ctx,
                "Servidor",
                nk_rect(0, 0, 300, 90),
                NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {
        float port_row_ratio[] = { 0.5f, 0.5f };
        nk_layout_row(ctx, NK_DYNAMIC, 35, 2, port_row_ratio);
        nk_property_int(ctx, "Port", 0, &port, 65535, 1, 1);

        if (nk_button_label(ctx, "Create chat group")) {
            chat_server = chat_server_create(port);
        }
    } 
    nk_end(ctx);


    glClearColor(bg.r, bg.g, bg.b, bg.a);
    glClear(GL_COLOR_BUFFER_BIT);
}
