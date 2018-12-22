#include <time.h>
#include <stdbool.h>
#include <SDL.h>

#include "cpu.h"
#include "cia1.h"
#include "vic.h"

void sdl_c64_loop(struct cpu_state *state)
{
    struct timespec start, stop;
    SDL_Window *window = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    }

    window = SDL_CreateWindow("Commodore C64",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              420, 400,
                              0 /*SDL_WINDOW_FULLSCREEN*/);

    SDL_Surface *surface = SDL_GetWindowSurface(window);

    bool end = false;
    SDL_Event event;

    if (surface->format->format != SDL_PIXELFORMAT_RGB888) {
        printf("%s\n", SDL_GetPixelFormatName(surface->format->format));
        return;
    }

    vic_screen(surface->pixels, surface->pitch);

    bool refresh = false;

    clock_gettime(CLOCK_MONOTONIC, &start);
    while (!end) {
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                end = true;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    end = true;
                    break;
                }
                break;
            }
        }

        cia1_cycle();
        cpu_step(state);
        vic_step(&refresh);

        if (refresh) {
            clock_gettime(CLOCK_MONOTONIC, &stop);
            SDL_UpdateWindowSurface(window);
            printf("Num ms %ld\n", (stop.tv_nsec - start.tv_nsec)/1000000);
            clock_gettime(CLOCK_MONOTONIC, &start);
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
}
