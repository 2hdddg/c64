#include <string.h>
#include <stdio.h>

#include "cia_timer.h"


void cia_timer_reset(struct cia_timer *timer)
{
    memset(&timer, 0, sizeof(*timer));
}

void cia_timer_load_latch(struct cia_timer *timer)
{
    timer->timer_hi = timer->latch_hi;
    timer->timer_lo = timer->latch_lo;
}

void cia_timer_set_latch_lo(struct cia_timer *timer, uint8_t lo)
{
    timer->latch_lo = lo;
}

void cia_timer_set_latch_hi(struct cia_timer *timer, uint8_t hi)
{
    timer->latch_hi = hi;
    if (!timer->started) {
        cia_timer_load_latch(timer);
    }
}

void cia_timer_control_A(struct cia_timer *timer,
                         uint8_t control)
{
    if (control & 0x10) {
        cia_timer_load_latch(timer);
    }
    timer->started = (control & 0x01) > 0;
    timer->port_B_on = (control & 0x02) > 0;
    timer->port_B_toggle = (control & 0x04) > 0;
    timer->one_shot = (control & 0x08) > 0;
    timer->input = control & 0x20 ?  pin_CNT : clock_cycle;

    if (timer->started) {
        printf("Started %s timer: %02x%02x\n",
               timer->one_shot ? "one shot" : "continous",
               timer->timer_hi, timer->timer_lo);
    }
}

/* Returns true if underflowed */
bool cia_timer_cycle_A(struct cia_timer *timer_A,
                       struct cia_timer *timer_B)
{
    bool underflowed = false;

    if (timer_A->started) {
        bool count;

        switch(timer_A->input) {
        case clock_cycle:
            count = true;
            break;
        default:
            count = false;
            printf("Unhandled timer input\n");
            break;
        }

        if (count) {
            timer_A->timer_lo--;
            if (timer_A->timer_lo == 0xff) {
                timer_A->timer_hi--;
                if (timer_A->timer_hi == 0xff) {
                    cia_timer_load_latch(timer_A);
                    underflowed = true;
                    timer_A->started = !timer_A->one_shot;
                }
            }
        }
    }
    return underflowed;
}

