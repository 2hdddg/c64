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

static uint8_t _vic_reg_val;
static uint8_t _sid_reg_val;
static uint8_t _cia1_reg_val;
static uint8_t _cia2_reg_val;

/* Mock IO interfaces */

uint8_t vic_reg_get(uint16_t absolute, uint8_t relative,
                    uint8_t *ram)
{
    return _vic_reg_val;
}

void vic_reg_set(uint8_t val, uint16_t absolute,
                 uint8_t relative, uint8_t *ram)
{
    _vic_reg_val = val;
}

uint8_t sid_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    return _sid_reg_val;
}

void sid_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
    _sid_reg_val = val;
}

uint8_t cia1_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    return _cia1_reg_val;
}

void cia1_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
    _cia1_reg_val = val;
}

uint8_t cia2_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    return _cia2_reg_val;
}

void cia2_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
    _cia2_reg_val = val;
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

static bool probe_basic()
{
    bool pla_mapped = pla_is_basic_mapped();
    bool mem_mapped = mem_get_for_cpu(basic_address()) == BASIC_VAL;

    if (pla_mapped != mem_mapped) {
        printf("PLA and memory has different opinion on if Basic "
               "is mapped or not!\n");
    }
    return pla_mapped;
}

static bool probe_kernal()
{
    bool pla_mapped = pla_is_kernal_mapped();
    bool mem_mapped = mem_get_for_cpu(kernal_address()) == KERNAL_VAL;

    if (pla_mapped != mem_mapped) {
        printf("PLA and memory has different opinion on if Kernal "
               "is mapped or not!\n");
    }
    return pla_mapped;
}

static bool probe_io()
{
    bool pla_mapped = pla_is_io_mapped();
    bool mem_mapped;

    /* Set values on IO memory */
    /* VIC */
    mem_set_for_cpu(0xd000, 0x10);
    /* SID */
    mem_set_for_cpu(0xd400, 0x20);
    /* TODO: Color RAM */
    /* CIA1 */
    mem_set_for_cpu(0xdc00, 0x30);
    /* CIA2 */
    mem_set_for_cpu(0xdd00, 0x40);

    mem_mapped = _vic_reg_val == 0x10 &&
                 _sid_reg_val == 0x20 &&
                 _cia1_reg_val == 0x30 &&
                 _cia2_reg_val == 0x40;

    if (pla_mapped != mem_mapped) {
        printf("PLA and memory has different opinion on if IO "
               "is mapped or not!\n");
    }

    return pla_mapped;
}

static bool probe_char()
{
    bool pla_mapped = pla_is_char_mapped();
    bool mem_mapped = mem_get_for_cpu(0xd000) == CHARGEN_VAL;

    if (pla_mapped != mem_mapped) {
        printf("PLA and memory has different opinion on if char "
               "is mapped or not!\n");
    }
    return pla_mapped;
}

int test_map_after_reset()
{
    /* Basic, IO and kernal should be mapped, not char */
    if (!probe_basic()) {
        printf("Basic should be mapped\n");
        return 0;
    }
    if (!probe_kernal()) {
        printf("Kernal should be mapped\n");
        return 0;
    }
    if (!probe_io()) {
        printf("IO should be mapped\n");
        return 0;
    }
    if (probe_char()) {
        printf("Char should NOT be mapped\n");
        return 0;
    }
    return 1;
}


