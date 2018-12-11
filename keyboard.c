#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "trace.h"
#include "keyboard.h"


uint8_t _lines[8];
uint8_t _data_port_A;

/* Debugging */
static struct trace_point *_trace_key      = NULL;
static struct trace_point *_trace_set_port = NULL;
static struct trace_point *_trace_get_port = NULL;


static int line_from_key(uint16_t key)
{
    /* TODO: Endianness */
    return key >> 8;
}

static uint8_t key_from_key(uint16_t key)
{
    /* TODO: Endianness */
    return (uint8_t)key;
}

void keyboard_init()
{
    _trace_key = trace_add_point("KBD", "key");
    _trace_set_port = trace_add_point("KBD", "set port");
    _trace_get_port = trace_add_point("KBD", "get port");
}

void keyboard_reset()
{
    for (int i = 0; i < 8; i++) {
        /* Set to no key */
        _lines[i] = 0xff;
    }
    _data_port_A = 0;
}

void keyboard_down(uint16_t key)
{
    int line;

    line = line_from_key(key);
    key  = key_from_key(key);
    _lines[line] &= key;

    TRACE(_trace_key, "%02x down on line %02x", key, line);
}

void keyboard_up(uint16_t key)
{
    int line;

    line = line_from_key(key);
    key  = key_from_key(key);
    _lines[line] |= ~key;

    TRACE(_trace_key, "%02x up on line %02x", key, line);
}

uint8_t keyboard_get_port_A()
{
    TRACE(_trace_get_port, "A: %02x", _data_port_A);
    return _data_port_A;
}

uint8_t keyboard_get_port_B()
{
    /* Default to no keys pressed */
    uint8_t keys = 0x00;
    /* Set to 0 for keyboard line to scan */
    uint8_t line = ~_data_port_A;

    /* If more than one keyboard line selected to
     * scan. Kernal sets this to 0 (all lines),
     * what is the expected behaviour in that case? */
    for (int i = 0; i < 8; i++) {
        if ((line & 0x01) ) {
            keys |= ~(_lines[i]);
        }
        line = line >> 1;
    }
    /* Should be zero for pressed key */
    keys = ~keys;

    TRACE(_trace_get_port, "B: %02x", keys);
    return keys;
}

void keyboard_set_port_A(uint8_t data)
{
    _data_port_A = data;
    TRACE(_trace_set_port, "A: %02x", data);
}

void keyboard_set_port_B(uint8_t data)
{
    /* Does nothing here! What should happen? */
    TRACE(_trace_set_port, "B: %02x", data);
}
