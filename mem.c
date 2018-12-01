#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "mem.h"

uint8_t _ram[65536];


struct mem_hooks {
    mem_set_hook set_hook;
    mem_get_hook get_hook;
};

struct mem_hooks _cpu_hooks[256];



void mem_init()
{
    memset(_cpu_hooks, 0, sizeof(_cpu_hooks));
    memset(_ram, 0, 0xffff);
}


void mem_reset()
{

    //_ram[0] = 0xef;
    //_ram[1] = 0x37;
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
}

void mem_set_for_cpu(uint16_t addr, uint8_t val)
{
    uint8_t          page   = addr >> 8;
    struct mem_hooks *hooks = &_cpu_hooks[page];

    if (hooks->set_hook) {
        hooks->set_hook(val, addr, addr & 0xff, &_ram[addr]);
    }
    else {
        _ram[addr] = val;
    }
}

uint8_t mem_get_for_cpu(uint16_t addr)
{
    uint8_t          page   = addr >> 8;
    struct mem_hooks *hooks = &_cpu_hooks[page];

    if (hooks->get_hook) {
        return hooks->get_hook(addr, addr & 0xff, &_ram[addr]);
    }
    else {
        return _ram[addr];
    }
}

void mem_install_hooks_for_cpu(struct mem_hook_install *install,
                               int num_install)
{
    while (num_install--) {
        uint8_t page_index = install->page_start;
        int     last_page = page_index + install->num_pages;

        if (last_page > 255) {
            printf("Too many pages\n");
        }

        for (; page_index < last_page; page_index++) {
            _cpu_hooks[page_index].set_hook = install->set_hook;
            _cpu_hooks[page_index].get_hook = install->get_hook;
        }

        install++;
    }
}

