#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "mem.h"

uint8_t _ram[65536];

/* Write to banked ROM will write to underlying RAM */

int mem_reset()
{
    memset(_ram, 0, 0xffff);

    _ram[0] = 0xef;
    _ram[1] = 0x37;
    /* Points to IRQ handler */
    _ram[0xfffe] = 0x48;
    _ram[0xffff] = 0xff;

    /* ROM IRQ service routine */
    _ram[0xff48] = 0x48;
    _ram[0xff49] = 0x8a;
    _ram[0xff4a] = 0x48;
    _ram[0xff4b] = 0x98;
    _ram[0xff4c] = 0x48;
    _ram[0xff4d] = 0xba;
    _ram[0xff4e] = 0xbd;
    _ram[0xff4f] = 0x04;
    _ram[0xff50] = 0x01;
    _ram[0xff51] = 0x29;
    _ram[0xff52] = 0x10;
    _ram[0xff53] = 0xf0;
    _ram[0xff54] = 0x03;
    _ram[0xff55] = 0x6c;
    _ram[0xff56] = 0x16;
    _ram[0xff57] = 0x03;

    return 0;
}

void mem_set_for_cpu(uint16_t addr, uint8_t val)
{
    _ram[addr] = val;
}

uint8_t mem_get_for_cpu(uint16_t addr)
{
    /* TODO: Handle banking of ROM */
    return _ram[addr];
}

