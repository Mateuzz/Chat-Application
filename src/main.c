#include "app.h"

int main(void)
{
    App *app = app_create();

    if (app) {
        app_run(app);
        app_delete(app);
    }

    return 0;
}
