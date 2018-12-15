#include <stdio.h>

#include "cia2.h"
#include "trace.h"

struct trace_point *_trace_error;

void cia2_init()
{
    /* Debugging */
    _trace_error = trace_add_point("CIA2", "ERROR");
}

uint8_t cia2_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    TRACE(_trace_error, "mem get %04x not handled", absolute);
    return 0;
}

void cia2_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
    TRACE(_trace_error, "mem set %04x:%02x not handled",
          absolute, val);
}

