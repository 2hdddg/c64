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
/* For outgoing data */
uint8_t _data_port_A;
uint8_t _data_port_B;


struct cia_timer _timer_A;
struct cia_timer _timer_B;

typedef uint8_t (*cia_get_peripheral)();
typedef void (*cia_set_peripheral)(uint8_t val);


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
    cia1_reset();
}

static inline bool is_direction_in(uint8_t directions, uint8_t line)
{
    return (directions & line) == 0;
}

static inline bool is_direction_out(uint8_t directions, uint8_t line)
{
    return !is_direction_in(directions, line);
}

static inline bool is_peripheral_high(uint8_t peripheral, uint8_t line)
{
    return (peripheral & line) > 0;
}

static inline uint8_t get_bit_if_set(uint8_t val, uint8_t bit)
{
    return (val & bit) > 0 ? bit : 0;
}

uint8_t port_get(uint8_t directions, uint8_t data,
                 cia_get_peripheral get)
{
    /* Peripherals */
    uint8_t in  = get();
    uint8_t val = 0x00;
    uint8_t bit = 0x01;

    //printf("Raw per: %02x\n", in);

    for (int i = 0; i < 8; i++) {
        if (is_direction_in(directions, bit)) {
            val |= get_bit_if_set(in, bit);
        }
        else {
            val |= get_bit_if_set(data, bit);
        }
        bit = bit << 1;
    }
    return val;
}

void port_set(uint8_t directions, uint8_t data,
              cia_set_peripheral set)
{
    uint8_t out = 0x00;
    uint8_t bit = 0x01;

    for (int i = 0; i < 8; i++) {
        if (is_direction_out(directions, bit)) {
            out |= get_bit_if_set(data, bit);
        }
        bit = bit << 1;
    }
    set(out);
}

void cia1_reset()
{
    cia_timer_reset(&_timer_A);
    cia_timer_reset(&_timer_B);
    _interrupt_data = 0x00;
    _interrupt_mask = 0x00;
    _data_direction_port_A = 0;
    _data_direction_port_B = 0;
    _data_port_A = 0;
    _data_port_B = 0;
}

void cia1_cycle()
{
    cia_timer_cycle(&_timer_A, &_timer_B);
    /* Generate interrupt */
    if (_timer_A.underflowed) {
        //printf("Timer underflowed\n");
        if (_timer_A.port_B_on) {
            /* TODO: */
        }

        _interrupt_data |= CIA_INT_UNDERFLOW_TIMER_A;
    }

    /* Update interrupt status */
    if (_interrupt_data & _interrupt_mask) {
        _interrupt_data |= CIA_INT_OCCURED;
        /* TODO: IRQ or NMI (cia2) */
        cpu_interrupt_request();
    }
}

uint8_t cia1_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    /* Registers are mirrored at each 16 bytes */
    uint8_t reg = (absolute - CIA1_ADDRESS) % 0x10;
    uint8_t val;

    switch (reg) {
    case CIA_REG_DATA_PORT_A:
        val = port_get(_data_direction_port_A, _data_port_A,
                       keyboard_get_port_A);
        //printf("Read CIA PORT A: %02x\n", val);
        return val;
    case CIA_REG_DATA_PORT_B:
        val = port_get(_data_direction_port_B, _data_port_B,
                       keyboard_get_port_B);
        //printf("Read CIA PORT B: %02x\n", val);
        return val;
    case CIA_REG_DATA_DIRECTION_PORT_A:
        return _data_direction_port_A;
    case CIA_REG_DATA_DIRECTION_PORT_B:
        return _data_direction_port_B;
    case CIA_REG_TIMER_A_LO:
        return _timer_A.timer_lo;
    case CIA_REG_TIMER_A_HI:
        return _timer_A.timer_hi;
    case CIA_REG_INTERRUPT_CONTROL:
        //printf("Reading & clearing interrupt status\n");
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
    /* Registers are repeated at each 16 bytes */
    uint8_t reg = (absolute - CIA1_ADDRESS) % 0x10;

    switch (reg) {
    case CIA_REG_DATA_PORT_A:
        _data_port_A = val;
        port_set(_data_direction_port_A, _data_port_A,
                 keyboard_set_port_A);
        //printf("Writing to CIA PORT A: %02x\n", val);
        break;
    case CIA_REG_DATA_PORT_B:
        _data_port_B = val;
        port_set(_data_direction_port_B, _data_port_B,
                 keyboard_set_port_B);
        break;
    case CIA_REG_DATA_DIRECTION_PORT_A:
        _data_direction_port_A = val;
        port_set(_data_direction_port_A, _data_port_A,
                 keyboard_set_port_A);
        break;
    case CIA_REG_DATA_DIRECTION_PORT_B:
        _data_direction_port_B = val;
        port_set(_data_direction_port_B, _data_port_B,
                 keyboard_set_port_B);
        break;
    case CIA_REG_TIMER_A_LO:
        cia_timer_set_latch_lo(&_timer_A, val);
        break;
    case CIA_REG_TIMER_A_HI:
        cia_timer_set_latch_hi(&_timer_A, val);
        break;
    case CIA_REG_INTERRUPT_CONTROL:
        control_interrupts(val);
        //printf("Changed interrupt mask to %02x\n", _interrupt_mask);
        break;
    case CIA_REG_TIMER_A_CONTROL:
        cia_timer_control_A(&_timer_A, val);
        /* TODO: Handle 6 & 7 */
        break;
    default:
        //printf("Writing unhandled CIA 1 register: %02x: %02x\n", reg, val);
        break;
    }
}

