#include <stdio.h>
#include <stdint.h>

#include "pla.h"
#include "mem.h"

/* Input pins, true indicates high */
bool _pin_loram;
bool _pin_hiram;
bool _pin_charen;

uint8_t *_rom_kernal;
uint8_t *_rom_basic;
uint8_t *_rom_chargen;


uint8_t mem_get_kernal(uint16_t absolute, uint8_t relative,
                       uint8_t *ram)
{
    return _rom_kernal[absolute - 0xe000];
}

uint8_t mem_get_basic(uint16_t absolute, uint8_t relative,
                      uint8_t *ram)
{
    return _rom_basic[absolute - 0xa000];
}

static void configure()
{
    struct mem_hook_install installs[] = {
        /* Kernal */
        {
            .page_start = 0xe0,
            .num_pages = 32,
            .set_hook = NULL,
            .get_hook = _pin_hiram ? mem_get_kernal : NULL,
        },
        /* Basic */
        {
            .page_start = 0xa0,
            .num_pages = 8192 / 256,
            .set_hook = NULL,
            .get_hook = _pin_loram ? mem_get_basic : NULL,
        },
    };

    mem_install_hooks_for_cpu(installs, 2);
}


void pla_init(uint8_t *rom_kernal,
              uint8_t *rom_basic,
              uint8_t *rom_chargen)
{
    _pin_loram = false;
    _pin_hiram = false;
    _pin_charen = false;
    _rom_kernal = rom_kernal;
    _rom_basic = rom_basic;
    _rom_chargen = rom_chargen;
}

void pla_pins_from_cpu(bool loram,
                       bool hiram,
                       bool charen)
{
    _pin_loram = loram;
    _pin_hiram = hiram;
    _pin_charen = charen;
    configure();
}
