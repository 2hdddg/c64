#pragma once

#include <stdint.h>

/* Mem access is 2Mhz. Interleaved between CPU and VIC.*/


void mem_init();
void mem_reset();

/* CPU memory API */
uint8_t mem_get_for_cpu(uint16_t addr);
void mem_set_for_cpu(uint16_t addr,
                     uint8_t val);

/* VIC memory API */
uint8_t mem_get_for_vic(uint16_t addr);
void mem_set_for_vic(uint16_t addr,
                     uint8_t val);


typedef void (*mem_set_hook)(uint8_t val, uint16_t absolute,
                             uint8_t relative, uint8_t *ram);
typedef uint8_t (*mem_get_hook)(uint16_t absolute, uint8_t relative,
                                uint8_t *ram);


struct mem_hook_install {
    mem_set_hook set_hook;
    mem_get_hook get_hook;
    uint8_t      page_start;
    uint8_t      num_pages;
};

void mem_install_hooks_for_cpu(const struct mem_hook_install *install,
                               int num_install);

void mem_dump_ram(int fd, uint16_t addr, uint16_t num);
void mem_dump_ram_as_text(int fd, uint16_t addr,
                          uint8_t cols, uint8_t rows);
void mem_get_ram(uint16_t addr, uint16_t num, uint8_t *out);

