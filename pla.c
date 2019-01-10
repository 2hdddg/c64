#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "pla.h"
#include "mem.h"
#include "cia1.h"
#include "cia2.h"
#include "vic.h"
#include "sid.h"
#include "trace.h"


/* Images */
uint8_t *_rom_kernal;
uint8_t *_rom_basic;
uint8_t *_rom_chargen;

/* Pins */
uint8_t _loram;
uint8_t _hiram;
uint8_t _charen;
uint8_t _exrom;
uint8_t _game;

/* Current bank configuration */
uint8_t _config_index;

static struct trace_point *_trace_banks;

typedef enum {
    RAM,
    BASIC,
    IO,
    KERNAL,
    CHAR,
    OPEN,
    CARTLO,
    CARTHI,
} bank_config;

#define LORAM_VAL  1
#define HIRAM_VAL  2
#define CHAREN_VAL 4
#define GAME_VAL   8
#define EXROM_VAL 16

struct config {
    bank_config page_000_015; /* Not needed, always RAM */
    bank_config page_016_127;
    bank_config page_128_159;
    bank_config page_160_191;
    bank_config page_192_207;
    bank_config page_208_223;
    bank_config page_224_255;
};

struct config _configs[] = {
    /* Configurations 0-23 are cartridge modes */
    { RAM,    RAM,    RAM,    RAM,    RAM,    RAM,    RAM }, /*  0 */
    { RAM,    RAM,    RAM,    RAM,    RAM,    RAM,    RAM }, /*  1 */
    { RAM,    RAM,    RAM, CARTHI,    RAM,   CHAR, KERNAL }, /*  2 */
    { RAM,    RAM, CARTLO, CARTHI,    RAM,   CHAR, KERNAL }, /*  3 */
    { RAM,    RAM,    RAM,    RAM,    RAM,    RAM,    RAM }, /*  4 */
    { RAM,    RAM,    RAM,    RAM,    RAM,     IO,    RAM }, /*  5 */
    { RAM,    RAM,    RAM, CARTHI,    RAM,     IO, KERNAL }, /*  6 */
    { RAM,    RAM, CARTLO, CARTHI,    RAM,     IO, KERNAL }, /*  7 */
    { RAM,    RAM,    RAM,    RAM,    RAM,    RAM,    RAM }, /*  8 */
    { RAM,    RAM,    RAM,    RAM,    RAM,   CHAR,    RAM }, /*  9 */
    { RAM,    RAM,    RAM,    RAM,    RAM,   CHAR, KERNAL }, /* 10 */
    { RAM,    RAM, CARTLO,  BASIC,    RAM,   CHAR, KERNAL }, /* 11 */
    { RAM,    RAM,    RAM,    RAM,    RAM,    RAM,    RAM }, /* 12 */
    { RAM,    RAM,    RAM,    RAM,    RAM,     IO,    RAM }, /* 13 */
    { RAM,    RAM,    RAM,    RAM,    RAM,     IO, KERNAL }, /* 14 */
    { RAM,    RAM, CARTLO,  BASIC,    RAM,     IO, KERNAL }, /* 15 */
    { RAM,   OPEN, CARTLO,   OPEN,   OPEN,     IO, CARTHI }, /* 16 */
    { RAM,   OPEN, CARTLO,   OPEN,   OPEN,     IO, CARTHI }, /* 17 */
    { RAM,   OPEN, CARTLO,   OPEN,   OPEN,     IO, CARTHI }, /* 18 */
    { RAM,   OPEN, CARTLO,   OPEN,   OPEN,     IO, CARTHI }, /* 19 */
    { RAM,   OPEN, CARTLO,   OPEN,   OPEN,     IO, CARTHI }, /* 20 */
    { RAM,   OPEN, CARTLO,   OPEN,   OPEN,     IO, CARTHI }, /* 21 */
    { RAM,   OPEN, CARTLO,   OPEN,   OPEN,     IO, CARTHI }, /* 22 */
    { RAM,   OPEN, CARTLO,   OPEN,   OPEN,     IO, CARTHI }, /* 23 */
    /* Non cartridge modes */
    { RAM,    RAM,    RAM,    RAM,    RAM,    RAM,    RAM }, /* 24 */
    { RAM,    RAM,    RAM,    RAM,    RAM,   CHAR,    RAM }, /* 25 */
    { RAM,    RAM,    RAM,    RAM,    RAM,   CHAR, KERNAL }, /* 26 */
    { RAM,    RAM,    RAM,  BASIC,    RAM,   CHAR, KERNAL }, /* 27 */
    { RAM,    RAM,    RAM,    RAM,    RAM,    RAM,    RAM }, /* 28 */
    { RAM,    RAM,    RAM,    RAM,    RAM,     IO,    RAM }, /* 29 */
    { RAM,    RAM,    RAM,    RAM,    RAM,     IO, KERNAL }, /* 30 */
    { RAM,    RAM,    RAM,  BASIC,    RAM,     IO, KERNAL }, /* 31 */

    /* Dummy */
    { OPEN,  OPEN,   OPEN,   OPEN,   OPEN,   OPEN,   OPEN }, /* 32 */
};

static uint8_t mem_get_kernal(uint16_t absolute, uint8_t relative,
                              uint8_t *ram)
{
    return _rom_kernal[absolute - 0xe000];
}

static uint8_t mem_get_basic(uint16_t absolute, uint8_t relative,
                             uint8_t *ram)
{
    return _rom_basic[absolute - 0xa000];
}

static uint8_t mem_get_charen(uint16_t absolute, uint8_t relative,
                              uint8_t *ram)
{
    return _rom_chargen[absolute - 0xd000];
}

const struct mem_hook_install _kernal_hook = {
    .page_start = 0xe0,
    .num_pages  = 32,
    .get_hook   = mem_get_kernal,
};

const struct mem_hook_install _basic_hook = {
    .page_start = 0xa0,
    .num_pages  = 8192 / 256,
    .get_hook   = mem_get_basic,
};

const struct mem_hook_install _charen_hook = {
    .page_start = 0xd0,
    .num_pages  = 4096 / 256,
    .get_hook   = mem_get_charen,
};

/* IO area is:
 * D000-D3FF  MOS 6567/6569 VIC-II Video Interface Controller
 * D400-D7FF  MOS 6581 SID Sound Interface Device
 * D800-DBFF  Color RAM (only lower nybbles are connected)
 * DC00-DCFF  MOS 6526 CIA Complex Interface Adapter #1
 * DD00-DDFF  MOS 6526 CIA Complex Interface Adapter #2
 * DE00-DEFF  User expansion #1 (-I/O1 on Expansion Port)
 * DF00-DFFF  User expansion #2 (-I/O2 on Expansion Port)
 */
const struct mem_hook_install _io_hooks[] = {
    {
        .page_start = 0xd0,
        .num_pages = 0x400 / 0x100,
        .set_hook = vic_reg_set,
        .get_hook = vic_reg_get,
    },
    {
        .page_start = 0xd4,
        .num_pages = 0x400 / 0x100,
        .set_hook = sid_mem_set,
        .get_hook = sid_mem_get,
    },
    {
        .page_start = 0xd8,
        .num_pages = 0x400 / 0x100,
        .set_hook = mem_color_ram_set,
        .get_hook = mem_color_ram_get,
    },
    {
        .page_start = 0xdc,
        .num_pages = 0x100 / 0x100,
        .set_hook = cia1_mem_set,
        .get_hook = cia1_mem_get,
    },
    {
        .page_start = 0xdd,
        .num_pages = 0x100 / 0x100,
        .set_hook = cia2_mem_set,
        .get_hook = cia2_mem_get,
    },
};

static uint8_t calculate_config_index()
{
    return _loram + _hiram + _charen + _exrom + _game;
}

static const char* get_bank_config_name(bank_config config)
{
    switch (config) {
    case RAM:
        return "RAM";
    case BASIC:
        return "Basic ROM";
    case IO:
        return "IO";
    case KERNAL:
        return "Kernal ROM";
    case CHAR:
        return "Character ROM";
    case OPEN:
        return "Unmapped";
    case CARTLO:
        return "Cartridge, low";
    case CARTHI:
        return "Cartridge, high";
    default:
        return "Unknown";
    }
}

static void apply_config(struct config *new, struct config *old)
{
    struct mem_hook_install ram_hook = { };

    /* RAM or BASIC until cartridge supported then CARTHI */
    if (new->page_160_191 != old->page_160_191) {
        switch (new->page_160_191) {
        case RAM:
            ram_hook.page_start = 160;
            ram_hook.num_pages  = 32;
            mem_install_hooks_for_cpu(&ram_hook, 1);
            break;
        case BASIC:
            mem_install_hooks_for_cpu(&_basic_hook, 1);
            break;
        default:
            break;
        }
        TRACE(_trace_banks, "Mapped banks a0-bf to %s",
              get_bank_config_name(new->page_160_191));
    }

    /* RAM, IO or CHAREN */
    if (new->page_208_223 != old->page_208_223) {
        switch (new->page_208_223) {
        case RAM:
            ram_hook.page_start = 208;
            ram_hook.num_pages  = 16;
            mem_install_hooks_for_cpu(&ram_hook, 1);
            break;
        case IO:
            mem_install_hooks_for_cpu(_io_hooks,
                sizeof(_io_hooks) / sizeof(_io_hooks[0]));
            break;
        case CHAR:
            mem_install_hooks_for_cpu(&_charen_hook, 1);
            break;
        default:
            break;
        }
        TRACE(_trace_banks, "Mapped banks d0-df to %s",
              get_bank_config_name(new->page_208_223));
    }

    /* RAM OR KERNAL until cartridge/game supported, than CARTHI */
    if (new->page_224_255 != old->page_224_255) {
        switch (new->page_224_255) {
        case RAM:
            ram_hook.page_start = 224;
            ram_hook.num_pages  = 32;
            mem_install_hooks_for_cpu(&ram_hook, 1);
            break;
        case KERNAL:
            mem_install_hooks_for_cpu(&_kernal_hook, 1);
            break;
        default:
            break;
        }
        TRACE(_trace_banks, "Mapped banks e0-ff to %s",
              get_bank_config_name(new->page_224_255));
    }
}

void pla_init(uint8_t *rom_kernal,
              uint8_t *rom_basic,
              uint8_t *rom_chargen)
{
    _rom_kernal   = rom_kernal;
    _rom_basic   = rom_basic;
    _rom_chargen = rom_chargen;

    /* Pull-up resistors, high by default */
    _loram  = LORAM_VAL;
    _hiram  = HIRAM_VAL;
    _charen = CHAREN_VAL;
    _exrom  = EXROM_VAL;
    _game   = GAME_VAL;

    _config_index = calculate_config_index();
    apply_config(&_configs[_config_index], &_configs[32]);

    /* Debugging */
    _trace_banks = trace_add_point("PLA", "banks");
}

void pla_pins_from_cpu(bool pin_loram,
                       bool pin_hiram,
                       bool pin_charen)
{
    uint8_t prev_config_index = _config_index;

    _config_index = calculate_config_index();
    apply_config(&_configs[_config_index],
                 &_configs[prev_config_index]);
}

bool pla_is_basic_mapped()
{
    /* Basic can only reside in these pages */
    return _configs[_config_index].page_160_191 == BASIC;
}

void pla_stat()
{
    struct config *c = &_configs[_config_index];

    printf("PLA\n");
    printf("Pages  Content\n");
    printf("00-0f  %s\n", get_bank_config_name(c->page_000_015));
    printf("10-7f  %s\n", get_bank_config_name(c->page_016_127));
    printf("80-9f  %s\n", get_bank_config_name(c->page_128_159));
    printf("a0-bf  %s\n", get_bank_config_name(c->page_160_191));
    printf("c0-ef  %s\n", get_bank_config_name(c->page_192_207));
    printf("d0-df  %s\n", get_bank_config_name(c->page_208_223));
    printf("e0-ff  %s\n", get_bank_config_name(c->page_224_255));
}

