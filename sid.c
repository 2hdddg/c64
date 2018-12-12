#include <stdio.h>

#include "sid.h"
#include "trace.h"

static struct trace_point *_trace_error   = NULL;

void sid_init()
{
    _trace_error = trace_add_point("SID", "ERROR");
}

uint8_t sid_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    TRACE(_trace_error, "get reg %04x not handled", absolute);
    return 0;
}

void sid_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
    TRACE(_trace_error, "set reg %04x not handled", absolute);
}
