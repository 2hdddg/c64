#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "command.h"

static bool parse_uint16(char *token, uint16_t *num)
{
    int  parsed;
    char *out;

    if (!token) {
        return false;
    }

    parsed = strtol(token, &out, 16);
    if (parsed == 0 && out == token) {
        return false;
    }

    if (parsed < 0 || parsed > 0xffff) {
        return false;
    }

    *num = (uint16_t)parsed;
    return true;
}

bool cmd_parse_uint16(char *token, uint16_t *num)
{
    if (!parse_uint16(token, num)) {
        printf("Invalid or missing 16 bit unsigned number\n");
        return false;
    }
    return true;
}

bool cmd_parse_address(char *token, uint16_t *address)
{
    if (!parse_uint16(token, address)) {
        printf("Invalid or missing address\n");
        return false;
    }
    return true;
}

bool cmd_parse_uint8(char *token, uint8_t *num)
{
    uint16_t big_num;

    if (!parse_uint16(token, &big_num) ||
        big_num > 0xff) {
        printf("Invalid or missing 8 bit unsigned number\n");
        return false;
    }
    *num = (uint8_t)big_num;
    return true;
}

uint16_t cmd_dump_mem(uint8_t *addr, uint16_t vaddr, int lines, int width)
{
    while (lines--) {
        printf("%04x ", vaddr);
        for (int i = 0; i < width; i++) {
            printf("%02x ", *addr);
            addr++;
        }

        vaddr += width;
        printf("\n");
    }
    return vaddr;
}
