#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "mem.h"

uint8_t _ram[65536];
uint8_t _color_ram[1000];


struct mem_hooks {
    mem_set_hook set_hook;
    mem_get_hook get_hook;
};

struct mem_hooks _cpu_hooks[256];


void mem_init()
{
    memset(_cpu_hooks, 0, sizeof(_cpu_hooks));
    memset(_ram, 0, 0xffff);
    memset(_color_ram, 0, 1000);
}

uint8_t* mem_get_ram_for_vic(uint16_t addr)
{
    return &_ram[addr];
}

uint8_t* mem_get_color_ram_for_vic()
{
    return _color_ram;
}

void mem_reset()
{
    memset(_ram, 0, 0xffff);
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

void mem_install_hooks_for_cpu(const struct mem_hook_install *install,
                               int num_install)
{
    while (num_install--) {
        uint8_t page_index = install->page_start;
        int     num_pages  = install->num_pages;

        while (num_pages--) {
            _cpu_hooks[page_index].set_hook = install->set_hook;
            _cpu_hooks[page_index].get_hook = install->get_hook;
            page_index++;
        }

        install++;
    }
}

void mem_color_ram_set(uint8_t val, uint16_t absolute,
                       uint8_t relative, uint8_t *ram)
{
    uint16_t offset = absolute - 0xd800;

    if (offset >= 1000) {
        printf("Color ofset!\n");
        return;
    }
    _color_ram[offset] = val;
}

uint8_t mem_color_ram_get(uint16_t absolute, uint8_t relative,
                          uint8_t *ram)
{
    uint16_t offset = absolute - 0xd800;

    if (offset >= 1000) {
        printf("Color ofset get!\n");
        return 0;
    }
    return _color_ram[offset];
}


void mem_dump_ram(int fd, uint16_t addr, uint16_t num)
{
    char text[8];
    for (int i = 0; i < num; i++) {
        sprintf(text, "%02x ", _ram[addr + i]);
        write(fd, text, 3);
        if ((i + 1) % 40 == 0) {
            write(fd, "\n", 1);
        }
    }
    write(fd, "\n", 1);
}

uint8_t* mem_get_ram(uint16_t addr)
{
    return &_ram[addr];
}

