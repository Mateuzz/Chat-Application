#define NK_IMPLEMENTATION
#define NK_SDL_GL3_IMPLEMENTATION
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#include "App.h"
#include "Interface.h"

#include <stdlib.h>

typedef enum InputResult {
    INPUT_RESULT_NONE,
    INPUT_RESULT_QUIT,
} InputResult;

static InputResult handle_input(App *app)
{
    SDL_Event evt;

    nk_input_begin(app->ctx);
    while (SDL_PollEvent(&evt)) {
        if (evt.type == SDL_QUIT)
            return INPUT_RESULT_QUIT;
        nk_sdl_handle_event(&evt);
    }
    nk_input_end(app->ctx);

    SDL_GetWindowSize(app->window, &app->win_width, &app->win_height);

    return INPUT_RESULT_NONE;
}

static void render_buffer(App *app)
{
    glViewport(0, 0, app->win_width, app->win_height);
    nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
    SDL_GL_SwapWindow(app->window);
}

App *app_create()
{
    App *app = malloc(sizeof(App));
    if (!app)
        return NULL;

    app->window = NULL;
    app->gl_context = NULL;

    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    app->window = SDL_CreateWindow("Demo",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   WINDOW_WIDTH,
                                   WINDOW_HEIGHT,
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI |
                                       SDL_WINDOW_RESIZABLE);
    if (!app->window)
        goto cleanup;

    app->gl_context = SDL_GL_CreateContext(app->window);
    if (!app->gl_context)
        goto cleanup;

    SDL_GetWindowSize(app->window, &app->win_width, &app->win_height);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glewExperimental = 1;
    if (glewInit() != GLEW_OK)
        goto cleanup;

    app->ctx = nk_sdl_init(app->window);
    if (!app->ctx)
        goto cleanup;

    {
        struct nk_font_atlas *atlas;
        nk_sdl_font_stash_begin(&atlas);
        nk_sdl_font_stash_end();
    }

    return app;

cleanup:
    SDL_DestroyWindow(app->window);
    SDL_GL_DeleteContext(app->gl_context);
    SDL_Quit();
    return NULL;
}

void app_run(App *app)
{
    while (true) {
        switch (handle_input(app)) {
        case INPUT_RESULT_QUIT:
            return;

        case INPUT_RESULT_NONE:
            break;
        }

        draw_gui(app->ctx);

        render_buffer(app);
    }
}

void app_delete(App *app)
{
    nk_sdl_shutdown();
    SDL_GL_DeleteContext(app->gl_context);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
    free(app);
}
