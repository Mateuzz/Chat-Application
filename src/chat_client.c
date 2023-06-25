#include "chat_client.h"

ChatUser *chat_user_create(void)
{
    ChatUser *user = malloc(sizeof(ChatUser));
    if (!user)
        return NULL;
    user->status = CHAT_USER_STATUS_DISCONNECTED;
    return user;
}

int chat_user_connect(ChatUser *user, int port, const char *ip)
{
    Socket *socket = &user->socket;
    ssize_t bytes_read = 0;
    if (!init_client(socket, AF_INET, SOCK_STREAM, port, ip))
        return CHAT_USER_ERROR_CONNECT;

    fcntl(socket->fd, F_SETFL, O_NONBLOCK);

    while (bytes_read < sizeof(user->in)) {
        read_socket_message(socket->fd, &user->in, &bytes_read, sizeof(user->in));
    }

    if (user->in.type == ACCEPTED_CLIENT) {
        user->status = CHAT_USER_STATUS_CONNECTED;
        strcpy(user->username, "Convidado");
        return CHAT_USER_SUCESS;
    }

    return CHAT_USER_ERROR_NON_ACCEPTED;
}

int chat_user_disconnect(ChatUser *user)
{
    if (user->status == CHAT_USER_STATUS_DISCONNECTED)
        return CHAT_USER_ERROR_DISCONNECTED;

    user->out.type = ENDED_CONNECTION;
    user->status = CHAT_USER_STATUS_DISCONNECTED;
    send(user->socket.fd, &user->out, sizeof(user->out), 0);
    return CHAT_USER_SUCESS;
}

ssize_t chat_user_send_message(ChatUser *user, const char *message)
{
    if (user->status == CHAT_USER_STATUS_DISCONNECTED) 
        return CHAT_USER_ERROR_DISCONNECTED;

    size_t len = MIN(MESSAGE_MAX, strlen(message));
    user->out.type = CLIENT_MESSAGE;
    memcpy(user->out.msg, message, len);
    return send(user->socket.fd, &user->out, sizeof(user->out), 0);
}

int chat_user_set_username(ChatUser *user, const char *name)
{
    if (user->status == CHAT_USER_STATUS_DISCONNECTED)
        return CHAT_USER_ERROR_DISCONNECTED;
    size_t len = MIN(strlen(name), USERNAME_MAX);
    user->out.type = CLIENT_INFO;
    memcpy(user->out.username, name, len);
    if (send(user->socket.fd, &user->out, sizeof(user->out), 0) > 0) {
        memcpy(user->username, name, len);
        return CHAT_USER_SUCESS;
    }
    return CHAT_USER_ERROR_SEND;
}

ssize_t chat_user_process_messages(ChatUser *user)
{
    if (user->status == CHAT_USER_STATUS_DISCONNECTED)
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
    if (user->status == CHAT_USER_STATUS_DISCONNECTED)
        return CHAT_USER_ERROR_DISCONNECTED;
    if (user->bytes_read == sizeof(user->in))
        return CHAT_USER_MESSAGE_READY;
    return CHAT_USER_MESSAGE_UNREADY;
}

ChatMessage *get_last_message(ChatUser *user)
{
    if (user->status == CHAT_USER_STATUS_DISCONNECTED)
        return NULL;
    user->bytes_read = 0;
    return &user->in;
}

void chat_user_delete(ChatUser *user)
{
    if (!user)
        return;
    if (user->status == CHAT_USER_STATUS_CONNECTED)
        close_socket(&user->socket);
    free(user);
}
