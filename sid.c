#include <stdio.h>
#include "sid.h"

uint8_t sid_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    printf("sid_mem_get: %04x\n", absolute);
    return 0;
}

void sid_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
    printf("sid_mem_set: %04x:%02x\n", absolute, val);
}
