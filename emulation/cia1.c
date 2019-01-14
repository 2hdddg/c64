#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cia_timer.h"
#include "cpu.h"
#include "cia.h"
#include "cia1.h"
#include "trace.h"
#include "keyboard.h"

static struct cia_state _state = { 0 };


void cia1_init()
{
    /* Callbacks, represents CIA1 pin connections */
    _state.on_get_peripheral_A = keyboard_get_port_A;
    _state.on_get_peripheral_B = keyboard_get_port_B;
    _state.on_set_peripheral_A = keyboard_set_port_A;
    _state.on_set_peripheral_B = keyboard_set_port_A;
    _state.on_interrupt        = cpu_interrupt_request;

    /* Debugging */
    _state.trace_set_port = trace_add_point("CIA1", "set port");
    _state.trace_get_port = trace_add_point("CIA1", "get port");
    _state.trace_timer    = trace_add_point("CIA1", "timer");
    _state.trace_error    = trace_add_point("CIA1", "ERROR");

    cia1_reset();
}

void cia1_reset()
{
    cia_reset(&_state);
}

void cia1_cycle()
{
    cia_cycle(&_state);
}

uint8_t cia1_reg_get(uint16_t absolute, uint8_t *ram)
{
    /* Registers are mirrored at each 16 bytes */
    uint8_t reg = (absolute - CIA1_ADDRESS) % 0x10;

    return cia_get_register(&_state, reg);
}

void cia1_reg_set(uint8_t val, uint16_t absolute, uint8_t *ram)
{
    /* Registers are repeated at each 16 bytes */
    uint8_t reg = (absolute - CIA1_ADDRESS) % 0x10;

    cia_set_register(&_state, reg, val);
}

