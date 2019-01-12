#include <time.h>
#include <stdbool.h>
#include <SDL.h>

#include "c64.h"
#include "vic.h"
#include "keyboard.h"

static struct timespec _start, _stop;
static struct SDL_Window *_window;

static uint16_t map_key(SDL_Keycode sym)
{
    switch (sym) {
    case 'q':
    case 'Q':
        return KEYB_Q;

    case 'w':
    case 'W':
        return KEYB_W;

    case 'e':
    case 'E':
        return KEYB_E;

    case 'r':
    case 'R':
        return KEYB_R;

    case 't':
    case 'T':
        return KEYB_T;

    case 'y':
    case 'Y':
        return KEYB_Y;

    case 'u':
    case 'U':
        return KEYB_U;

    case 'i':
    case 'I':
        return KEYB_I;

    case 'o':
    case 'O':
        return KEYB_O;

    case 'p':
    case 'P':
        return KEYB_P;

    case 'a':
    case 'A':
        return KEYB_A;

    case 's':
    case 'S':
        return KEYB_S;

    case 'd':
    case 'D':
        return KEYB_D;

    case 'f':
    case 'F':
        return KEYB_F;

    case 'g':
    case 'G':
        return KEYB_G;

    case 'h':
    case 'H':
        return KEYB_H;

    case 'j':
    case 'J':
        return KEYB_J;

    case 'k':
    case 'K':
        return KEYB_K;

    case 'l':
    case 'L':
        return KEYB_L;

    case 'z':
    case 'Z':
        return KEYB_Z;

    case 'x':
    case 'X':
        return KEYB_X;

    case 'c':
    case 'C':
        return KEYB_C;

    case 'v':
    case 'V':
        return KEYB_V;

    case 'b':
    case 'B':
        return KEYB_B;

    case 'n':
    case 'N':
        return KEYB_N;

    case 'm':
    case 'M':
        return KEYB_M;

    case '0':
        return KEYB_0;

    case '1':
        return KEYB_1;

    case '2':
        return KEYB_2;

    case '3':
        return KEYB_3;

    case '4':
        return KEYB_4;

    case '5':
        return KEYB_5;

    case '6':
        return KEYB_6;

    case '7':
        return KEYB_7;

    case '8':
        return KEYB_8;

    case '9':
        return KEYB_9;

    case ' ':
        return KEYB_SPACE;

    case ',':
        return KEYB_COMMA;

    case ':':
        return KEYB_COLON;

    case '*':
        return KEYB_STAR;

    case '+':
        return KEYB_PLUS;

    case '-':
        return KEYB_MINUS;

    case '=':
        return KEYB_EQ;

    case SDLK_RETURN:
        return KEYB_RETURN;

    case SDLK_BACKSPACE:
        return KEYB_DEL;

    case SDLK_LSHIFT:
        return KEYB_LEFT_SHIFT;

    case SDLK_RSHIFT:
        return KEYB_RIGHT_SHIFT;

    case SDLK_LALT:
        return KEYB_COMMODORE;
    }

    return 0;
}

static void do_refresh()
{
    clock_gettime(CLOCK_MONOTONIC, &_stop);
    SDL_UpdateWindowSurface(_window);
    //printf("Num ms %ld\n", (_stop.tv_nsec - start.tv_nsec)/1000000);
    clock_gettime(CLOCK_MONOTONIC, &_start);
}

void sdl_c64_loop()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return;
    }

    _window = SDL_CreateWindow("Commodore C64",
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               520, 520,
                               0 /*SDL_WINDOW_FULLSCREEN*/);

    SDL_Surface *surface = SDL_GetWindowSurface(_window);

    bool end = false;
    SDL_Event event;

    if (surface->format->format != SDL_PIXELFORMAT_RGB888) {
        //printf("%s\n", SDL_GetPixelFormatName(surface->format->format));
        return;
    }

    vic_screen(surface->pixels, surface->pitch);
    vic_set_refresh_hook(do_refresh);

    uint16_t key;

    clock_gettime(CLOCK_MONOTONIC, &_start);
    while (!end) {
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                end = true;
                break;
            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    end = true;
                    break;
                default:
                    key = map_key(event.key.keysym.sym);
                    if (key) {
                        keyboard_down(key);
                    }
                }
                break;
            }
            case SDL_KEYUP: {
                default:
                    key = map_key(event.key.keysym.sym);
                    if (key) {
                        keyboard_up(key);
                    }
                }
                break;
            }
        }

        c64_step();
    }
    vic_snapshot("./snap.png");

    SDL_DestroyWindow(_window);
    SDL_Quit();
}
