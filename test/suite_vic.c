#include <string.h>
#include <stdint.h>

uint8_t _ram[0xffff];
uint8_t _color_ram[1000];



int each_before()
{
    memset(_ram, 0, 0xffff);
    memset(_color_ram, 0, 1000);
    return 0;
}

