/* Implementation of PLA, programmable logic array.
 * Controls which chip that is connected to the
 * data bus.
 * This implementation modifies mem pages with hooks.
 */



#pragma once
#include <stdbool.h>

void pla_init();

void pla_pins_from_cpu(bool loram_high,
                       bool hiram_high,
                       bool charen_high);

