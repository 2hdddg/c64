#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "keyboard.h"


uint8_t _lines[8];
uint8_t _data_port_A;

/* Debugging */
int     _trace_keys;
int     _trace_port_set;
int     _trace_port_get;
char    _trace_buffer[100];

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

void keyboard_reset()
{
    for (int i = 0; i < 9; i++) {
        /* Set to no key */
        _lines[i] = 0xff;
    }
    _data_port_A = 0;
    /* Debug stuff */
    _trace_keys = -1;
    _trace_port_set = -1;
    _trace_port_get = -1;
}

void keyboard_down(uint16_t key)
{
    int line;

    line = line_from_key(key);
    key  = key_from_key(key);
    _lines[line] &= key;

    if (_trace_keys != -1) {
        sprintf(_trace_buffer,
                "KBD: Key down, line %02x, key %02x\n", line, key);
        write(_trace_keys, _trace_buffer, strlen(_trace_buffer));
    }
}

void keyboard_up(uint16_t key)
{
    int line;

    line = line_from_key(key);
    key  = key_from_key(key);
    _lines[line] |= ~key;

    if (_trace_keys != -1) {
        sprintf(_trace_buffer,
                "KBD: Key up, line %02x, key %02x\n", line, key);
        write(_trace_keys, _trace_buffer, strlen(_trace_buffer));
    }
}

uint8_t keyboard_get_port_A()
{
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

    if (_trace_port_get != -1) {
        sprintf(_trace_buffer,
                "KBD: Get port B: %02x\n", keys);
        write(_trace_port_get, _trace_buffer, strlen(_trace_buffer));
    }

    return keys;
}

void keyboard_set_port_A(uint8_t data)
{
    _data_port_A = data;
    if (_trace_port_set != -1) {
        sprintf(_trace_buffer,
                "KBD: Setting port A to: %02x\n", data);
        write(_trace_port_set, _trace_buffer, strlen(_trace_buffer));
    }
}

void keyboard_set_port_B(uint8_t data)
{
    /* Does nothing here! What should happen? */
    if (_trace_port_set != -1) {
        sprintf(_trace_buffer,
                "KBD: Setting port B to: %02x\n", data);
        write(_trace_port_set, _trace_buffer, strlen(_trace_buffer));
    }
}

void keyboard_trace_keys(int fd)
{
    _trace_keys = fd;
}

void keyboard_trace_port_set(int fd)
{
    _trace_port_set = fd;
}

void keyboard_trace_port_get(int fd)
{
    _trace_port_get = fd;
}

