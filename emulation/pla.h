/* Implementation of PLA, programmable logic array.
 *
 * Controls which chip that is connected to the data bus.
 *
 * This implementation modifies mem pages with hooks.
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

void pla_init(uint8_t *rom_kernal,
              uint8_t *rom_basic,
              uint8_t *rom_chargen);

void pla_reset();

/* Pins from CPU address 0/1 that affects memory layout. */
void pla_pins_from_cpu(bool loram_high,
                       bool hiram_high,
                       bool charen_high);

/* For debugging */
bool pla_is_basic_mapped();
bool pla_is_kernal_mapped();

/* For debugging */
void pla_stat();
