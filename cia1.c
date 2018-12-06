#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cia1.h"
#include "cpu.h"

uint8_t _interrupt_data;
uint8_t _interrupt_mask;
uint8_t _data_direction_port_A;
uint8_t _data_direction_port_B;

typedef enum {
    clock_cycle,
    pin_CNT,
    /* Only applicable for timer B */
    timer_A_underflow,
    timer_A_underflow_pin_CNT,
} input_mode;

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
    input_mode input;

    bool underflowed;
};


struct cia_timer _timer_A;
struct cia_timer _timer_B;


static void load_latch(struct cia_timer *timer)
{
    timer->timer_hi = timer->latch_hi;
    timer->timer_lo = timer->latch_lo;
}

static void set_latch_lo(struct cia_timer *timer, uint8_t lo)
{
    timer->latch_lo = lo;
}

static void set_latch_hi(struct cia_timer *timer, uint8_t hi)
{
    timer->latch_hi = hi;
    if (!timer->started) {
        load_latch(timer);
    }
}

static void control_timer_A(struct cia_timer *timer,
                            uint8_t control)
{
    if (control & 0x10) {
        load_latch(timer);
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

static void control_interrupts(uint8_t control)
{
    if (control & 0x80) {
        /* Ones in control sets corresponding bits in mask,
         * zeroes in control shouldn't affect mask. */
        _interrupt_mask |= (control & 0x7f);
    }
    else {
        /* Ones in control clears corresponding bits in mask,
         * zeroes in control shouldn't affect mask. */
        _interrupt_mask &= ~control;
    }
}

void cia1_init()
{
    _interrupt_data = 0x00;
    _interrupt_mask = 0x00;
}

void cia1_reset()
{
    memset(&_timer_A, 0, sizeof(_timer_A));
    memset(&_timer_B, 0, sizeof(_timer_B));
}

void cia1_cycle()
{
    bool underflowed = false;

    if (_timer_A.started) {
        bool count;

        switch(_timer_A.input) {
        case clock_cycle:
            count = true;
            break;
        default:
            count = false;
            printf("Unhandled timer input\n");
            break;
        }

        if (count) {
            _timer_A.timer_lo--;
            if (_timer_A.timer_lo == 0xff) {
                _timer_A.timer_hi--;
                if (_timer_A.timer_hi == 0xff) {
                    load_latch(&_timer_A);
                    underflowed = true;
                    _timer_A.started = !_timer_A.one_shot;
                }
            }
        }
    }

    /* Generate interrupt */
    if (underflowed) {
        //printf("Timer underflowed\n");
        if (_timer_A.port_B_on) {
            /* TODO: */
        }

        _interrupt_data |= 0x01;
    }

    /* Update interrupt status */
    if (_interrupt_data & _interrupt_mask) {
        _interrupt_data |= 0x80;
        /* TODO: IRQ or NMI (cia2) */
        cpu_interrupt_request();
    }
}

uint8_t cia1_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    /* Registers are mirrored at each 16 bytes */
    uint8_t reg = (absolute - 0xdc00) % 0x10;
    uint8_t val;

    switch (reg) {
    case 0x02:
        return _data_direction_port_A;
    case 0x03:
        return _data_direction_port_B;
    case 0x04:
        return _timer_A.timer_lo;
    case 0x05:
        return _timer_A.timer_hi;
    case 0x0d:
        printf("Reading & clearing interrupt status\n");
        val = _interrupt_data;
        /* Cleared on read */
        _interrupt_data = 0x00;
        /* No more interrupt */
        return val;
    default:
        //printf("Reading unhandled CIA 1 register: %02x\n", reg);
        break;
    }

    return 0;
}

void cia1_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
    /* Registers are mirrored at each 16 bytes */
    uint8_t reg = (absolute - 0xdc00) % 0x10;

    switch (reg) {
    case 0x02:
        _data_direction_port_A = val;
        break;
    case 0x03:
        _data_direction_port_B = val;
        break;
    case 0x04:
        set_latch_lo(&_timer_A, val);
        break;
    case 0x05:
        set_latch_hi(&_timer_A, val);
        break;
    case 0x0d:
        control_interrupts(val);
        printf("Changed interrupt mask to %02x\n", _interrupt_mask);
        break;
    case 0x0e:
        control_timer_A(&_timer_A, val);
        /* TODO: Handle 6 & 7 */
        break;
    default:
        //printf("Writing unhandled CIA 1 register: %02x: %02x\n", reg, val);
        break;
    }
}

