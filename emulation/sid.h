#pragma once
#include <stdint.h>

#include "mem.h"

void sid_init();

uint8_t sid_reg_get(uint16_t absolute, uint8_t *ram);
void sid_reg_set(uint8_t val, uint16_t absolute, uint8_t *ram);

