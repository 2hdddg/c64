#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>


#define TRACE(point, format, args...)           \
    if (point->fd != -1) {                      \
        char trace_buf[50];                     \
        int  len;                               \
        len = sprintf(trace_buf,                \
                      "%s %s: ",                \
                      point->sys, point->name); \
        write(point->fd, trace_buf, len);       \
        len = sprintf(trace_buf, format, args); \
        write(point->fd, trace_buf, len);       \
        write(point->fd, "\n", 1);              \
    }

#define TRACE0(point, text)                     \
    if (point->fd != -1) {                      \
        char trace_buf[50];                     \
        int  len;                               \
        len = sprintf(trace_buf,                \
                      "%s %s: ",                \
                      point->sys, point->name); \
        write(point->fd, trace_buf, len);       \
        write(point->fd, text, strlen(text));   \
        write(point->fd, "\n", 1);              \
    }

#define TRACE_NOT_IMPL(point, feature)              \
    if (point->fd != -1) {                          \
        char trace_buf[50];                         \
        int  len;                                   \
        len = sprintf(trace_buf,                    \
                      "%s %s: ",                    \
                      point->sys, point->name);     \
        write(point->fd, trace_buf, len);           \
        write(point->fd, feature, strlen(feature)); \
        write(point->fd, "\n", 1);                  \
    }


struct trace_point {
    const char *sys;
    const char *name;
    int        fd;
};

void trace_init();

struct trace_point* trace_add_point(const char *sys,
                                    const char *name);
bool trace_enable_point(const char *sys,
                        const char *name,
                        int fd);

