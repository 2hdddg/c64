#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "trace.h"
#include "vic.h"
#include "snapshot.h"

/* Emulates Video Interface Controller II (VIC-II).
 * PAL version (6569).
 */

struct cycle {
    uint16_t x;
    bool     v; /* Visible */
};

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
uint16_t _bitmap_data_addr  = 0x0000;
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
uint16_t _curr_fetching = 0;
uint32_t *_curr_pixel;

/* Filled during bad line */
#define LINE_OFFSET 3
uint8_t _curr_video_line[40+LINE_OFFSET];
uint8_t _curr_color_line[40+LINE_OFFSET];

static bool _main_flip_flop;
static bool _vert_flip_flop;

int _curr_cycle = 0;

uint8_t _pixels;
uint32_t _color_fg;

struct cycle _line_cycles[63] = {
    /* 0 - 9 */
    { .x = 0x194, }, { .x = 0x19c, },
    { .x = 0x1a4, }, { .x = 0x1ac, },
    { .x = 0x1b4, }, { .x = 0x1bc, },
    { .x = 0x1c4, }, { .x = 0x1cc, },
    { .x = 0x1d4, }, { .x = 0x1dc, },
    /* 10 - 19 */
    { .x = 0x1e4, .v = true, },
    { .x = 0x1ec, .v = true, },
    { .x = 0x1f4, .v = true, },
    { .x = 0x1fc, .v = true, },
    { .x = 0x004, .v = true, },
    { .x = 0x00c, .v = true, },
    { .x = 0x014, .v = true, },
    { .x = 0x01c, .v = true, },
    { .x = 0x024, .v = true, },
    { .x = 0x02c, .v = true, },
    /* 20 - 29 */
    { .x = 0x034, .v = true, },
    { .x = 0x03c, .v = true, },
    { .x = 0x044, .v = true, },
    { .x = 0x04c, .v = true, },
    { .x = 0x054, .v = true, },
    { .x = 0x05c, .v = true, },
    { .x = 0x064, .v = true, },
    { .x = 0x06c, .v = true, },
    { .x = 0x074, .v = true, },
    { .x = 0x07c, .v = true, },
    /* 30 - 39 */
    { .x = 0x084, .v = true, },
    { .x = 0x08c, .v = true, },
    { .x = 0x094, .v = true, },
    { .x = 0x09c, .v = true, },
    { .x = 0x0a4, .v = true, },
    { .x = 0x0ac, .v = true, },
    { .x = 0x0b4, .v = true, },
    { .x = 0x0bc, .v = true, },
    { .x = 0x0c4, .v = true, },
    { .x = 0x0cc, .v = true, },
    /* 40 - 49 */
    { .x = 0x0d4, .v = true, },
    { .x = 0x0dc, .v = true, },
    { .x = 0x0e4, .v = true, },
    { .x = 0x0ec, .v = true, },
    { .x = 0x0f4, .v = true, },
    { .x = 0x0fc, .v = true, },
    { .x = 0x104, .v = true, },
    { .x = 0x10c, .v = true, },
    { .x = 0x114, .v = true, },
    { .x = 0x11c, .v = true, },
    /* 50 - 59 */
    { .x = 0x124, .v = true, },
    { .x = 0x12c, .v = true, },
    { .x = 0x134, .v = true, },
    { .x = 0x13c, .v = true, },
    { .x = 0x144, .v = true, },
    { .x = 0x14c, .v = true, },
    { .x = 0x154, .v = true, },
    { .x = 0x15c, .v = true, },
    { .x = 0x164, .v = true, },
    { .x = 0x16c, .v = true, },
    /* 60 - 63 */
    { .x = 0x174, },
    { .x = 0x17c, },
    { .x = 0x184, },
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
    printf("Bitmap data    : %04x\n", _bitmap_data_addr);
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

void vic_reset()
{
    _bitmap_graphics     = false;
    _extended_color_text = false;
    _display_enable      = false;
    _multicolor          = false;
    _40_columns          = false;
    _25_rows             = false;
    _reset               = false;
    _char_pixels_addr    = 0x0000;
    _bitmap_data_addr    = 0x0000;
    _video_matrix_addr   = 0x0000;
    _bank                = 0x00;
    _bank_offset         = 0x0000;
    _char_rom_offset     = 0x0000;
    _scroll_y            = 0;
    _scroll_x            = 0;
    _raster_compare      = 0;
    _interrupt_mask      = 0;
    _interrupt_flag      = 0;
    _border_color        = 0;
    _background_color0   = 0;
    _background_color1   = 0;
    _background_color2   = 0;
    _curr_y              = 0;
    _curr_x              = 0;
    _curr_fetching       = 0;
    _curr_pixel          = _screen;
    _main_flip_flop      = true;
    _vert_flip_flop      = true;
    _curr_cycle          = 0;
    _pixels              = 0;
    _color_fg            = 0;

    memset(_curr_video_line, 0, 40+LINE_OFFSET);
    memset(_curr_color_line, 0, 40+LINE_OFFSET);
    memset(_raw_regs, 0, 0x40);

    _setup_drawable_area();
    vic_set_bank(vic_bank_0);
}

void vic_init(uint8_t *char_rom,
              uint8_t *ram,
              uint8_t *color_ram)
{
    _char_rom  = char_rom;
    _ram       = ram;
    _color_ram = color_ram;

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
    uint16_t offset = (absolute - 0xd000) % 0x40;

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

    /* Vertical fine scrolling and control */
    case VIC_REG_SCROLY:
        _scroll_y            = (val & 0b00000111);
        _25_rows             = (val & VIC_SCROLY_ROW_25) > 0;
        _display_enable      = (val & VIC_SCROLY_DISPLAY_EN) > 0;
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
    /* Horizontal fine scrolling and control */
    case VIC_REG_SCROLX:
        _40_columns = (val & VIC_SCROLX_COL_40) > 0;
        _multicolor = (val & VIC_SCROLX_MULTICOLOR) > 0;
        _scroll_x   = (val & VIC_SCROLX_SCROLL);
        _reset      = (val & VIC_SCROLX_RESET) > 0;
        _setup_drawable_area();
        break;
    /* Memory control register */
    case VIC_REG_VMCSB:
        _char_pixels_addr  = (val & VIC_VMCSB_CHAR_PIX_ADDR) *
                             1024;
        _bitmap_data_addr  = ((val & VIC_VMCSB_CHAR_PIX_ADDR) >> 3) *
                             8192;
        _video_matrix_addr = ((val & VIC_VMCSB_VID_MATR_ADDR) >> 4) *
                             1024;
        break;
    /* VICIRQ */
    case 0x19:
        /* Read only ? */
        break;
    /* IRQMASK */
    case 0x1a:
        _interrupt_mask = val;
        break;
    case VIC_REG_EXTCOL:
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
    memcpy(_curr_video_line+LINE_OFFSET, from, 40);

    /* Color data */
    from = _color_ram + offset;
    memcpy(_curr_color_line+LINE_OFFSET, from, 40);
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

static void draw_pixel_standard_text_mode()
{
    if ((_curr_x & 0b111) == _scroll_x) {
        /* G access */
        int      index  = _curr_x / 8;
        uint8_t  code   = _curr_video_line[index];
        int      line   = (_curr_y - _scroll_y) % 8;
        uint16_t offset = (code * (8)) + line;
        uint8_t  color  = _curr_color_line[index] & 0x7f;
        uint16_t addr   = _char_pixels_addr + offset;

        if (_char_rom_offset > 0 &&
            addr >= _char_rom_offset &&
            addr < _char_rom_offset + 0x1000) {
            _pixels = _char_rom[offset];
        }
        else {
            _pixels = _ram[offset];
        }
        _color_fg = palette[color];
    }
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

static void draw_pixel_standard_bitmap_mode()
{
    if ((_curr_x & 0b111) == _scroll_x) {
        int      index  = _curr_x / 8;
        int      y      = ((_curr_y - 0x30) >> 3) * 40;
        int      line   = (_curr_y - _scroll_y) % 8;
        uint16_t offset = y + (index * 8) + line;
        uint16_t addr   = _bitmap_data_addr + offset;

        _pixels = _ram[addr];
        _color_fg = _curr_video_line[index];
    }
    check_x();
    if (_main_flip_flop || _vert_flip_flop) {
        *_curr_pixel = _border_color;
    }
    else {
        if (_pixels & 0b10000000) {
            *_curr_pixel = palette[_color_fg >> 4];
        }
        else {
            *_curr_pixel = palette[_color_fg & 0x7f];
        }
    }
    _pixels = _pixels << 1;
    _curr_pixel++;
    _curr_x++;

}

void vic_step(bool *refresh, int* skip, bool *stall_cpu)
{
    struct cycle *cycle;

    /* Fast forward */
    if (_curr_y < 8 || _curr_y > 7+292) {
        *skip = 62;
        _curr_cycle = 62;
    }

    if (_curr_fetching) {
        /* Filling lines */
        _curr_fetching--;
        *stall_cpu = true;
    }
    else if (_curr_cycle == 5 &&
        /* Need to start filling lines */
        _curr_y >= 0x30 && _curr_y <= 0xf7 &&
        ((_curr_y & 0b111) == _scroll_y)) {
        _curr_fetching = 40;
        c_access();
    }

    cycle = &_line_cycles[_curr_cycle];
    _curr_x = cycle->x;

    if (cycle->v) {
        int num   = 8;

        while (num--) {
            if (!_bitmap_graphics) {
                draw_pixel_standard_text_mode();
            }
            else {
                draw_pixel_standard_bitmap_mode();
            }
        }
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

void vic_snapshot(const char *name)
{
    snap_screen(_screen, _pitch, 400, 400, name);
}
