/* MOS Complex interface adapter (CIA) 6526
 *
 * http://archive.6502.org/datasheets/mos_6526_cia_preliminary_nov_1981.pdf
 *
 * Controlled by phase two system clock. one Mhz
 * Timers decremented at 1 microsecond interval.
 * Depends on NTSC/PAL.
 *
 * The C64 has two CIAs.
 */
#pragma once

#include <stdint.h>

#include "cia_timer.h"

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

typedef uint8_t (*cia_get_peripheral)(uint8_t interesting_bits);
typedef void (*cia_set_peripheral)(uint8_t val, uint8_t valid_bits);
typedef void (*cia_interrupt)();

struct cia_state {
    uint8_t interrupt_data;
    uint8_t interrupt_mask;

    uint8_t data_direction_port_A;
    uint8_t data_direction_port_B;

    /* For outgoing data */
    uint8_t data_port_A;
    uint8_t data_port_B;

    struct cia_timer timer_A;
    uint8_t          timer_A_raw;
    struct cia_timer timer_B;
    uint8_t          timer_B_raw;

    /* Callbacks */
    cia_get_peripheral on_get_peripheral_A;
    cia_get_peripheral on_get_peripheral_B;
    cia_set_peripheral on_set_peripheral_A;
    cia_set_peripheral on_set_peripheral_B;
    cia_interrupt      on_interrupt;

    /* Debugging */
    struct trace_point *trace_set_port;
    struct trace_point *trace_get_port;
    struct trace_point *trace_timer;
    struct trace_point *trace_error;
};

void cia_reset(struct cia_state *state);

void cia_cycle(struct cia_state *state);

void cia_set_register(struct cia_state *state,
                      uint8_t reg, uint8_t val);
uint8_t cia_get_register(struct cia_state *state,
                         uint8_t reg);

