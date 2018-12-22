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

/* Input pins, true indicates high */
bool _pin_loram;
bool _pin_hiram;
bool _pin_charen;
/*
bool _pin_exrom;
bool _pin_game;
*/

uint8_t *_rom_kernal;
uint8_t *_rom_basic;
uint8_t *_rom_chargen;


static struct trace_point *_trace_banks;

bool _kernal_mapped;
bool _basic_mapped;
bool _charen_mapped;


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

uint8_t mem_get_charen(uint16_t absolute, uint8_t relative,
                       uint8_t *ram)
{
    return _rom_chargen[absolute - 0xd000];
}

static inline void trace_switch(const char *in, const char *out,
                                bool is, bool was, int fd)
{
    if (is && !was) {
        write(fd, in, strlen(in));
    }
    else if (!is && was) {
        write(fd, out, strlen(out));
    }
}

static inline void trace_banks()
{
    const char *kernal_in  = "Banking: KERNAL @ 0xe000\n";
    const char *kernal_out = "Banking: ROM @ 0xe000\n";
    const char *basic_in   = "Banking: BASIC @ 0xa000\n";
    const char *basic_out  = "Banking: RAM @ 0xa000\n";
    const char *charen_in  = "Banking: CHAREN @ 0xd000\n";
    const char *io_in      = "Banking: IO @ 0xd000\n";

    if (_trace_banks->fd >= 0) {
        trace_switch(kernal_in, kernal_out,
                     _pin_hiram, _kernal_mapped,
                     _trace_banks->fd);
        trace_switch(basic_in, basic_out,
                     _pin_loram, _basic_mapped,
                     _trace_banks->fd);
        trace_switch(charen_in, io_in,
                     !_pin_charen, _charen_mapped,
                     _trace_banks->fd);
    }
}

static void configure()
{
    /* GAME and EXROM affects I/O area but when this area is
     * mapped as I/O or charen. No support for game or extensions. */

    const struct mem_hook_install installs1[] = {
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
    mem_install_hooks_for_cpu(installs1, 2);

    const struct mem_hook_install installs2[] = {
        /* Charen */
        {
            .page_start = 0xd0,
            .num_pages = 4096 / 256,
            .set_hook = NULL,
            .get_hook = mem_get_charen,
        },
    };
    /* IO area is:
     * D000-D3FF    MOS 6567/6569 VIC-II Video Interface Controller
     * D400-D7FF    MOS 6581 SID Sound Interface Device
     * D800-DBFF    Color RAM (only lower nybbles are connected)
     * DC00-DCFF    MOS 6526 CIA Complex Interface Adapter #1
     * DD00-DDFF    MOS 6526 CIA Complex Interface Adapter #2
     * DE00-DEFF    User expansion #1 (-I/O1 on Expansion Port)
     * DF00-DFFF    User expansion #2 (-I/O2 on Expansion Port)
     */
    const struct mem_hook_install installs3[] = {
        {
            .page_start = 0xd0,
            .num_pages = 0x400 / 0x100,
            .set_hook = vic_mem_set,
            .get_hook = vic_mem_get,
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
    if (!_pin_charen) {
        mem_install_hooks_for_cpu(installs2, 1);
    }
    else {
        mem_install_hooks_for_cpu(installs3, 5);
    }

    trace_banks();
    _kernal_mapped = _pin_hiram;
    _basic_mapped = _pin_loram;
    _charen_mapped = !_pin_charen;
}


void pla_init(uint8_t *rom_kernal,
              uint8_t *rom_basic,
              uint8_t *rom_chargen)
{
    _pin_loram = false;
    _pin_hiram = false;
    _pin_charen = false;
    /* Pull-up resistor, 1 by default */
    /*
    _pin_exrom = true;
    _pin_game = true;
    */

    _rom_kernal = rom_kernal;
    _rom_basic = rom_basic;
    _rom_chargen = rom_chargen;
    _kernal_mapped = false;
    _basic_mapped = false;

    /* Debugging */
    _trace_banks = trace_add_point("PLA", "banks");
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

bool pla_is_basic_mapped()
{
    return _basic_mapped;
}
