#pragma once
#include <stdint.h>

#include "mem.h"

uint8_t vic_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram);

void vic_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram);

