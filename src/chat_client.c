#include "chat_client.h"

ChatUser *chat_user_create(void)
{
    ChatUser *user = malloc(sizeof(ChatUser));
    if (!user)
        return NULL;
    user->status = CHAT_USER_STATUS_NONE;
    return user;
}

int chat_user_connect(ChatUser *user, int port, const char *ip)
{
    Socket *socket = &user->socket;
    if (!init_client(socket, AF_INET, SOCK_STREAM, port, ip)) {
        user->status = CHAT_USER_STATUS_FAILED;
        return CHAT_USER_ERROR_CONNECT;
    }

    fcntl(socket->fd, F_SETFL, O_NONBLOCK);

    user->status =  CHAT_USER_STATUS_NON_CONFIRMED;
    user->bytes_read = 0;
    strncpy(user->chat_ip_address, ip, sizeof(user->chat_ip_address));
    user->chat_port = port;

    PRINT_DEBUG("ChatUser: Conexao com grupo do chat iniciada\n");

    return CHAT_USER_SUCESS;
}

int chat_user_disconnect(ChatUser *user, enum ChatUserStatus new_status)
{
    if (user->status != CHAT_USER_STATUS_CONNECTED && user->status != CHAT_USER_STATUS_NON_CONFIRMED)
        return CHAT_USER_ERROR_DISCONNECTED;

    user->out.type = CHAT_MESSAGE_CLIENT_END_CONNECTION;
    user->status = new_status;
    user->bytes_read = 0;
    if (new_status == CHAT_USER_STATUS_DISCONNECTED) {
        send(user->socket.fd, &user->out, sizeof(user->out), 0);
    }
    close_socket(&user->socket);
    return CHAT_USER_SUCESS;
}

ssize_t chat_user_process_messages(ChatUser *user)
{
    if (user->status != CHAT_USER_STATUS_CONNECTED && user->status != CHAT_USER_STATUS_NON_CONFIRMED)
        return CHAT_USER_ERROR_DISCONNECTED;
    ssize_t bytes_read = 0;
    ssize_t n = 0;
    while ((n = read_socket_message(user->socket.fd, &user->in, &user->bytes_read, sizeof(user->in))) > 0) {
        if (n == -1) {
            return CHAT_USER_ERROR_READ;
        }
        bytes_read += n;
    }
    return bytes_read;
}

int chat_user_message_ready(ChatUser *user)
{
    if (user->status != CHAT_USER_STATUS_CONNECTED && user->status != CHAT_USER_STATUS_NON_CONFIRMED)
        return CHAT_USER_ERROR_DISCONNECTED;
    if (user->bytes_read == sizeof(user->in))
        return CHAT_USER_MESSAGE_READY;
    return CHAT_USER_MESSAGE_UNREADY;
}

ChatMessage *get_next_message(ChatUser *user)
{
    user->bytes_read = 0;
    return &user->in;
}

void chat_user_delete(ChatUser *user)
{
    if (!user)
        return;
    if (user->status == CHAT_USER_STATUS_CONNECTED || user->status == CHAT_USER_STATUS_NON_CONFIRMED)
        close_socket(&user->socket);
    free(user);
}
