#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "trace.h"
#include "vic.h"

static uint32_t _colors[] = {
    /* Black */
    0x00000000,
    /* White */
    0x00FFFFFF,
    /* Red */
    0x0068372B,
    /* Cyan */
    0x0070A4B2,
    /* Violet/purple */
    0x006F3D86,
    /* Green */
    0x00588D43,
    /* Blue */
    0x00352879,
    /* Yellow */
    0x00B8C76F,
    /* Orange */
    0x006F4F25,
    /* Brown */
    0x00433900,
    /* Light red */
    0x009A6759,
    /* Dark grey, grey 1 */
    0x00444444,
    /* Grey 2 */
    0x006C6C6C,
    /* Light green */
    0x009AD284,
    /* Light blue */
    0x006C5EB5,
    /* Light grey, grey 3 */
    0x00959595,
};

static struct trace_point *_trace_set_reg = NULL;
static struct trace_point *_trace_get_reg = NULL;
static struct trace_point *_trace_error   = NULL;
static struct trace_point *_trace_bank    = NULL;

static enum vic_bank _bank = 0x00;

static uint32_t *_screen;
static uint32_t _pitch;

void vic_init()
{
    _trace_set_reg = trace_add_point("VIC", "set reg");
    _trace_get_reg = trace_add_point("VIC", "get reg");
    _trace_error   = trace_add_point("VIC", "ERROR");
    _trace_bank    = trace_add_point("VIC", "bank");
}

void vic_screen(uint32_t *screen, uint32_t pitch)
{
    _screen = screen;
    _pitch  = pitch;
}

uint8_t vic_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    TRACE(_trace_error, "get reg %04x not handled", absolute);
    return 0;
}

void vic_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
    TRACE(_trace_error, "set reg %04x not handled", absolute);
}

void vic_set_bank(enum vic_bank bank)
{
    _bank = bank;
    TRACE(_trace_bank, "select %01x", bank);
}

enum vic_bank vic_get_bank()
{
    return _bank;
}

uint16_t _curr_y = 200;
uint16_t _curr_x;
uint32_t *_line;

void vic_step(bool *refresh)
{
    *refresh = false;
    if (_curr_x > 319) {
        /* Line done */
        _curr_y++;
        _line = (uint32_t*)(((uint8_t*)_screen) + _pitch * _curr_y);
        _curr_x = 0;
    }

    if (_curr_y > 199) {
        /* Screen done */
        *refresh = true;
        _curr_y = 0;
        _curr_x = 0;
        _line = _screen;
        return;
    }

    *_line++ = _colors[14];
    *_line++ = _colors[14];
    *_line++ = _colors[14];
    *_line++ = _colors[14];
    *_line++ = _colors[14];
    *_line++ = _colors[14];
    *_line++ = _colors[14];
    *_line++ = _colors[14];
    _curr_x += 8;
}
