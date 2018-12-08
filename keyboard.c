#include <stdbool.h>
#include <stdio.h>

#include "keyboard.h"


uint8_t _lines[8];
int     _select_line;

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
    for (int i = 0; i < 8; i++) {
        _lines[i] = 0xff;
    }
    _select_line = -1;
}

void keyboard_down(uint16_t key)
{
    int line;

    line = line_from_key(key);
    key  = key_from_key(key);

    printf("Before %02x\n", _lines[line]);

    _lines[line] &= key;

    printf("Keyboard down line %d key 0x%02x -> %02x\n",
           line, key, _lines[line]);
}

void keyboard_up(uint16_t key)
{
    int line;

    line = line_from_key(key);
    key  = key_from_key(key);

    _lines[line] |= ~key;
    printf("Keyboard up line %02x key %02x -> %02x\n",
           line, key, _lines[line]);
}

uint8_t keyboard_get_port_A()
{
    printf("Get port A\n");
    return 0;
}

uint8_t keyboard_get_port_B()
{
    /* No key */
    uint8_t line = 0xff;

    if (_select_line != -1) {
        line = _lines[_select_line];
    }
    return line;
}

void keyboard_set_port_A(uint8_t line)
{
    /* Select which line to read */
    switch (line) {
        case 0xfe:
            _select_line = 0;
            break;
        case 0xfd:
            _select_line = 1;
            break;
        case 0xfb:
            _select_line = 2;
            break;
        case 0xf7:
            _select_line = 3;
            break;
        case 0xef:
            _select_line = 4;
            break;
        case 0xdf:
            _select_line = 5;
            break;
        case 0xbf:
            _select_line = 6;
            break;
        case 0x7f:
            _select_line = 7;
            break;
        case 0xff:
        case 0x00:
            _select_line = -1;
            break;
        default:
            printf("What line????: %02x\n", line);
            break;
    }
    if (_select_line != -1) {
        printf("Selecting line %02x\n", _select_line);
    }
}

void keyboard_set_port_B(uint8_t lines)
{
    printf("Setting port B to %02x\n", lines);
}

