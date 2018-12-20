#include <stdio.h>

#include "cia.h"
#include "cia2.h"
#include "vic.h"
#include "trace.h"


static struct cia_state _state = { 0 };

static uint8_t _get_port_A(uint8_t interesting_bits)
{
    uint8_t val = 0x00;

    /* Query VIC for memory location */
    val = (uint8_t)vic_get_bank();

    /* TODO: Query RS-232 */
    /* TODO: Query serial bus */

    return val;
}

static uint8_t _get_port_B(uint8_t interesting_bits)
{
    TRACE_NOT_IMPL(_state.trace_error, "get port B");
    return 0;
}

static void _set_port_A(uint8_t d, uint8_t valid_lines)
{
    /* Select VIC bank */
    if ((valid_lines & 0b11) != 0) {
        vic_set_bank(d & 0b11);
    }

    /* TODO: */
}

static void _set_port_B(uint8_t d, uint8_t valid_lines)
{
    TRACE_NOT_IMPL(_state.trace_error, "set port B");
}

static void _interrupt()
{
    TRACE_NOT_IMPL(_state.trace_error, "interrupt");
}

void cia2_init()
{
    /* Callbacks, represents CIA1 pin connections */
    _state.on_get_peripheral_A = _get_port_A;
    _state.on_get_peripheral_B = _get_port_B;
    _state.on_set_peripheral_A = _set_port_A;
    _state.on_set_peripheral_B = _set_port_B;
    _state.on_interrupt        = _interrupt;

    /* Debugging */
    _state.trace_set_port = trace_add_point("CIA2", "set port");
    _state.trace_get_port = trace_add_point("CIA2", "get port");
    _state.trace_timer    = trace_add_point("CIA2", "timer");
    _state.trace_error    = trace_add_point("CIA2", "ERROR");
}

void cia2_reset()
{
    cia_reset(&_state);
}

void cia2_cycle()
{
    cia_cycle(&_state);
}

uint8_t cia2_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    /* Registers are mirrored at each 16 bytes */
    uint8_t reg = (absolute - CIA2_ADDRESS) % 0x10;

    return cia_get_register(&_state, reg);
}

void cia2_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
    /* Registers are repeated at each 16 bytes */
    uint8_t reg = (absolute - CIA2_ADDRESS) % 0x10;

    cia_set_register(&_state, reg, val);
}

