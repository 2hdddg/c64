#include <time.h>
#include <ncurses.h>

#include "petscii.h"
#include "keyboard.h"
#include "cpu.h"
#include "cia1.h"
#include "mem.h"

static uint16_t map_key(int ch, uint16_t *extra)
{
    *extra = 0;

    switch (ch) {
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

    case '!':
        *extra = KEYB_LEFT_SHIFT;
        return KEYB_EXCLAMATION;

    case '"':
        *extra = KEYB_LEFT_SHIFT;
        return KEYB_QUOTE;

    case '#':
        *extra = KEYB_LEFT_SHIFT;
        return KEYB_POUND;

    case '%':
        *extra = KEYB_LEFT_SHIFT;
        return KEYB_PERCENT;

    case '&':
        *extra = KEYB_LEFT_SHIFT;
        return KEYB_AMP;

    case ' ':
        return KEYB_SPACE;

    /* Enter */
    case 10:
        return KEYB_RETURN;

    default:
        return 0;
    }
}

static void draw_screen(uint8_t *screen)
{
    for (int y = 0; y < 25; y++) {
        mvaddch(y, 0, ' ');
        for (int x = 0; x < 40; x++) {
            addch(ascii_screen[*screen]);
            screen++;
        }
    }
    refresh();
}

static void run_c64_loop(struct cpu_state *state)
{
    struct timespec start, stop;
    int num_key_downs = 0;
    uint16_t key = 0;
    uint16_t extra_key = 0;
    int ch;

    while (true) {
        clock_gettime(CLOCK_MONOTONIC, &start);

        cia1_cycle();
        cpu_step(state);

        ch = getch();
        if (ch == KEY_F(2)) {
            break;
        }

        if (num_key_downs == 0) {
            key = map_key(ch, &extra_key);
            if (key) {
                num_key_downs = 19000;
                keyboard_down(key);
                if (extra_key) {
                    keyboard_down(extra_key);
                }
            }
        }
        else {
            num_key_downs--;
            if (num_key_downs == 0 && key) {
                keyboard_up(key);
                if (extra_key) {
                    keyboard_up(extra_key);
                }
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &stop);
        if ((stop.tv_nsec - start.tv_nsec) > 100*25) {
            start = stop;
            draw_screen(mem_get_ram(0x0400));
        }
    }
}

int run_ncurses(struct cpu_state *state)
{
    WINDOW *win = initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);

    nodelay(win, TRUE);
    run_c64_loop(state);

    endwin();

    return 0;
}

