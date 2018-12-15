#include <string.h>
#include <stdio.h>

#include "cia_timer.h"


static void load_latch(struct cia_timer *timer)
{
    timer->timer_hi = timer->latch_hi;
    timer->timer_lo = timer->latch_lo;
}

void cia_timer_reset(struct cia_timer *timer)
{
    memset(timer, 0, sizeof(*timer));
}

void cia_timer_set_latch_lo(struct cia_timer *timer, uint8_t lo)
{
    timer->latch_lo = lo;
}

void cia_timer_set_latch_hi(struct cia_timer *timer, uint8_t hi)
{
    timer->latch_hi = hi;
    if (!timer->started) {
        load_latch(timer);
    }
}

void cia_timer_control_A(struct cia_timer *timer,
                         uint8_t control)
{
    if (control & CIA_TIMER_CTRL_LOAD_LATCH) {
        load_latch(timer);
    }
    timer->started       = (control & CIA_TIMER_CTRL_START) > 0;
    timer->port_B_on     = (control & CIA_TIMER_CTRL_PORT_B_ON) > 0;
    timer->port_B_toggle = (control & CIA_TIMER_CTRL_PORT_B_TOGGLE) > 0;
    timer->one_shot      = (control & CIA_TIMER_CTRL_ONE_SHOT) > 0;
    timer->input         = (control & CIA_TIMER_A_CTRL_INPUT_CNT) ?
                            pin_CNT : clock_cycle;
    if (timer->started) {
        TRACE(timer->trace, "A: started %02x%03x, %s",
              timer->timer_hi, timer->timer_lo,
              timer->one_shot ? "one shot" : "continous");
    }
}

void cia_timer_control_B(struct cia_timer *timer,
                         uint8_t control)
{
    uint8_t input = (control & 0b01100000) >> 5;

    if (control & CIA_TIMER_CTRL_LOAD_LATCH) {
        load_latch(timer);
    }
    timer->started       = (control & CIA_TIMER_CTRL_START) > 0;
    timer->port_B_on     = (control & CIA_TIMER_CTRL_PORT_B_ON) > 0;
    timer->port_B_toggle = (control & CIA_TIMER_CTRL_PORT_B_TOGGLE) > 0;
    timer->one_shot      = (control & CIA_TIMER_CTRL_ONE_SHOT) > 0;

    switch (input) {
    case 0b00:
        timer->input = clock_cycle;
        break;
    case 0b01:
        timer->input = pin_CNT;
        break;
    case 0b10:
        timer->input = timer_A_underflow;
        break;
    case 0b11:
        timer->input = timer_A_underflow_pin_CNT;
        break;
    }

    if (timer->started) {
        TRACE(timer->trace, "B: started %02x%03x, %s",
              timer->timer_hi, timer->timer_lo,
              timer->one_shot ? "one shot" : "continous");
    }
}

void cia_timer_cycle(struct cia_timer *timer_A,
                     struct cia_timer *timer_B)
{
    timer_A->underflowed = false;
    if (timer_A->started) {
        bool count = false;

        switch(timer_A->input) {
        case clock_cycle:
            count = true;
            break;
        default:
            TRACE(timer_A->trace, "A: unhandled timer input: %d\n",
                  timer_A->input);
            break;
        }

        if (count) {
            timer_A->timer_lo--;
            if (timer_A->timer_lo == 0xff) {
                timer_A->timer_hi--;
                if (timer_A->timer_hi == 0xff) {
                    load_latch(timer_A);
                    timer_A->underflowed = true;
                    timer_A->started = !timer_A->one_shot;
                }
            }
        }
    }

    if (timer_B->started) {
        bool count = false;

        switch (timer_B->input) {
        case clock_cycle:
            count = true;
            break;
        case timer_A_underflow:
            count = timer_A->underflowed;
            break;
        default:
            TRACE(timer_B->trace, "B: unhandled timer input: %d\n",
                  timer_B->input);
        }

        if (count) {
            timer_B->timer_lo--;
            if (timer_B->timer_lo == 0xff) {
                timer_B->timer_hi--;
                if (timer_B->timer_hi == 0xff) {
                    load_latch(timer_B);
                    timer_B->underflowed = true;
                    timer_B->started = !timer_B->one_shot;
                }
            }
        }
    }
}

