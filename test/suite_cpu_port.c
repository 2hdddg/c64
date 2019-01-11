#include <stdbool.h>
#include <stdio.h>

#include "cpu_port.h"
#include "mem.h"

uint8_t _val;

bool _loram;
bool _hiram;
bool _charen;

/* Mocked PLA implementation */
void pla_pins_from_cpu(bool loram,
                       bool hiram,
                       bool charen)
{
    _loram  = loram;
    _hiram  = hiram;
    _charen = charen;
}

int each_before()
{
    mem_init();
    mem_reset();
    _val = 0xff;
    _loram= false;
    _hiram= false;
    _charen= false;
    cpu_port_init();
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

int test_init_peripheral_values()
{
    _val = mem_get_for_cpu(0x01);
    return assert_val(_val, 0x37);
}

/* Verifies that initial values will set all pins to PLA
 * to high. */
int test_init_sets_mem_pins_on_pla()
{
    /* Call to cpu_port_init is done in setup */
    return _loram && _hiram && _charen;
}

/* Verifies that when direction is set to out and values
 * are written from CPU those values are sent to PLA. */
int test_direction_out_sets_custom_value_on_pla()
{
    uint8_t direction = CPU_PORT_LORAM | CPU_PORT_HIRAM |
                        CPU_PORT_CHAREN;

    /* At this point direction is still IN so this shouldn't
     * affect PLA. */
    mem_set_for_cpu(0x0001, 0x00);
    if (!_loram || !_hiram || !_charen) {
        printf("Expected pins to stay high\n");
        return 0;
    }

    mem_set_for_cpu(0x0000, direction);
    if (!(_loram && _hiram && _charen)) {
        printf("Expected pins to go low\n");
        return 0;
    }

    return 1;
}
