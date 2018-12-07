#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cia_timer.h"
#include "cpu.h"
#include "cia1.h"

uint8_t _interrupt_data;
uint8_t _interrupt_mask;
uint8_t _data_direction_port_A;
uint8_t _data_direction_port_B;


struct cia_timer _timer_A;
struct cia_timer _timer_B;


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
    cia_timer_reset(&_timer_A);
    cia_timer_reset(&_timer_B);
}

void cia1_cycle()
{
    bool underflowed_A = cia_timer_cycle_A(&_timer_A,
                                           &_timer_B);
    /* Generate interrupt */
    if (underflowed_A) {
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
        cia_timer_set_latch_lo(&_timer_A, val);
        break;
    case 0x05:
        cia_timer_set_latch_hi(&_timer_A, val);
        break;
    case 0x0d:
        control_interrupts(val);
        printf("Changed interrupt mask to %02x\n", _interrupt_mask);
        break;
    case 0x0e:
        cia_timer_control_A(&_timer_A, val);
        /* TODO: Handle 6 & 7 */
        break;
    default:
        //printf("Writing unhandled CIA 1 register: %02x: %02x\n", reg, val);
        break;
    }
}

