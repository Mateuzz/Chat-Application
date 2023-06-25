#include "app.h"

/*  TODO: 
 *  ADicionar painel de adicionar mensagem
 *  dar opcao de disconnect
 *  mensagens tanto do cliente como do server devem ter um limite uma hora (com wrap)
 *  permitir mudar nome de usuario
 *  permitir banir usuario
 *  colocar popup em caso de erro em criar server ou join server
 *  criar janela para dono do servidor
 */

int main(void)
{
    App *app = app_create();

    if (app) {
        app_run(app);
        app_delete(app);
    }

    return 0;
}
