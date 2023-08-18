#include "app.h"

int main(void)
{
    App *app = app_create();

    if (!app)
        return 1;

    app_run(app);
    app_delete(app);

    return 0;
}
