#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "mem.h"
#include "cpu.h"
#include "cpu_port.h"
#include "pla.h"
#include "cia1.h"
#include "keyboard.h"

uint8_t _basic_rom[8192];
uint8_t _kernal_rom[8192];
uint8_t _chargen_rom[4096];

bool load_rom(const char *path,
              uint8_t *rom_out, uint16_t size)
{
    FILE *f = fopen(path, "rb");
    uint16_t read;

    printf("Loading ROM: %s\n", path);

    if (f != NULL) {
        read = fread(rom_out, 1, size, f);
        if (read != size) {
            printf("Failed to read all bytes, got %d\n", read);
            return false;
        }
        return true;
    }
    else
        printf("Failed to read rom at %s\n", path);

    return false;
}
#include <ncurses.h>
#include "petscii.h"

uint16_t map_key(int ch, uint16_t *extra)
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

int run_console(struct cpu_state *state)
{
    struct timespec start, stop;

    WINDOW *win = initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    int ch;
    uint8_t screen_ram[25*40];

    int num_key_downs = 0;
    uint16_t key = 0;
    uint16_t extra_key = 0;

    nodelay(win, TRUE);
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
            mem_get_ram(0x0400, 25*40, screen_ram);
            uint8_t *n = screen_ram;
            for (int y = 0; y < 25; y++) {
                mvaddch(y, 0, ' ');
                for (int x = 0; x < 40; x++) {
                    addch(ascii_screen[*n]);
                    n++;
                }
            }
            refresh();
        }

    }

    endwin();

    return 0;
}

int main(int argc, char **argv)
{
    struct cpu_state state = {
        .pc = 0xfce2,
    };

    if (!load_rom("../rom/basic_v2.bin", _basic_rom, 8192) ||
        !load_rom("../rom/kernal_rev3.bin", _kernal_rom, 8192) ||
        !load_rom("../rom/chargen.bin", _chargen_rom, 4096)) {
        return -1;
    }

    mem_init();
    pla_init(_kernal_rom, _basic_rom, _chargen_rom);
    pla_trace_banks(STDOUT_FILENO);
    cia1_init();

    cpu_port_init();
    cpu_init(mem_get_for_cpu, mem_set_for_cpu,
             -1/*STDOUT_FILENO*/);
    mem_reset();
    cia1_reset();
    printf("Powering on..\n");
    cpu_poweron();
    keyboard_reset();
    cpu_set_state(&state);
    /*
    keyboard_trace_keys(STDOUT_FILENO);
    keyboard_trace_port_set(STDOUT_FILENO);
    keyboard_trace_port_get(STDOUT_FILENO);
    */
#if 0
    int num = 15000000;
    cpu_set_state(&state);
    while (num--) {
        /* Should happen at approx 1Mhz */
        cia1_cycle();
/*
        if (num == 10000) {
            keyboard_down(KEY_S);
        }
        if (num == 7000) {
            keyboard_up(KEY_S);
        }
*/
        cpu_step(&state);
    }
    mem_dump_ram_as_text(STDOUT_FILENO, 0x0400, 40, 25);
#endif

#if 0
    num = 150000;
    while (num--) {
        /* Should happen at approx 1Mhz */
        cia1_cycle();
        cpu_step(&state);
    }
    mem_dump_ram_as_text(STDOUT_FILENO, 0x0400, 40, 25);
#endif

    //mem_dump_ram(STDOUT_FILENO, 0x0400, 40*25);

    //cpu_disassembly_at(STDOUT_FILENO, 0xff48, 10);
    run_console(&state);

    return 0;
}

