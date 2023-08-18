#pragma once

#include "nuklear_common.h"
#include "chat.h"
#include "chat_server.h"

void chat_client_window(struct nk_context *ctx, ChatClient *client, int ww, int wh);
void chat_server_window(struct nk_context *ctx, ChatServer **server, int ww, int wh);
