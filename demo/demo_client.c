#include "chat_client.h"

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

    user->out.type = CHAT_MESSAGE_CLIENT_CHANGE_INFO;
    strcpy(user->out.username, "Mateus");

    if (chat_user_send_message(user) < 0) {
        printf("Erro ao mudar mudar de nome\n");
        return error;
    }

    int i = 0;
    while (i < 20) {
        while (chat_user_process_messages(user) > 0) {

        }

        if (chat_user_message_ready(user) == CHAT_USER_MESSAGE_READY)  {
            ChatMessage *in = get_next_message(user);

            switch (in->type) {
            case CHAT_MESSAGE_CLIENT_MESSAGE:
                printf("Alguem mandou uma mensagen\n");
                printf("%s: %s\n", in->username, in->msg);
                break;

            default:
                printf("Estranho\n");
            }
        }

        sleep(1);
        user->out.type = CHAT_MESSAGE_CLIENT_MESSAGE;
        sprintf(user->out.msg, "Mensagem n° %d\n", i);
        if (chat_user_send_message(user) < 0) {
            printf("Erro ao mandar mensagem\n");
        }

        ++i;
    }

    if (chat_user_disconnect(user) == CHAT_USER_SUCESS) {
        puts("Conexao desconectada");
    }

    chat_user_delete(user);

    return 0;
}
