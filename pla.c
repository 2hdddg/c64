#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "pla.h"
#include "mem.h"

/* Input pins, true indicates high */
bool _pin_loram;
bool _pin_hiram;
bool _pin_charen;

uint8_t *_rom_kernal;
uint8_t *_rom_basic;
uint8_t *_rom_chargen;


int _trace_banks_fd;
bool _kernal_mapped;
bool _basic_mapped;


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
    const char *basic_in  = "Banking: BAIC @ 0xa000\n";
    const char *basic_out = "Banking: RAM @ 0xa000\n";

    if (_trace_banks_fd >= 0) {
        trace_switch(kernal_in, kernal_out,
                     _pin_hiram, _kernal_mapped,
                     _trace_banks_fd);
        trace_switch(basic_in, basic_out,
                     _pin_loram, _basic_mapped,
                     _trace_banks_fd);
    }
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

    trace_banks();
    _kernal_mapped = _pin_hiram;
    _basic_mapped = _pin_loram;
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
    _trace_banks_fd = -1;
    _kernal_mapped = false;
    _basic_mapped = false;
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

void pla_trace_banks(int fd)
{
    _trace_banks_fd = fd;
}
