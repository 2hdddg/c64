#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "trace.h"
#include "vic.h"

static struct trace_point *_trace_set_reg = NULL;
static struct trace_point *_trace_get_reg = NULL;

void vic_init()
{
    _trace_set_reg = trace_add_point("VIC", "set reg");
    _trace_get_reg = trace_add_point("VIC", "get reg");
}

uint8_t vic_mem_get(uint16_t absolute, uint8_t relative,
                     uint8_t *ram)
{
    TRACE(_trace_get_reg, "%04x", absolute);
    return 0;
}

void vic_mem_set(uint8_t val, uint16_t absolute,
                  uint8_t relative, uint8_t *ram)
{
    TRACE(_trace_set_reg, "%04x to %02x", absolute, val);
}
