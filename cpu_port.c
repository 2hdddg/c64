#include <stdbool.h>

#include "cpu_port.h"
#include "mem.h"
#include "pla.h"

/* At address 0x00 */
uint8_t _data_direction_reg;
/* At address 0x01 */
uint8_t _peripheral_reg;

/* I/O lines */
/* These lines are always 1 by external pull up resistors */
const uint8_t _line_loram  = CPU_PORT_LORAM;
const uint8_t _line_hiram  = CPU_PORT_HIRAM;
const uint8_t _line_charen = CPU_PORT_CHAREN;


static inline bool _is_direction_in(uint8_t line)
{
    return (_data_direction_reg & line) == 0;
}

static inline bool _is_peripheral_high(uint8_t line)
{
    return (_peripheral_reg & line) > 0;
}

static void _on_changed()
{
    /* When data direction is set to IN the corresponding value
     * in peripheral should come from peripheral I/O line. */
    if (_is_direction_in(CPU_PORT_LORAM)) {
        _peripheral_reg |= _line_loram;
    }
    if (_is_direction_in(CPU_PORT_HIRAM)) {
        _peripheral_reg |= _line_hiram;
    }
    if (_is_direction_in(CPU_PORT_CHAREN)) {
        _peripheral_reg |= _line_charen;
    }

    /* Forward to pins on PLA */
    pla_pins_from_cpu(_is_peripheral_high(CPU_PORT_LORAM),
                      _is_peripheral_high(CPU_PORT_LORAM),
                      _is_peripheral_high(CPU_PORT_LORAM));
}

static void _mem_set(uint8_t val, uint16_t absolute,
                     uint8_t relative, uint8_t *ram)
{
    switch (relative) {
        case 0:
            _data_direction_reg = val;
            _on_changed();
            break;
        case 1:
            _peripheral_reg = val;
            _on_changed();
            break;
        default:
            *ram = val;
            break;
    }
}

static uint8_t _mem_get(uint16_t absolute, uint8_t relative,
                        uint8_t *ram)
{
    switch (relative) {
        case 0:
            return _data_direction_reg;
        case 1:
            return _peripheral_reg;
        default:
            return *ram;
    }
}

void cpu_port_init()
{
    _data_direction_reg = 0x00;
    _peripheral_reg = 0x00;
    _on_changed();

    /* Install hooks for CPU reading/writing to 0x00 & 0x01. */
    struct mem_hook_install install = {
        .set_hook = _mem_set,
        .get_hook = _mem_get,
        .page_start = 0,
        .num_pages = 1,
    };
    mem_install_hooks_for_cpu(&install, 1);
}

