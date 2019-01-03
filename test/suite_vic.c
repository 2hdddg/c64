#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "vic.h"

uint8_t _ram[0xffff];
uint8_t _color_ram[1000];
uint8_t _char_rom[4096];
uint32_t _screen[500*500];
uint32_t _pitch = 500;

/* Defined in vic_palette.c */
uint32_t palette[16];

/* Where screen pixels start */
#define SCREEN_START_TOP      9
#define SCREEN_START_LEFT     0
/* Size of borders, 40 cols/25 lines */
#define BORDER_WIDTH_LEFT    46
#define BORDER_WIDTH_RIGHT   36
#define BORDER_HEIGHT_TOP    43
#define BORDER_HEIGHT_BOTTOM 49

/* -2 might not be right... */
#define NUM_LINES (43 + 200 + 49 - 2)
#define NUM_COLS  (46 + 320 + 36 - 2)

/* Coordinates of first drawable pixels, 40 cols/25 lines */
#define START_X 52
#define START_Y 42

#define CHAR1_INDEX 7
#define CHAR2_INDEX 38
uint8_t _char1[] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
};
uint8_t _char2[] = {
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
};

static void set_reg(uint16_t reg, uint8_t val)
{
    vic_reg_set(val, 0xd000 + reg, 0, NULL);
}

static uint8_t get_reg(uint16_t reg)
{
    return vic_reg_get(0xd000 + reg, 0, NULL);
}

static void render_frame()
{
    bool refresh = false;
    int skip = 0;
    bool stall_cpu = false;

    while (!refresh) {
        vic_step(&refresh, &skip, &stall_cpu);
    }
}

int once_before()
{
    vic_init(_char_rom, _ram, _color_ram);
    vic_screen(_screen, _pitch*4 /* In bytes*/);
    return 0;
}

int each_before()
{
    memset(_ram, 0, sizeof(_ram));
    memset(_color_ram, 0, sizeof(_color_ram));
    memset(_screen, 0, sizeof(_screen));
    memset(_char_rom, 0, sizeof(_char_rom));
    /* Copy example char pixels to char rom */
    memcpy(_char_rom+(CHAR1_INDEX*8), _char1, sizeof(_char1));
    memcpy(_char_rom+(CHAR2_INDEX*8), _char2, sizeof(_char2));

    vic_reset();
    vic_set_bank(vic_bank_0);

    /* Setup defaults after boot */
    set_reg(VIC_REG_SCROLY,
            3 | VIC_SCROLY_ROW_25 | VIC_SCROLY_DISPLAY_EN);
    set_reg(VIC_REG_SCROLX,
            VIC_SCROLX_COL_40);
    set_reg(VIC_REG_EXTCOL, 14);
    set_reg(VIC_REG_BGCOL0, 6);
    set_reg(VIC_REG_VMCSB, 4 | (1 << 4));

    return 0;
}

int test_all_borders_when_display_disabled()
{
    uint32_t color;
    uint32_t *line;

    set_reg(VIC_REG_SCROLY,
            get_reg(VIC_REG_SCROLY) & ~VIC_SCROLY_DISPLAY_EN);
    render_frame();

    for (int y = 0; y < NUM_LINES; y++) {
        line = _screen + ((SCREEN_START_TOP + y) * _pitch) +
               SCREEN_START_LEFT;
        for (int x = 0; x < NUM_COLS; x++) {
            color = line[x];

            if (color != palette[14]) {
                vic_stat();
                printf("Expected border: %08x at %d,%d "
                       "but was %08x\n", palette[14], x, y, color);
                return 0;
            }
        }
    }

    return 1;
}

/* Counts number of background pixels on the complete screen to
 * verify height. */
int test_height()
{
    uint32_t color;
    uint32_t *line;
    int x = NUM_COLS / 2; /* Assume center pixel is screen */
    bool found = false;
    int num_lines = 0;

    render_frame();

    for (int y = 0; y < NUM_LINES; y++) {
        line = _screen + ((SCREEN_START_TOP + y) * _pitch) +
               SCREEN_START_LEFT;
        color = line[x];
        if (color == palette[6]) {
            if (!found) {
                num_lines = 1;
                found = true;
                printf("Found start of disp at %d\n", y);
            }
            else {
                num_lines++;
            }
        }
        else {
            if (found) {
                break;
            }
        }
    }

    if (num_lines != 200) {
        vic_stat();
        printf("Expected height 200 but was %d\n", num_lines);
        return 0;
    }

    return 1;
}

/* Counts number of background pixels on the complete screen to
 * verify width. */
int test_width()
{
    uint32_t color;
    uint32_t *line;
    int y = NUM_LINES / 2;
    bool found = false;
    int num_cols = 0;

    render_frame();

    line = _screen + ((SCREEN_START_TOP + y) * _pitch) +
           SCREEN_START_LEFT;
    for (int x = 0; x < NUM_COLS; x++) {
        color = line[x];
        if (color == palette[6]) {
            if (!found) {
                num_cols = 1;
                found = true;
                printf("Found start of disp at %d\n", x);
            }
            else {
                num_cols++;
            }
        }
        else {
            if (found) {
                break;
            }
        }
    }

    if (num_cols != 320) {
        vic_stat();
        printf("Expected width 320 but was %d\n", num_cols);
        return 0;
    }

    return 1;
}

/* Verifies that borders are on the correct places */
int test_borders()
{
    uint32_t color;
    uint32_t *line;
    int num_bg = 0;

    render_frame();

    for (int y = 0; y < NUM_LINES; y++) {
        line = _screen + ((SCREEN_START_TOP + y) * _pitch) +
               SCREEN_START_LEFT;
        for (int x = 0; x < NUM_COLS; x++) {
            color = line[x];

            /* All border */
            if (y < START_Y || y >= START_Y+200 ||
                x < START_X || x >= START_X+320) {
                if (color != palette[14]) {
                    printf("Expected border: %08x at %d,%d "
                           "but was %08x\n", palette[14], x, y, color);
                    return 0;
                }
            }
            else {
                if (color != palette[6]) {
                    printf("Expected bg: %08x at %d,%d "
                           "but was %08x\n", palette[6], x, y, color);
                    return 0;
                }
                num_bg++;
            }
        }
    }

    if (num_bg != 320*200) {
        return 0;
    }

    return 1;
}

static uint32_t get_drawable_pixel(int x, int y)
{
    uint32_t *line = _screen;

    line += (SCREEN_START_TOP + y + START_Y) * _pitch;
    return line[SCREEN_START_LEFT + START_X + x];
}

/* Reads 8x8 pixels */
static void read_char_pixels(int xstart, int ystart, uint32_t *pixels)
{
    for (int y = ystart; y < 8; y++) {
        for (int x = xstart; x < 8; x++) {
            *pixels = get_drawable_pixel(x, y);
            pixels++;
        }
    }
}

static uint8_t pixels_to_char_line(uint32_t *pixels, uint32_t color)
{
    uint8_t line = 0;

    for (int i = 0; i < 8; i++) {
        line = line << 1;
        if (*pixels == color) {
            line |= 1;
        }
        pixels++;
    }
    return line;
}

int test_render_chars()
{
    uint8_t *video_matrix = _ram + 0x400;
    uint8_t *color_ram = _color_ram;
    uint32_t char_pixels[8*8];
    uint8_t  line;

    /* Fill ram with lines of char1 and char2 */
    for (int i = 0; i < 25; i++) {
        memset(video_matrix,
               i & 0x01 ? CHAR2_INDEX : CHAR1_INDEX, 40);
        /* Fill color ram with red or yellow */
        memset(_color_ram, i & 0x01 ? 2 : 7, 40);

        video_matrix += 40;
        color_ram += 40;
    }

    render_frame();

    for (int row = 0; row < 1/*25*/; row++) {
        uint8_t *the_char = row & 0x01 ? _char2 : _char1;
        uint32_t the_color = row & 0x01 ? palette[2] : palette[7];
        for (int col = 0; col < 40; col++) {
            read_char_pixels(col * 8, row * 8, char_pixels);
            /* Check line by line in char */
            for (int i = 0; i < 8; i++) {
                line = pixels_to_char_line(char_pixels + (i * 8),
                                           the_color);
                if (line != the_char[i]) {
                    vic_stat();
                    printf("Char line %d on row %d col %d should be "
                           "%02x but was %02x\n",
                           i, row, col, the_char[i], line);
                    return 0;
                }
            }
        }
    }

    return 1;
}

