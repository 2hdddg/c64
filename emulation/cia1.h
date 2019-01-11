#pragma once
#include <stdint.h>

#include "cia.h"

#define CIA1_ADDRESS 0xdc00

void cia1_init();
void cia1_reset(); /* RES pin low */
void cia1_cycle();

/* PLA maps address space */
uint8_t cia1_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram);

void cia1_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram);

