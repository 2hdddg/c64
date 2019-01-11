#include "pla.h"


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

int each_before()
{
    return 0;
}

