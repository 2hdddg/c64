#include "cia.h"
#include "trace.h"

static void control_interrupts(struct cia_state *state,
                               uint8_t control)
{
    if (control & 0x80) {
        /* Ones in control sets corresponding bits in mask,
         * zeroes in control shouldn't affect mask. */
        state->interrupt_mask |= (control & 0x7f);
    }
    else {
        /* Ones in control clears corresponding bits in mask,
         * zeroes in control shouldn't affect mask. */
        state->interrupt_mask &= ~control;
    }
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

static uint8_t port_get(uint8_t directions, uint8_t data,
                 cia_get_peripheral get)
{
    /* Peripherals */
    uint8_t in  = get();
    uint8_t val = 0x00;
    uint8_t bit = 0x01;

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

static void port_set(uint8_t directions, uint8_t data,
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

void cia_reset(struct cia_state *state)
{
    cia_timer_reset(&state->timer_A);
    cia_timer_reset(&state->timer_B);
    state->interrupt_data = 0x00;
    state->interrupt_mask = 0x00;
    state->data_direction_port_A = 0;
    state->data_direction_port_B = 0;
    state->data_port_A = 0;
    state->data_port_B = 0;
}

void cia_cycle(struct cia_state *state)
{
    cia_timer_cycle(&state->timer_A, &state->timer_B);
    /* Generate interrupt */
    if (state->timer_A.underflowed) {
        TRACE0(state->trace_timer, "A: underflowed");
        if (state->timer_A.port_B_on) {
            TRACE_NOT_IMPL(state->trace_error, "underflow to port B");
            /* TODO: */
        }

        state->interrupt_data |= CIA_INT_UNDERFLOW_TIMER_A;
    }

    /* Update interrupt status */
    if (state->interrupt_data & state->interrupt_mask) {
        state->interrupt_data |= CIA_INT_OCCURED;
        state->on_interrupt();
    }
}

void cia_set_register(struct cia_state *state,
                      uint8_t reg, uint8_t val)
{
    switch (reg) {
    case CIA_REG_DATA_PORT_A:
        state->data_port_A = val;
        port_set(state->data_direction_port_A, state->data_port_A,
                 state->on_set_peripheral_A);
        TRACE(state->trace_set_port, "A: %02x", val);
        break;
    case CIA_REG_DATA_PORT_B:
        state->data_port_B = val;
        port_set(state->data_direction_port_B, state->data_port_B,
                 state->on_set_peripheral_B);
        TRACE(state->trace_set_port, "B: %02x", val);
        break;
    case CIA_REG_DATA_DIRECTION_PORT_A:
        state->data_direction_port_A = val;
        port_set(state->data_direction_port_A, state->data_port_A,
                 state->on_set_peripheral_A);
        TRACE(state->trace_set_port, "A direction: %02x", val);
        break;
    case CIA_REG_DATA_DIRECTION_PORT_B:
        state->data_direction_port_B = val;
        port_set(state->data_direction_port_B, state->data_port_B,
                 state->on_set_peripheral_B);
        TRACE(state->trace_set_port, "B direction: %02x", val);
        break;
    case CIA_REG_TIMER_A_LO:
        cia_timer_set_latch_lo(&state->timer_A, val);
        break;
    case CIA_REG_TIMER_A_HI:
        cia_timer_set_latch_hi(&state->timer_A, val);
        break;
    case CIA_REG_INTERRUPT_CONTROL:
        control_interrupts(state, val);
        //printf("Changed interrupt mask to %02x\n", _interrupt_mask);
        break;
    case CIA_REG_TIMER_A_CONTROL:
        cia_timer_control_A(&state->timer_A, val);
        /* TODO: Handle 6 & 7 */
        break;
    default:
        TRACE(state->trace_error, "set reg %02x not handled", reg);
        break;
    }
}

uint8_t cia_get_register(struct cia_state *state,
                         uint8_t reg)
{
    uint8_t val;

    switch (reg) {
    case CIA_REG_DATA_PORT_A:
        val = port_get(state->data_direction_port_A, state->data_port_A,
                       state->on_get_peripheral_A);
        TRACE(state->trace_get_port, "A: %02x", val);
        return val;
    case CIA_REG_DATA_PORT_B:
        val = port_get(state->data_direction_port_B, state->data_port_B,
                       state->on_get_peripheral_B);
        TRACE(state->trace_get_port, "B: %02x", val);
        return val;
    case CIA_REG_DATA_DIRECTION_PORT_A:
        return state->data_direction_port_A;
    case CIA_REG_DATA_DIRECTION_PORT_B:
        return state->data_direction_port_B;
    case CIA_REG_TIMER_A_LO:
        return state->timer_A.timer_lo;
    case CIA_REG_TIMER_A_HI:
        return state->timer_A.timer_hi;
    case CIA_REG_INTERRUPT_CONTROL:
        //printf("Reading & clearing interrupt status\n");
        val = state->interrupt_data;
        /* Cleared on read */
        state->interrupt_data = 0x00;
        /* No more interrupt */
        return val;
    default:
        TRACE(state->trace_error, "get reg %02x not handled", reg);
        break;
    }

    return 0;
}


