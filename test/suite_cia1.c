#include <stdio.h>

#include "cia1.h"

/* Mocking CPU */
void cpu_interrupt_request()
{
}

int each_before()
{
    cia1_init();
    cia1_reset();

    return 0;
}


