#pragma once

typedef struct App App;

App* app_create();
void app_run(App *app);
void app_delete(App *app);
