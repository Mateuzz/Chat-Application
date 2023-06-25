#include "app.h"

/* /*! TODO: 
 *  ADicionar painel de adicionar mensagem
 *  permitir mudar nome de usuario
 *  permitir banir usuario
 *  colocar popup em caso de erro em criar server ou join server
 *  dar opcao de disconnect
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
