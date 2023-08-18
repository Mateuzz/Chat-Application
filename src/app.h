#pragma once

typedef struct App App;

App *app_create(void);
void app_run(App *app);
void app_delete(App *app);
