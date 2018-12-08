#pragma once
#include <stdint.h>

#include "mem.h"
#include "keyboard.h"

/* MOS Complex interface adapter (CIA) 6526
 *
 * http://archive.6502.org/datasheets/mos_6526_cia_preliminary_nov_1981.pdf
 */

/* CIA registers */
#define CIA_REG_DATA_PORT_A           0x00
#define CIA_REG_DATA_PORT_B           0x01
#define CIA_REG_DATA_DIRECTION_PORT_A 0x02
#define CIA_REG_DATA_DIRECTION_PORT_B 0x03
#define CIA_REG_TIMER_A_LO            0x04
#define CIA_REG_TIMER_A_HI            0x05
#define CIA_REG_TIMER_B_LO            0x06
#define CIA_REG_TIMER_B_HI            0x07
#define CIA_REG_REAL_TIME_CLOCK_0_1S  0x08
#define CIA_REG_REAL_TIME_CLOCK_S     0x09
#define CIA_REG_REAL_TIME_CLOCK_M     0x0a
#define CIA_REG_REAL_TIME_CLOCK_H     0x0b
#define CIA_REG_SERIAL_SHIFT          0x0c
#define CIA_REG_INTERRUPT_CONTROL     0x0d
#define CIA_REG_TIMER_A_CONTROL       0x0e
#define CIA_REG_TIMER_B_CONTROL       0x0f

/* CIA interrupts */
#define CIA_INT_UNDERFLOW_TIMER_A     0x01
#define CIA_INT_UNDERFLOW_TIMER_B     0x02
#define CIA_INT_TIME_OF_DAY_ALARM     0x04
#define CIA_INT_SDR_FULL_OR_EMPTY     0x08
#define CIA_INT_FLAG_PIN              0x10
/* When reading, any interrupt above happened */
#define CIA_INT_OCCURED               0x80
/* When writing, mask is modified depending on bit 7 */
#define CIA_INT_MASK_SET              0x80

#define CIA1_ADDRESS 0xdc00

/* Controlled by phase two system clock. one Mhz
 * Timers decremented at 1 microsecond interval.
 * Depends on NTSC/PAL.
 */

void cia1_init();

void cia1_reset(); /* RES pin low */

void cia1_cycle();

/* PLA maps address space */
uint8_t cia1_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram);

void cia1_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram);

