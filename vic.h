#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "mem.h"

void vic_init();

void vic_screen(uint32_t *screen, uint32_t pitch);

/* Values match CIA2 port A */
enum vic_bank {
    /* 0x0000 - 0x3fff */
    vic_bank_0 = 0b11,
    /* 0x4000 - 0x7fff */
    vic_bank_1 = 0b10,
    /* 0x8000 - 0xbfff */
    vic_bank_2 = 0b01,
    /* 0xc000 - 0xffff */
    vic_bank_3 = 0b00,
};

uint8_t vic_reg_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram);

void vic_reg_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram);

void vic_set_bank(enum vic_bank bank);
enum vic_bank vic_get_bank();

void vic_step(bool *refresh);

//void vic_trace_register_set(int fd);
//void vic_trace_register_get(int fd);
