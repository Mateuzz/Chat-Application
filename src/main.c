#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <unistd.h>
#include <stdbool.h>

int main(void)
{
    bool running = true;

    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window *window = SDL_CreateWindow("Chat", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 720, SDL_WINDOW_RESIZABLE);

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                running = false;
                break;
            }
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
}
