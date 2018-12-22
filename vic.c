#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "trace.h"
#include "vic.h"

/* Emulates Video Interface Controller II (VIC-II).
 * PAL version (6569).
 */

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


static uint8_t *_char_rom;
static uint8_t *_ram;
static uint8_t *_color_ram;

bool _bitmap_graphics     = false;
bool _extended_color_text = false;
/* Screen will be blank, no dirty lines, more power to the CPU */
bool _screen_blanking     = false;
bool _multicolor          = false;

/* Address within VIC bank that contains char pixels */
uint16_t _char_pixels_addr  = 0x0000;
/* Address within VIC bank that contains chars and sprite shapes */
uint16_t _video_matrix_addr = 0x0000;

/* Output */
static uint32_t *_screen;
static uint32_t _pitch;


void vic_init(uint8_t *char_rom)
{
    _char_rom = char_rom;

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

uint8_t vic_reg_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    TRACE(_trace_error, "get reg %04x not handled", absolute);
    return 0;
}

void vic_reg_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
    uint16_t offset = (absolute - 0xd00) % 0x40;

    switch (offset) {
    /* 0x00-0x10 Sprites */
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
    case 0x0a:
    case 0x0b:
    case 0x0c:
    case 0x0d:
    case 0x0e:
    case 0x0f:
    case 0x10:
    case 0x15:
    case 0x17:
    case 0x1b:
    case 0x1c:
    case 0x1d:
    case 0x1e:
    case 0x1f:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b:
    case 0x2c:
    case 0x2d:
    case 0x2e:
        TRACE_NOT_IMPL(_trace_error, "sprites");
        break;

    /* SCROLY, vertical fine scrolling and control */
    case 0x11:
        _screen_blanking     = (val & 0b00010000) > 0;
        _bitmap_graphics     = (val & 0b00100000) > 0;
        _extended_color_text = (val & 0b01000000) > 0;
        /* TODO: More bits! */
        break;
    case 0x16:
    /* SCROLX, horizontal fine scrolling and control */
        _multicolor = (val & 0b00010000) > 0;
        /* TODO: More bits! */
        break;
    case 0x18:
    /* VMCSB */
        _char_pixels_addr  = (val & 0b00001110) * 1024;
        _video_matrix_addr = ((val & 0b11110000) >> 4) * 1024;
        break;
    case 0x12:
    case 0x19:
    case 0x1a:
        TRACE_NOT_IMPL(_trace_error, "raster interrupt");
        break;
    case 0x13:
    case 0x14:
        TRACE_NOT_IMPL(_trace_error, "light pen");
        break;

    default:
        TRACE(_trace_error, "set reg %04x not handled", absolute);
        break;
    }

}

void vic_set_bank(enum vic_bank bank)
{
_bank = bank;
    switch (_bank) {
    case vic_bank_0:
        _ram = mem_get_ram_for_vic(0x0000);
        break;
    case vic_bank_1:
        _ram = mem_get_ram_for_vic(0x4000);
        break;
    case vic_bank_2:
        _ram = mem_get_ram_for_vic(0x8000);
        break;
    case vic_bank_3:
        _ram = mem_get_ram_for_vic(0xc000);
        break;
    }
    _color_ram = mem_get_color_ram_for_vic();
    TRACE(_trace_bank, "select %01x", bank);
}

enum vic_bank vic_get_bank()
{
    return _bank;
}

/*
+------------------------------------------------+  <- Line 0 (6569)
|       .                                .       |
|       .   Vertical blanking interval   .       |
|       .                                .       |
+---+---+--------------------------------+---+---+  \
|   |   |                                |   |   |  |
| H |   |            Top border          |   | H |  |
| o |   |                                |   | o |  |
| r |   +--------------------------------+   | r |  |
| i |   |                                |   | i |  |
| z |   |                                |   | z |  |
| o |   |                                |   | o |  |
| n |   |                                |   | n |  |
| t |   |                                |   | t |  |
| a |   |                                | r | a |  |
| l | l |                                | i | l |  |
|   | e |                                | g |   |  |
| b | f |                                | h | b |  |
| l | t |                                | t | l |  |
| a |   |         Display window         |   | a |  |- Visible lines
| n | b |                                | b | n |  |
| k | o |                                | o | k |  |
| i | r |                                | r | i |  |
| n | d |                                | d | n |  |
| g | e |                                | e | g |  |
|   | r |                                | r |   |  |
| i |   |                                |   | i |  |
| n |   |                                |   | n |  |
| t |   |                                |   | t |  |
| e |   |                                |   | e |  |
| r |   |                                |   | r |  |
| v |   +--------------------------------+   | v |  |
| a |   |                                |   | a |  |
| l |   |          Lower border          |   | l |  |
|   |   |                                |   |   |  |
+---+---+--------------------------------+---+---+  /
|       .                                .       |
|       .   Vertical blanking interval   .       |
|       .                                .       |
+------------------------------------------------+

# of lines:          312
Visible lines:       284
Cycles/line:          63
Visible pixels/line: 403
First vblank line:   300
Last vblank line:     15
First 


*/

/* Blanking */
#define Y_START         0
#define Y_WINDOW_START 50
#define Y_WINDOW_END  250
#define Y_END         312
/* Blanking */

/* Blanking */
#define X_START          0
#define X_WINDOW_START  40
#define X_WINDOW_END   360
#define X_END          400
/* Blanking */

#define X_PER_CYCLE 8
#define Y_BLANKING 2
#define X_BLANKING 2

uint16_t _curr_y   = Y_END;
uint16_t _curr_x   = X_END;
uint16_t _blanking = 0;


void vic_step(bool *refresh)
{
    uint8_t  *line = (uint8_t*)_screen;
    uint32_t *pixels;
    int      c;

    *refresh = false;

    if (_blanking) {
        _blanking--;
        return;
    }

    if (_curr_x > X_END) {
        _curr_y++;
        _curr_x   = X_START;
        _blanking = X_BLANKING;
    }
    if (_blanking) {
        _blanking--;
        return;
    }

    if (_curr_y > Y_END) {
        _curr_y   = Y_START;
        _blanking = Y_BLANKING;
        *refresh  = true;
    }
    if (_blanking) {
        _blanking--;
        return;
    }

    line  += (_pitch * _curr_y);
    pixels = (uint32_t*)line;
    pixels += _curr_x;

    /* Border */
    if (_curr_x < X_WINDOW_START ||
        _curr_x > X_WINDOW_END ||
        _curr_y < Y_WINDOW_START ||
        _curr_y > Y_WINDOW_END) {
        for (c = 0; c < X_PER_CYCLE; c++) {
            pixels[c] = _colors[14];
        }
        _curr_x += X_PER_CYCLE;
        return;
    }

    /* Inside window */
    for (c = 0; c < X_PER_CYCLE; c++) {
        pixels[c] = _colors[1];
    }
    _curr_x += X_PER_CYCLE;
}
