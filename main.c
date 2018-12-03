#include <unistd.h>
#include <stdio.h>

#include "mem.h"
#include "cpu.h"
#include "cpu_port.h"
#include "pla.h"

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

    cpu_port_init();
    cpu_init(mem_get_for_cpu, mem_set_for_cpu,
             -1/*STDOUT_FILENO*/);
    mem_reset();
    printf("Powering on..\n");
    cpu_poweron();

    int num = 1500000;
    cpu_set_state(&state);
    while (num--) {
        cpu_step(&state);
    }

    //cpu_disassembly_at(STDOUT_FILENO, 0xff48, 10);

    return 0;
}

