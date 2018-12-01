#include "pla.h"

/* Input pins, true indicates high */
bool _pin_loram;
bool _pin_hiram;
bool _pin_charen;

void pla_init()
{
    _pin_loram = false;
    _pin_hiram = false;
    _pin_charen = false;
}

void pla_pins_from_cpu(bool loram_high,
                       bool hiram_high,
                       bool charen_high)
{

}
