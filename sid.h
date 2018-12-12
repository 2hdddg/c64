#pragma once
#include <stdint.h>

#include "mem.h"

void sid_init();

uint8_t sid_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram);

void sid_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram);

