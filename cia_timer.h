#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Kind of input that decrements timer. */
typedef enum {
    clock_cycle,
    pin_CNT,
    /* Only applicable for timer B */
    timer_A_underflow,
    timer_A_underflow_pin_CNT,
} cia_timer_input_mode;

struct cia_timer {
    /* Latch configuration */
    uint8_t latch_hi;
    uint8_t latch_lo;

    /* Current values */
    uint8_t timer_hi;
    uint8_t timer_lo;

    /* Configuration */
    bool started;
    bool port_B_on;
    bool port_B_toggle; /* Otherwise 1 cycle pulse */
    bool one_shot;
    cia_timer_input_mode input;

    /* True if last cycle caused underflow */
    bool underflowed;
};

/* Control flags */
#define CIA_TIMER_CTRL_START         0x01
#define CIA_TIMER_CTRL_PORT_B_ON     0x02
#define CIA_TIMER_CTRL_PORT_B_TOGGLE 0x04
#define CIA_TIMER_CTRL_ONE_SHOT      0x08
#define CIA_TIMER_CTRL_LOAD_LATCH    0x10
/* Only timer A */
#define CIA_TIMER_A_CTRL_INPUT_CNT   0x20

void cia_timer_reset(struct cia_timer *timer);
void cia_timer_set_latch_lo(struct cia_timer *timer, uint8_t lo);
void cia_timer_set_latch_hi(struct cia_timer *timer, uint8_t hi);

void cia_timer_control_A(struct cia_timer *timer,
                         uint8_t control);
void cia_timer_cycle(struct cia_timer *timer_A,
                     struct cia_timer *timer_B);

