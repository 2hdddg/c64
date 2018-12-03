#pragma once
#include <stdint.h>

#include "mem.h"

/* Controlled by phase two system clock. one Mhz
 * Timers decremented at 1 microsecond interval.
 * Depends on NTSC/PAL.
 */

void cia1_create();

void cia1_reset(); /* RES pin low */


/* PLA maps address space */
uint8_t cia1_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram);

void cia1_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram);

