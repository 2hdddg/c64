#pragma once
#include <stdint.h>

#include "mem.h"


void cia2_init();

uint8_t cia2_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram);

void cia2_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram);

