#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "trace.h"
#include "vic.h"

/* Emulates Video Interface Controller II (VIC-II).
 * PAL version (6569).
 */

struct cycle {
    uint8_t  i; /* Char index */
    uint16_t x;
    bool     v; /* Visible */
};

/* Defined in vic_palette.c */
uint32_t palette[16];

#define X_DRAW_START 46

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

/* First/last line of drawable area.
 * Changed when toggling between 24/25 rows. */
uint16_t _top;
uint16_t _bottom;
/* First/last column of drawable area.
 * Changed when toggling between 38/40 columns. */
uint16_t _left;
uint16_t _right;

uint16_t _curr_y        = 0;
uint16_t _curr_x        = 401;
//uint16_t _curr_blanking = 0;
uint16_t _curr_fetching = 0;
uint32_t *_curr_pixel;

/* Filled during bad line */
uint8_t _curr_video_line[40];
uint8_t _curr_color_line[40];

static bool _main_flip_flop;
static bool _vert_flip_flop;

int _curr_cycle = 0;

uint8_t _pixels;
uint32_t _color_fg;

struct cycle _line_cycles[63] = {
    /* 0 - 9 */
    { .i = -1, .x = 0x194, }, { .i = -1, .x = 0x19c, },
    { .i = -1, .x = 0x1a4, }, { .i = -1, .x = 0x1ac, },
    { .i = -1, .x = 0x1b4, }, { .i = -1, .x = 0x1bc, },
    { .i = -1, .x = 0x1c4, }, { .i = -1, .x = 0x1cc, },
    { .i = -1, .x = 0x1d4, }, { .i = -1, .x = 0x1dc, },
    /* 10 - 19 */
    { .i = -1, .x = 0x1e4, .v = true, },
    { .i = -1, .x = 0x1ec, .v = true, },
    { .i = -1, .x = 0x1f4, .v = true, },
    { .i = -1, .x = 0x1fc, .v = true, },
    { .i = -1, .x = 0x004, .v = true, },
    { .i = -1, .x = 0x00c, .v = true, },
    { .i =  0, .x = 0x014, .v = true, },
    { .i =  1, .x = 0x01c, .v = true, },
    { .i =  2, .x = 0x024, .v = true, },
    { .i =  3, .x = 0x02c, .v = true, },
    /* 20 - 29 */
    { .i =  4, .x = 0x034, .v = true, },
    { .i =  5, .x = 0x03c, .v = true, },
    { .i =  6, .x = 0x044, .v = true, },
    { .i =  7, .x = 0x04c, .v = true, },
    { .i =  8, .x = 0x054, .v = true, },
    { .i =  9, .x = 0x05c, .v = true, },
    { .i = 10, .x = 0x064, .v = true, },
    { .i = 11, .x = 0x06c, .v = true, },
    { .i = 12, .x = 0x074, .v = true, },
    { .i = 13, .x = 0x07c, .v = true, },
    /* 30 - 39 */
    { .i = 14, .x = 0x084, .v = true, },
    { .i = 15, .x = 0x08c, .v = true, },
    { .i = 16, .x = 0x094, .v = true, },
    { .i = 17, .x = 0x09c, .v = true, },
    { .i = 18, .x = 0x0a4, .v = true, },
    { .i = 19, .x = 0x0ac, .v = true, },
    { .i = 20, .x = 0x0b4, .v = true, },
    { .i = 21, .x = 0x0bc, .v = true, },
    { .i = 22, .x = 0x0c4, .v = true, },
    { .i = 23, .x = 0x0cc, .v = true, },
    /* 40 - 49 */
    { .i = 24, .x = 0x0d4, .v = true, },
    { .i = 25, .x = 0x0dc, .v = true, },
    { .i = 26, .x = 0x0e4, .v = true, },
    { .i = 27, .x = 0x0ec, .v = true, },
    { .i = 28, .x = 0x0f4, .v = true, },
    { .i = 29, .x = 0x0fc, .v = true, },
    { .i = 30, .x = 0x104, .v = true, },
    { .i = 31, .x = 0x10c, .v = true, },
    { .i = 32, .x = 0x114, .v = true, },
    { .i = 33, .x = 0x11c, .v = true, },
    /* 50 - 59 */
    { .i = 34, .x = 0x124, .v = true, },
    { .i = 35, .x = 0x12c, .v = true, },
    { .i = 36, .x = 0x134, .v = true, },
    { .i = 37, .x = 0x13c, .v = true, },
    { .i = 38, .x = 0x144, .v = true, },
    { .i = 39, .x = 0x14c, .v = true, },
    { .i = -1, .x = 0x154, .v = true, },
    { .i = -1, .x = 0x15c, .v = true, },
    { .i = -1, .x = 0x164, .v = true, },
    { .i = -1, .x = 0x16c, .v = true, },
    /* 60 - 63 */
    { .i = -1, .x = 0x174, },
    { .i = -1, .x = 0x17c, },
    { .i = -1, .x = 0x184, },
};

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
    _left = 24;
    _right   = 344;
    if (!_40_columns) {
        _left = 31;
        _right   = 335;
    }

    _top = 51;
    _bottom   = 251;
    if (!_25_rows) {
        _top += 8 - _scroll_y;
        _bottom -= 8 -_scroll_y;
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
    _curr_pixel = _screen;
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

/* Fills internal 40*12 bits video matrix/color line buffer. */
static void c_access()
{
    uint8_t *from;
    uint16_t offset;

    /* Video matrix / chars */
    offset = ((_curr_y - 0x30) >> 3) * 40;
    from = _ram + _bank_offset + _video_matrix_addr + offset;
    memcpy(_curr_video_line, from, 40);

    /* Color data */
    from = _color_ram + offset;
    memcpy(_curr_color_line, from, 40);
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

static inline void check_y()
{
    if (_curr_y == _bottom) {
        _vert_flip_flop = true;
    }
    else if (_curr_y == _top &&
             _display_enable) {
        _vert_flip_flop = false;
    }
}

static inline void check_x()
{
    if (_curr_x == _right) {
        _main_flip_flop = true;
    }
    else if (_curr_x == _left &&
             _curr_y == _bottom) {
        _vert_flip_flop = true;
    }
    else if (_display_enable &&
             _curr_x == _left &&
             _curr_y == _top) {
        _vert_flip_flop = false;
    }

    if (_curr_x == _left && !_vert_flip_flop) {
        _main_flip_flop = false;
    }
}

static inline void draw_pixel()
{
    check_x();
    if (_main_flip_flop || _vert_flip_flop) {
        *_curr_pixel = _border_color;
    }
    else {
        if (_pixels & 0b10000000) {
            *_curr_pixel = _color_fg;
        }
        else {
            *_curr_pixel = _background_color0;
        }
    }
    _pixels = _pixels << 1;
    _curr_pixel++;
    _curr_x++;
}

static void inline read_pixels(struct cycle *cycle)
{
    uint8_t char_code    = _curr_video_line[cycle->i];
    int      char_line   = (_curr_y - _scroll_y) % 8;
    uint16_t char_offset = (char_code * (8)) + char_line;
    uint8_t  curr_pixels = g_access(char_offset);
    uint8_t  color_index = _curr_color_line[cycle->i] & 0x7f;
    uint32_t color_fg    = palette[color_index];

    _pixels = curr_pixels;
    _color_fg = color_fg;
}

void vic_step(bool *refresh, int* skip)
{
    struct cycle *cycle;

    /* Fast forward */
    if (_curr_y < 8 || _curr_y > 7+292) {
        *skip = 62;
        _curr_cycle = 62;
    }

    if (_curr_cycle == 5 &&
        _curr_y >= 0x30 && _curr_y <= 0xf7 &&
        ((_curr_y & 0b111) == _scroll_y)) {
        _curr_fetching = 40;
        c_access();
    }

    cycle = &_line_cycles[_curr_cycle];
    _curr_x = cycle->x;

    if (cycle->v) {
        /* Draw last 4 pixels from last cycle */
        draw_pixel();
        draw_pixel();
        draw_pixel();
        draw_pixel();

        read_pixels(cycle);
        draw_pixel();
        draw_pixel();
        draw_pixel();
        draw_pixel();
    }
    else {
        _curr_x += 8;
    }

    _curr_cycle++;
    if (_curr_cycle == 63) {
        _curr_y++;
        _curr_pixel = (uint32_t*)(((uint8_t*)_screen) + _pitch * _curr_y);
        if (_curr_y == 313) {
            _curr_y = 0;
            *refresh = true;
        }
        else {
            *skip = 5;
            _curr_cycle = 5;
        }
        check_y();
    }
}
