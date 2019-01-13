#include "kernal.h"

static const uint16_t _address = 0xe000;

uint16_t kernal_address()
{
    return _address;
}

