#include "chat_client.h"
#include "network.h"

int main(void)
{
    ChatUser *user = chat_user_create();
    int error;

    if (!user) {
        printf("Nao conseguiu criar usuario\n");
        return 1;
    }

    if ((error = chat_user_connect(user, 8080, LOCAL_HOST_IP)) != CHAT_USER_SUCESS) {
        printf("Erro em conectar chat ao server: %d\n", error);
        return error;
    }

    if ((error = chat_user_set_username(user, "Mateus")) != CHAT_USER_SUCESS) {
        printf("Erro ao mudar de nome: %d\n", error);
        return error;
    }

    sleep(1);

    error = chat_user_send_message(user, "Uma mensagem enviada ao servidor");
    if (error < 0) {
        printf("Erro ao mudar mandar mensagem: %d\n", error);
        return error;
    }

    while (chat_user_message_ready(user) != CHAT_USER_MESSAGE_READY) {
        chat_user_process_messages(user);
    }

    if (chat_user_message_ready(user) == CHAT_USER_MESSAGE_READY) {
        printf("Mensagem recebida do servidor\n");
        ChatMessage *in = get_last_message(user);

        switch (in->type) {
        case ACCEPTED_CLIENT:
            printf("CLiente foi aceito\n");
            break;

        case REFUSED_CLIENT:
            printf("Cliente recusado\n");
            break;

        case CLIENT_MESSAGE:
            printf("Alguem mandou uma mensagen\n");
            printf("%s: %s\n", in->username, in->msg);
            break;

        default:
            printf("Estranho\n");
        }
    } else {
        printf("Mensagem nao pronta anda\n");
    }

    chat_user_delete(user);
}
