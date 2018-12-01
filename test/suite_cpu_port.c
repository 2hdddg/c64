#include <stdbool.h>
#include <stdio.h>

#include "cpu_port.h"
#include "mem.h"

uint8_t _val;
bool _loram_high;
bool _hiram_high;
bool _charen_high;

/* Mocked PLA implementation */
void pla_pins_from_cpu(bool loram_high,
                       bool hiram_high,
                       bool charen_high)
{
    _loram_high = loram_high;
    _hiram_high = hiram_high;
    _charen_high = charen_high;
}

int each_before()
{
    mem_init();
    mem_reset();
    _val = 0xff;
    _loram_high = false;
    _hiram_high = false;
    _charen_high = false;
    return 0;
}

static int assert_val(uint8_t actual, uint8_t expected)
{
    if (actual != expected) {
        printf("Expected %02x but was %02x\n",
               expected, actual);
        return 0;
    }
    return 1;
}


/* Will reach 0x2f after some cycles or by kernal ? */
#if 0
int test_direction_values()
{
    /* Value before initializing port should be zero */
    _val = mem_get_for_cpu(0x00);
    if (!assert_val(_val, 0x00)) {
        printf("Should be zero before port init\n");
        return 0;
    }

    cpu_port_init();
    _val = mem_get_for_cpu(0x00);
    return assert_val(_val, 0x2f);

    return 1;
}
#endif

int test_init_peripheral_values()
{
    /* Value before initializing port should be zero */
    _val = mem_get_for_cpu(0x01);
    if (!assert_val(_val, 0x00)) {
        printf("Should be zero before port init\n");
        return 0;
    }

    cpu_port_init();
    _val = mem_get_for_cpu(0x01);
    return assert_val(_val, 0x37);
}

int test_init_sets_mem_pins_on_pla()
{
    cpu_port_init();

    return _loram_high && _hiram_high && _charen_high;
}
