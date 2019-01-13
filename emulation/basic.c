#include <stdio.h>
#include <string.h>

#include "basic.h"
#include "pla.h"
#include "mem.h"

static const uint16_t _address = 0xa000;


uint16_t get_address(uint16_t addr)
{
    uint8_t lo = mem_get_for_cpu(addr);
    uint8_t hi = mem_get_for_cpu(addr + 1);

    return hi << 8 | lo;
}

void basic_stat()
{
    if (!pla_is_basic_mapped()) {
        printf("BASIC not mapped in memory\n");
        return;
    }
    printf("BASIC address %04x\n", _address);
    printf("BASIC variables\n");
    /* Program location */
    printf("   TXTTAB %04x\n", get_address(0x2b));
    /* Commandline buffer */
    printf("      BUF %04x\n", 0x0200);

/*
    Line buffer
    0x0200 - 0x258 BUF
*/
}

uint16_t basic_address()
{
    return _address;
}

