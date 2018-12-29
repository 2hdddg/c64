#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "trace.h"
#include "vic.h"

/* Emulates Video Interface Controller II (VIC-II).
 * PAL version (6569).
 */

/* Defined in vic_palette.c */
uint32_t palette[16];

static struct trace_point *_trace_set_reg = NULL;
static struct trace_point *_trace_get_reg = NULL;
static struct trace_point *_trace_error   = NULL;
static struct trace_point *_trace_bank    = NULL;

static enum vic_bank _bank       = 0x00;
static uint16_t _bank_offset     = 0x0000;
static uint16_t _char_rom_offset = 0x0000;

static uint8_t *_char_rom;
static uint8_t *_ram;
static uint8_t *_color_ram;

bool _bitmap_graphics;
bool _extended_color_text;
/* When false, screen will be blank, no dirty lines,
 * more power to the CPU. */
bool _display_enable;
bool _multicolor;
bool _40_columns;
bool _25_rows;
bool _reset;

uint8_t _scroll_y;
uint8_t _scroll_x;

uint16_t _raster_compare = 0;
uint8_t  _interrupt_mask = 0;
uint8_t  _interrupt_flag = 0;

uint32_t _border_color;
uint32_t _background_color0;
uint32_t _background_color1;
uint32_t _background_color2;

/* For easier get impl */
uint8_t _raw_regs[0x40];

/* Address within VIC bank that contains char pixels */
uint16_t _char_pixels_addr  = 0x0000;
/* Address within VIC bank that contains chars and sprite shapes */
uint16_t _video_matrix_addr = 0x0000;

/* Output */
static uint32_t *_screen;
static uint32_t _pitch;

/* First/last line of visible area */
const uint16_t _y_visible_start = 0;
const uint16_t _y_visible_end   = 312;
/* First/last column of visible area */
const uint16_t _x_visible_start = 0;
const uint16_t _x_visible_end   = 400;
/* Number of blanking before restart */
const uint16_t _y_blanking      = 2;
const uint16_t _x_blanking      = 2;
/* First/last line of drawable area.
 * Changed when toggling between 24/25 rows. */
uint16_t _y_drawable_start;
uint16_t _y_drawable_end;
/* First/last column of drawable area.
 * Changed when toggling between 38/40 columns. */
uint16_t _x_drawable_start;
uint16_t _x_drawable_end;

uint16_t _curr_y        = 312;
uint16_t _curr_x        = 401;
uint16_t _curr_blanking = 0;
uint16_t _curr_fetching = 0;
uint32_t *_curr_pixel;

/* Offset corresponds to _x_visible_start of 24 pixels */
#define LINE_OFFSET 3
/* Filled during bad line */
uint8_t _curr_video_line[40+LINE_OFFSET];
uint8_t _curr_color_line[40+LINE_OFFSET];

void vic_stat()
{
    printf("VIC \n");
    printf("Display enable : %s\n", _display_enable ? "yes" : "no");
    printf("Bitmap graphics: %s\n", _bitmap_graphics ? "yes" : "no");
    printf("Extended color : %s\n", _extended_color_text ? "yes" : "no");
    printf("Multicolor     : %s\n", _multicolor ? "yes" : "no");
    printf("Video matrix   : %04x\n", _video_matrix_addr);
    printf("Char pixels    : %04x\n", _char_pixels_addr);
    printf("Scroll y       : %02x\n", _scroll_y);
    printf("Scroll x       : %02x\n", _scroll_x);
    printf("Raster compare : %04x\n", _raster_compare);
    printf("40 columns     : %s\n", _40_columns ? "yes" : "no");
    printf("25 rows        : %s\n", _25_rows ? "yes" : "no");
    printf("Reset          : %s\n", _reset ? "yes" : "no");
    printf("Interrupt mask : %02x\n", _interrupt_mask);
    printf("Interrupt flag : %02x\n", _interrupt_flag);
}

static void _setup_drawable_area()
{
    _x_drawable_start = 24;
    _x_drawable_end   = 343;
    if (!_40_columns) {
        _x_drawable_start += 8;
        _x_drawable_end   -= 8;
    }

    _y_drawable_start = 51;
    _y_drawable_end   = 251;
    if (!_25_rows) {
        _y_drawable_start += 8 - _scroll_y;
        _y_drawable_end -= 8 -_scroll_y;
    }
}

static void vic_reset()
{
    _bitmap_graphics     = false;
    _extended_color_text = false;
    _display_enable      = false;
    _multicolor          = false;
    _40_columns          = false;
    _25_rows             = false;
    _reset               = false;

    _setup_drawable_area();
}

void vic_init(uint8_t *char_rom,
              uint8_t *ram,
              uint8_t *color_ram)
{
    _char_rom  = char_rom;
    _ram       = ram;
    _color_ram = color_ram;

    memset(_raw_regs, 0, 0x40);
    vic_reset();

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
    uint16_t offset = (absolute - 0xd00) % 0x40;

    switch (offset) {
    case 0x12:
        return _curr_y & 0xff;
    default:
        TRACE(_trace_error, "get reg %04x not handled", absolute);
        return _raw_regs[offset];
    }
}

void vic_reg_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
    uint16_t offset = (absolute - 0xd00) % 0x40;

    /* Save for fallback impl of get */
    _raw_regs[offset] = val;

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
        _scroll_y            = (val & 0b00000111);
        _25_rows             = (val & 0b00001000) > 0;
        _display_enable      = (val & 0b00010000) > 0;
        _bitmap_graphics     = (val & 0b00100000) > 0;
        _extended_color_text = (val & 0b01000000) > 0;
        if (val & 0b10000000) {
            /* Bit 8 of raster compare */
            _raster_compare |= 0x100;
        }
        else {
            _raster_compare &= 0b011111111;
        }
        _setup_drawable_area();
        break;
    case 0x12:
    /* RASTER */
        _raster_compare = (_raster_compare & 0xff00) | val;
        break;
    /* SCROLX, horizontal fine scrolling and control */
    case 0x16:
        _40_columns = (val & 0b00001000) > 0;
        _multicolor = (val & 0b00010000) > 0;
        _scroll_x   = (val & 0b00000111);
        _reset      = (val & 0b00100000) > 0;
        _setup_drawable_area();
        break;
    /* VMCSB */
    case 0x18:
        _char_pixels_addr  = (val & 0b00001110) * 1024;
        _video_matrix_addr = ((val & 0b11110000) >> 4) * 1024;
        break;
    /* VICIRQ */
    case 0x19:
        /* Read only ? */
        break;
    /* IRQMASK */
    case 0x1a:
        _interrupt_mask = val;
        break;
    case 0x20:
        _border_color = palette[val & 0x0f];
        break;
    case 0x21:
        _background_color0 = palette[val & 0x0f];
        break;
    case 0x22:
        _background_color1 = palette[val & 0x0f];
        break;
    case 0x23:
        _background_color2 = palette[val & 0x0f];
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
        _bank_offset     = 0x0000;
        _char_rom_offset = 0x1000;
        break;
    case vic_bank_1:
        _bank_offset     = 0x4000;
        _char_rom_offset = 0x0000;
        break;
    case vic_bank_2:
        _bank_offset     = 0x8000;
        _char_rom_offset = 0x9000;
        break;
    case vic_bank_3:
        _bank_offset     = 0xc000;
        _char_rom_offset = 0x0000;
        break;
    }
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

#define X_PER_CYCLE 8


static bool is_bad_line()
{
    return _curr_x == _x_visible_start &&
           _curr_y >= _y_drawable_start &&
           _curr_y < _y_drawable_end &&
           ((_curr_y & 0b111) == _scroll_y);
}

/* Fills internal 40*12 bits video matrix/color line buffer. */
static void c_access()
{
    uint8_t *from;
    uint16_t offset;

    /* Video matrix / chars */
    offset = ((_curr_y - _y_drawable_start) >> 3) * 40;
    from = _ram + _bank_offset + _video_matrix_addr + offset;
    memcpy(_curr_video_line+LINE_OFFSET, from, 40);

    /* Color data */
    from = _color_ram + offset;
    memcpy(_curr_color_line+LINE_OFFSET, from, 40);
}

/* Reads pixel data, char rom or bitmap */
static inline uint8_t g_access(uint16_t offset)
{
    uint16_t addr = _char_pixels_addr + offset;

    if (_char_rom_offset > 0 &&
        addr >= _char_rom_offset &&
        addr < _char_rom_offset + 0x1000) {
        return _char_rom[offset];
    }
    return _ram[offset];
}

void vic_step(bool *refresh)
{
    int c = X_PER_CYCLE;
    *refresh = false;

    if (_curr_blanking) {
        _curr_blanking--;
        return;
    }

    /* New raster line? */
    if (_curr_x > _x_visible_end) {
        _curr_y++;
        _curr_x = _x_visible_start;
        _curr_blanking = _x_blanking - 1;
        _curr_pixel = (uint32_t*)(((uint8_t*)_screen) + _pitch * _curr_y);
        return;
    }

    /* Last raster line? */
    if (_curr_y > _y_visible_end) {
        _curr_y = _y_visible_start;
        _curr_blanking = _y_blanking - 1;
        *refresh  = true;
        return;
    }

    /* CPU is stalled while fetching */
    if (_curr_fetching) {
        _curr_fetching--;
    }
    /* Check if another 40 chars + colors needs to be fetched */
    else if (is_bad_line()) {
        _curr_fetching = 40;
        c_access();
    }

    /* Top/bottom border */
    if (_curr_y < _y_drawable_start ||
        _curr_y >= _y_drawable_end) {
        while (c--) {
            *_curr_pixel = _border_color;
            _curr_pixel++;
        }
        _curr_x += X_PER_CYCLE;
        return;
    }

    /* Left border */
    while (_curr_x < _x_drawable_start && c) {
        *_curr_pixel = _border_color;
        _curr_pixel++;
        c--;
        _curr_x++;
    }

    if (c && _curr_x >= _x_drawable_start) {
        /* Inside window */
        int      char_index  = _curr_x / 8;
        uint8_t  char_code   = _curr_video_line[char_index];
        int      char_line   = (_curr_y - _scroll_y) % 8;
        uint16_t char_offset = (char_code * (8)) + char_line;
        uint8_t  char_pixels = g_access(char_offset);
        uint8_t  color_index = _curr_color_line[char_index] & 0x7f;
        uint32_t color_fg    = palette[color_index];

        while (c &&
               _curr_x <= _x_drawable_end) {
            if (char_pixels & 0b10000000) {
                *_curr_pixel = color_fg;
            }
            else {
                *_curr_pixel = _background_color0;
            }
            _curr_pixel++;
            char_pixels = char_pixels << 1;
            _curr_x++;
            c--;
        }
    }

    /* Right border */
    if (c && _curr_x > _x_drawable_end) {
        while (c) {
            *_curr_pixel = _border_color;
            _curr_pixel++;
            c--;
            _curr_x++;
        }
    }
}
