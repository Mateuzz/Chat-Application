#include "App.h"

int main(void)
{
    App app = {0};

    if (app_init(&app))
        app_run(&app);

    app_delete(&app);

    return 0;
}
