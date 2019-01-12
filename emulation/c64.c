#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


#include "emulation/mem.h"
#include "emulation/cpu.h"
#include "emulation/cia1.h"
#include "emulation/cia2.h"
#include "emulation/vic.h"
#include "emulation/keyboard.h"
#include "emulation/sid.h"
#include "emulation/cpu_port.h"
#include "emulation/pla.h"

/* ROMs */
static uint8_t _basic_rom[8192];
static uint8_t _kernal_rom[8192];
static uint8_t _chargen_rom[4096];

static struct cpu_state _cpu_state;

static int _vic_skips = 0;
static bool _stall_cpu = false;

static bool load_rom(const char *path,
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

int c64_init(const char *rom_path)
{
    if (!load_rom("../rom/basic_v2.bin", _basic_rom, 8192) ||
        !load_rom("../rom/kernal_rev3.bin", _kernal_rom, 8192) ||
        !load_rom("../rom/chargen.bin", _chargen_rom, 4096)) {
        return -1;
    }

    mem_init();
    pla_init(_kernal_rom, _basic_rom, _chargen_rom);
    keyboard_init();
    sid_init();
    cpu_port_init();
    cpu_init(mem_get_for_cpu, mem_set_for_cpu);
    cia1_init();
    cia2_init();
    vic_init(_chargen_rom,
             mem_get_ram(0),
             mem_get_color_ram_for_vic());

    mem_reset();
    cia1_reset();
    cia2_reset();
    keyboard_reset();

    _cpu_state.pc = 0xfce2;
    cpu_set_state(&_cpu_state);

    return 0;
}

void c64_reset()
{
}

void c64_step()
{
    cia1_cycle();
    if (_vic_skips) {
        _vic_skips--;
    }
    else {
        vic_step(&_vic_skips, &_stall_cpu);
    }
    if (_vic_skips) {
        _vic_skips--;
    }
    else {
        vic_step(&_vic_skips, &_stall_cpu);
    }
    if (!_stall_cpu) {
        cpu_step(&_cpu_state);
    }
    else {
        _stall_cpu = false;
    }
}
