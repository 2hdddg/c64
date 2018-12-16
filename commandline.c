#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>

#include "mem.h"
#include "cpu.h"
#include "trace.h"
#include "commandline.h"

static int  _log_fd;
static bool _exit_loop;
static bool *_exit_app;

static uint16_t _ram_address = 0;
static uint16_t _dis_address = 0;

typedef void (*command_handler)();

struct command {
    char            *name;
    command_handler handler;
};

static void on_trace()
{
    char   *token = strtok(NULL, " ");
    struct trace_point *p;

    if (!token || strcmp(token, "list") == 0) {
        /* List all traces */
        printf("Enabled trace points:\n");
        p = trace_enum_points(NULL);
        while (p) {
            if (p->fd != -1) {
                printf("%8s %s\n", p->sys, p->name);
            }
            p = trace_enum_points(p);
        }
        printf("Disabled trace points:\n");
        p = trace_enum_points(NULL);
        while (p) {
            if (p->fd == -1) {
                printf("%8s %s\n", p->sys, p->name);
            }
            p = trace_enum_points(p);
        }
    }
    else if (strcmp(token, "on") == 0 ||
             strcmp(token, "off") == 0) {
        char *sys    = strtok(NULL, " ");
        char *name   = strtok(NULL, " ");
        bool turn_on = strcmp(token, "on") == 0;
        int  fd      = turn_on ? _log_fd : -1;

        p = trace_enum_points(NULL);
        while (p) {
            if (sys == NULL || strcmp(p->sys, sys) == 0) {
                if (name == NULL || strcmp(p->name, name) == 0) {
                    p->fd = fd;
                }
            }
            p = trace_enum_points(p);
        }
    }
    else {
        printf("Unknown trace parameter\n");
    }
}

static void on_ram()
{
    const uint16_t width = 16;
    uint8_t        *ram  = mem_get_ram(_ram_address);
    int            num   = width * 10;
    char          *token = strtok(NULL, " ");

    if (token) {
        char *out;
        int user_address = strtol(token, &out, 16);
        if (user_address == 0 && out == token) {
            printf("Illegal address: %s\n", token);
            return;
        }
        _ram_address = user_address & 0xfff0;
    }

    if (_ram_address + num > 0xffff) {
        num = 0xffff - _ram_address;
    }

    int lines = (num + (width - 1)) / width;

    while (lines--) {
        printf("%04x ", _ram_address);
        for (int i = 0; i < width; i++) {
            printf("%02x ", *ram);
            ram++;
        }

        _ram_address += width;
        printf("\n");
    }
}

static void on_dis()
{
    int  num    = 10;
    char *token = strtok(NULL, " ");

    if (token) {
        char *out;
        int user_address = strtol(token, &out, 16);
        if (user_address == 0 && out == token) {
            printf("Illegal address: %s\n", token);
            return;
        }
        _dis_address = user_address & 0xfff0;
    }

    cpu_disassembly_at(STDOUT_FILENO, _dis_address, num, &_dis_address);
}

static void on_c64()
{
    _exit_loop = true;
}

static void xon_exit()
{
    _exit_loop = true;
    *_exit_app = true;
}

/*
 * SYNTAX:
 *
 * trace all|none|list|on sys name|off sys name
 * ram address [count=32 [width=8]]
 * c64
 * exit
 * dis [address=pc [count=10]]
 *
 */
struct command _commands[] = {
    {
        .name    = "trace",
        .handler = on_trace,
    },
    {
        .name    = "exit",
        .handler = xon_exit,
    },
    {
        .name    = "c64",
        .handler = on_c64,
    },
    {
        .name    = "ram",
        .handler = on_ram,
    },
    {
        .name    = "dis",
        .handler = on_dis,
    },
};
int _num_commands = sizeof(_commands) / sizeof(_commands[0]);

void commandline_loop()
{
    char   *line = NULL;
    size_t size;
    size_t len;

    /* TODO: Remove all leftovers from ncurses */

    _exit_loop = false;
    while (!_exit_loop) {
        printf(">>");
        fflush(stdout);

        len = getline(&line, &size, stdin);
        if (len > 0) {
            line[len - 1] = 0;
            char *token = strtok(line, " ");
            char *name = token ? token : line;
            if (name) {
                for (int i = 0; i < _num_commands; i++) {
                    if (strcmp(_commands[i].name, name) == 0) {
                        _commands[i].handler();
                        break;
                    }
                }
            }
        }
    }
    free(line);
}

int commandline_init(bool *exit, struct cpu_state *state)
{
    _log_fd = open("./log", O_CREAT|O_TRUNC|O_WRONLY);
    _exit_app = exit;

    /* Defaults */
    trace_enable_point("VIC",  "set reg", -1);
    trace_enable_point("CIA1", "ERROR", -1);
    trace_enable_point("CIA1", "timer", -1);
    trace_enable_point("CIA2", "ERROR", -1);
    trace_enable_point("CIA2", "timer", -1);
    trace_enable_point("CIA2", "set port", -1);
    trace_enable_point("CIA2", "get port", -1);
    trace_enable_point("PLA",  "banks", _log_fd);
    trace_enable_point("KBD",  "set port", -1);
    trace_enable_point("KBD",  "get port", -1);
    trace_enable_point("KBD",  "key", -1);
    trace_enable_point("CIA1", "set port", -1);
    trace_enable_point("CIA1", "get port", -1);
    trace_enable_point("CIA1", "timer", -1);
    trace_enable_point("CPU",  "execution", -1);

    return 0;
}
