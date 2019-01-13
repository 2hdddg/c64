#include <stdio.h>
#include <string.h>

#include "emulation/mem.h"
#include "emulation/basic.h"
#include "emulation/kernal.h"
#include "emulation/pla.h"


static uint8_t _basic_rom[8192];
static uint8_t _kernal_rom[8192];
static uint8_t _chargen_rom[4096];

#define BASIC_VAL 0x55
#define KERNAL_VAL 0x65
#define CHARGEN_VAL 0x75

/* Mock IO interfaces */

uint8_t vic_reg_get(uint16_t absolute, uint8_t relative,
                    uint8_t *ram)
{
    return 0;
}

void vic_reg_set(uint8_t val, uint16_t absolute,
                 uint8_t relative, uint8_t *ram)
{
}

uint8_t sid_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    return 0;
}

void sid_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
}

uint8_t cia1_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    return 0;
}

void cia1_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
}

uint8_t cia2_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    return 0;
}

void cia2_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
}

int once_before()
{
    mem_init();
    pla_init(_kernal_rom, _basic_rom, _chargen_rom);
    return 0;
}

int each_before()
{
    mem_reset();
    pla_reset();
    memset(_basic_rom, BASIC_VAL, sizeof(_basic_rom));
    memset(_kernal_rom, KERNAL_VAL, sizeof(_kernal_rom));
    memset(_chargen_rom, CHARGEN_VAL, sizeof(_chargen_rom));
    return 0;
}

static bool probe_basic(bool *error)
{
    bool pla_mapped = pla_is_basic_mapped();
    bool mem_mapped = mem_get_for_cpu(basic_address()) == BASIC_VAL;

    if (pla_mapped != mem_mapped) {
        printf("PLA and memory has different opinion on if Basic "
               "is mapped or not!\n");
        *error = true;
    }
    return pla_mapped;
}

static bool probe_kernal(bool *error)
{
    bool pla_mapped = pla_is_kernal_mapped();
    bool mem_mapped = mem_get_for_cpu(kernal_address()) == KERNAL_VAL;

    if (pla_mapped != mem_mapped) {
        printf("PLA and memory has different opinion on if Kernal "
               "is mapped or not!\n");
        *error = true;
    }
    return pla_mapped;
}

#if 0
static bool probe_io()
{
    return false;
}

static bool probe_char()
{
    return false;
}

static bool probe_ram_at(uint16_t addr)
{
    return false;
}

#endif

int test_map_after_reset()
{
    bool error = false;

    /* Basic, IO and kernal should be mapped, not char */
    if (!probe_basic(&error) || error) {
        printf("Basic should be mapped\n");
        return error ? -1 : 0;
    }
    if (!probe_kernal(&error) || error) {
        printf("Kernal should be mapped\n");
        return error ? -1 : 0;
    }
#if 0
    if (!probe_io()) {
        printf("IO should be mapped\n");
        return 0;
    }
    if (probe_char()) {
        printf("Char should NOT be mapped\n");
        return 0;
    }
#endif
    return 1;
}


