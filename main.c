#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "trace.h"
#include "mem.h"
#include "cpu.h"
#include "cpu_port.h"
#include "pla.h"
#include "cia1.h"
#include "keyboard.h"
#include "vic.h"

uint8_t _basic_rom[8192];
uint8_t _kernal_rom[8192];
uint8_t _chargen_rom[4096];

int log_fd;

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

int setup(struct cpu_state *state)
{
    if (!load_rom("../rom/basic_v2.bin", _basic_rom, 8192) ||
        !load_rom("../rom/kernal_rev3.bin", _kernal_rom, 8192) ||
        !load_rom("../rom/chargen.bin", _chargen_rom, 4096)) {
        return -1;
    }

    mem_init();
    pla_init(_kernal_rom, _basic_rom, _chargen_rom);
    pla_trace_banks(STDOUT_FILENO);
    keyboard_init();

    cpu_port_init();
    cpu_init(mem_get_for_cpu, mem_set_for_cpu);
    mem_reset();
    cia1_init();
    cia1_reset();
    printf("Powering on..\n");
    cpu_poweron();
    keyboard_reset();
    state->pc = 0xfce2;
    cpu_set_state(state);

    vic_init();
    return 0;
}

int run_ncurses(struct cpu_state *state);

void just_run(struct cpu_state *state)
{
    //int num = 15000000;
    int num = 15;
    while (num--) {
        /* Should happen at approx 1Mhz */
        cpu_step(state);
        cia1_cycle();
    }
}

int main(int argc, char **argv)
{
    struct cpu_state state;

    trace_init();
    log_fd = open("./log", O_CREAT|O_TRUNC|O_WRONLY);
    int the_log = log_fd;

    if (setup(&state) != 0) {
        return -1;
    }

    trace_enable_point("VIC", "set reg", the_log);
/*
    trace_enable_point("KBD", "set port", the_log);
    trace_enable_point("KBD", "get port", the_log);
    trace_enable_point("KBD", "key", the_log);
    trace_enable_point("CIA1", "ERROR", the_log);
    trace_enable_point("CIA1", "set port", the_log);
    trace_enable_point("CIA1", "get port", the_log);
    trace_enable_point("CIA1", "timer", the_log);
    trace_enable_point("CPU", "execution", the_log);
*/

    if (argc)
        run_ncurses(&state);
    else
        just_run(&state);

    return 0;
}

