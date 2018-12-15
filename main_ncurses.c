#include <time.h>
#include <stdlib.h>

#include <menu.h>
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
            uint8_t c = *screen;
            addch(ascii_screen[c] | (c >= 0x80 ? A_REVERSE : 0));
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
        if (ch == 27) {
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

#if 0
MENU* create_trace_menu()
{
    ITEM **items;
    int num_traces;
    int i;
    struct trace_point *trace;
    char *text;
    MENU *menu;

    num_traces = trace_count_points();
    items = calloc(num_traces + 1, sizeof(ITEM*));

    i = 0;
    trace = trace_enum_points(NULL);
    while (trace != NULL) {
        bool is_on = trace->fd != -1;
        text = malloc(100);
        sprintf(text, "%s %s",
                trace->sys, trace->name);
        items[i] = new_item(text, is_on ? "on" : "off");
        set_item_value(items[i], is_on);
        trace = trace_enum_points(trace);
        i++;
    }
    items[i] = NULL;

    menu = new_menu(items);
    menu_opts_off(menu, O_ONEVALUE);

    return menu;
}
#endif
/* TODO: Free menu */

typedef void (*menu_handler)();

struct menu_choice {
    const char   *text;
    menu_handler handler;
};

static void set_fd(int fd)
{
    struct trace_point *trace;

    trace = trace_enum_points(NULL);
    while (trace != NULL) {
        trace->fd = fd;
        trace = trace_enum_points(trace);
    }
}

static int _log_fd;

static void turn_on_all()
{
    set_fd(_log_fd);
}

static void turn_off_all()
{
    set_fd(-1);
}

static struct menu_choice _main_menu_choices[] = {
    {
        .text = "Trace: Turn on all",
        .handler = turn_on_all,
    },
    {
        .text = "Trace: Turn off all",
        .handler = turn_off_all,
    },
};
static int _num_main_menu_choices = 2;

MENU* create_main_menu()
{
    ITEM **items;
    int i;

    items = calloc(_num_main_menu_choices + 1, sizeof(ITEM*));

    for (i = 0; i < _num_main_menu_choices; i++) {
        items[i] = new_item(_main_menu_choices[i].text, NULL);
        set_item_userptr(items[i], _main_menu_choices[i].handler);
    }
    items[i] = NULL;

    return new_menu(items);
}

void run_menu_loop(bool *exit)
{
    int ch = 0;
    //MENU *trace_menu = create_trace_menu();
    MENU *main_menu = create_main_menu();
    MENU *curr_menu = main_menu;

    post_menu(main_menu);
    refresh();

    while (true) {
        ch = getch();

        /* Check for escape to go back to c64, escape
         * is sent for arrow keys and other keys as well,
         * but when true ESC, first 27 and then -1 */
        if (ch == 27) {
            ch = getch();
            if (ch == -1) {
                /* Escape go back to C64 */
                return;
            }
            /* For some reason we need to translate these... */
            ch = getch();
            switch (ch) {
            case 0x41:
                ch = KEY_UP;
                break;
            case 0x42:
                ch = KEY_DOWN;
                break;
            case 0x43:
                ch = KEY_RIGHT;
                break;
            case 0x44:
                ch = KEY_LEFT;
                break;
            default:
                break;
            }
        }

        switch (ch) {
        case 'q':
        case 'Q':
            *exit = true;
            return;
        case 'j':
        case 'J':
        case KEY_DOWN:
            menu_driver(curr_menu, REQ_DOWN_ITEM);
            break;
        case 'k':
        case 'K':
        case KEY_UP:
            menu_driver(curr_menu, REQ_UP_ITEM);
            break;
        case 10: {
                /* Enter */
                ITEM *curr = current_item(curr_menu);
                menu_handler handler = item_userptr(curr);
                handler();
            }
            break;
        default:
            continue;
        }

    }
}

int run_ncurses(struct cpu_state *state, int log_fd)
{
    WINDOW *win = initscr();
    //raw();
    noecho();
    //keypad(stdscr, TRUE);
    nodelay(win, TRUE);
    bool exit = false;

    _log_fd = log_fd;

    while (!exit) {
        run_c64_loop(state);
        run_menu_loop(&exit);
    }

    endwin();

    return 0;
}

