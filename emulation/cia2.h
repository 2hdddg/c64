#pragma once
#include <stdint.h>

#include "mem.h"

#define CIA2_ADDRESS 0xdd00

void cia2_init();
void cia2_reset();
void cia2_cycle();

uint8_t cia2_reg_get(uint16_t absolute, uint8_t *ram);

void cia2_reg_set(uint8_t val, uint16_t absolute, uint8_t *ram);

