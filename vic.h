#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "mem.h"

/* Registers */
#define VIC_REG_SCROLY 0x11
#define VIC_REG_SCROLX 0x16
#define VIC_REG_VMCSB  0x18
#define VIC_REG_EXTCOL 0x20
#define VIC_REG_BGCOL0 0x21

/* SCROLX flags and masks */
#define VIC_SCROLX_COL_40       0b00001000
#define VIC_SCROLX_MULTICOLOR   0b00010000
#define VIC_SCROLX_SCROLL       0b00000111
#define VIC_SCROLX_RESET        0b00100000

/* SCROLY flags and masks */
#define VIC_SCROLY_DISPLAY_EN   0b00010000
#define VIC_SCROLY_ROW_25       0b00001000
#define VIC_SCROLY_SCROLL       0b00000111

/* VMCSB masks */
#define VIC_VMCSB_CHAR_PIX_ADDR 0b00001110
#define VIC_VMCSB_VID_MATR_ADDR 0b11110000


void vic_init(uint8_t *char_rom,
              uint8_t *ram,
              uint8_t *color_ram);

void vic_screen(uint32_t *screen, uint32_t pitch);
void vic_reset();

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

void vic_step(bool *refresh, int *skip, bool *stall_cpu);

void vic_stat();
void vic_snapshot(const char *name);
