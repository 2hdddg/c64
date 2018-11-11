#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "mem.h"

struct mem_h {
    uint8_t ram[65536];
};

/* Write to banked ROM will write to underlying RAM */

int mem_create(struct mem_h **mem_h_out)
{
    struct mem_h *mem_h;

    *mem_h_out = NULL;

    mem_h = calloc(1, sizeof(*mem_h));
    if (!mem_h) {
        return -1;
    }

    *mem_h_out = mem_h;
    return 0;
}

int mem_reset(struct mem_h *mem_h)
{
    uint8_t *ram = mem_h->ram;

    memset(ram, 0, 0xffff);

    ram[0] = 0xef;
    ram[1] = 0x37;
    /* Points to IRQ handler */
    ram[0xfffe] = 0x48;
    ram[0xffff] = 0xff;

    /* ROM IRQ service routine */
    ram[0xff48] = 0x48;
    ram[0xff49] = 0x8a;
    ram[0xff4a] = 0x48;
    ram[0xff4b] = 0x98;
    ram[0xff4c] = 0x48;
    ram[0xff4d] = 0xba;
    ram[0xff4e] = 0xbd;
    ram[0xff4f] = 0x04;
    ram[0xff50] = 0x01;
    ram[0xff51] = 0x29;
    ram[0xff52] = 0x10;
    ram[0xff53] = 0xf0;
    ram[0xff54] = 0x03;
    ram[0xff55] = 0x6c;
    ram[0xff56] = 0x16;
    ram[0xff57] = 0x03;

    return 0;
}

void mem_set(struct mem_h *mem_h, uint16_t addr, uint8_t val)
{
    mem_h->ram[addr] = val;
}

uint8_t mem_get(struct mem_h *mem_h, uint16_t addr)
{
    /* TODO: Handle banking of ROM */
    return mem_h->ram[addr];
}

void mem_destroy(struct mem_h *mem_h)
{
    free(mem_h);
}

