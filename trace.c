#include <string.h>
#include <malloc.h>

#include "trace.h"

struct trace_point_item {
    struct trace_point      point;
    struct trace_point_item *next;
};

struct trace_point_item *_first;

void trace_init()
{
    _first = NULL;
}

struct trace_point* trace_add_point(const char *sys,
                                    const char *name)
{
    struct trace_point_item *to_add = malloc(sizeof(*to_add));

    to_add->point.sys  = sys;
    to_add->point.name = name;
    to_add->point.fd   = -1;
    to_add->next       = NULL;

    if (_first == NULL) {
        _first = to_add;
    }
    else {
        to_add->next = _first;
        _first = to_add;
    }
    return &to_add->point;
}

bool trace_enable_point(const char *sys,
                        const char *name,
                        int fd)
{
    struct trace_point_item *item = _first;

    while (item) {
        if (strcmp(item->point.sys, sys) == 0 &&
            strcmp(item->point.name, name) == 0) {
            item->point.fd = fd;
            return true;
        }
        item = item->next;
    }
    return false;
}

