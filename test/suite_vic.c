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
/* Size of borders, 25 cols/40 rows */
#define BORDER_WIDTH_LEFT    46
#define BORDER_WIDTH_RIGHT   36
#define BORDER_HEIGHT_TOP    43
#define BORDER_HEIGHT_BOTTOM 49

#define NUM_LINES (43 + 200 + 49 - 2)
#define NUM_COLS  (46 + 320 + 36 - 2)


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
    memset(_ram, 0, 0xffff);
    memset(_color_ram, 0, 1000);
    memset(_screen, 0, 500*500);
    vic_reset();

    /* Setup defaults after boot */
    set_reg(VIC_REG_SCROLY,
            3 | VIC_SCROLY_ROW_25 | VIC_SCROLY_DISPLAY_EN);
    set_reg(VIC_REG_SCROLX,
            VIC_SCROLX_COL_40);
    set_reg(VIC_REG_EXTCOL, 14);
    set_reg(VIC_REG_BGCOL0, 6);

    return 0;
}

int test_all_borders_when_display_disabled()
{
    uint32_t color;
    uint32_t *line;

    set_reg(VIC_REG_SCROLY,
            get_reg(VIC_REG_SCROLY) & ~VIC_SCROLY_DISPLAY_EN);

    vic_stat();
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

//52, 42
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
            if (y < 42 || y >= 42+200 ||
                x < 52 || x >= 52+320) {
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

