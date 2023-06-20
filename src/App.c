#define NK_IMPLEMENTATION
#define NK_SDL_GL3_IMPLEMENTATION

#include "App.h"
#include "gui/layout.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

bool app_init(App *app)
{
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
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!app->window)
        return false;

    app->gl_context = SDL_GL_CreateContext(app->window);
    if (!app->gl_context)
        return false;

    SDL_GetWindowSize(app->window, &app->win_width, &app->win_height);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glewExperimental = 1;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to setup GLEW\n");
        return false;
    }

    app->ctx = nk_sdl_init(app->window);

    {
        struct nk_font_atlas *atlas;
        nk_sdl_font_stash_begin(&atlas);
        nk_sdl_font_stash_end();
    }

    return true;
}

void app_run(App *app)
{
    bool running = true;
    struct nk_context *ctx = app->ctx;
    struct nk_colorf bg = {0.8f, 0.0f, 0.1f, 1.0f};

    while (running) {
        /* Input */
        SDL_Event evt;
        nk_input_begin(ctx);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT)
                return;
            nk_sdl_handle_event(&evt);
        }
        nk_input_end(ctx);

        draw_gui(app->ctx);

        SDL_GetWindowSize(app->window, &app->win_width, &app->win_height);
        glViewport(0, 0, app->win_width, app->win_height);

        nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
        SDL_GL_SwapWindow(app->window);
    }
}

void app_delete(App *app)
{
    nk_sdl_shutdown();
    SDL_GL_DeleteContext(app->gl_context);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
}
