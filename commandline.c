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
#include "basic.h"
#include "vic.h"

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
    uint8_t        *ram  = NULL;
    int            num   = width * 10;
    char          *token = strtok(NULL, " ");
    int           lines  = 0;

    if (token) {
        char *out;
        int  user_address = strtol(token, &out, 16);
        if (user_address == 0 && out == token) {
            printf("Illegal address: %s\n", token);
            return;
        }
        _ram_address = user_address & 0xfff0;
    }

    if (_ram_address + num > 0xffff) {
        num = 0xffff - _ram_address;
    }

    lines = (num + (width - 1)) / width;
    ram =  mem_get_ram(_ram_address);
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

static void set_vic_reg(uint16_t reg, uint8_t val)
{
    vic_reg_set(val, 0xd000 + reg, 0, NULL);
}

static uint8_t get_vic_reg(uint16_t reg)
{
    return vic_reg_get(0xd000 + reg, 0, NULL);
}

static void on_vic()
{
    char *token = strtok(NULL, " ");

    if (!token || strcmp(token, "stat") == 0) {
        vic_stat(STDOUT_FILENO);
    }
    if (strcmp(token, "cols") == 0) {
        token = strtok(NULL, " ");
        if (!token || strcmp(token, "40") == 0) {
            uint8_t val = get_vic_reg(VIC_REG_SCROLX);
            val |= VIC_SCROLX_COL_40;
            set_vic_reg(VIC_REG_SCROLX, val);
        }
        else if (strcmp(token, "38") == 0) {
            uint8_t val = get_vic_reg(VIC_REG_SCROLX);
            val &= ~VIC_SCROLX_COL_40;
            set_vic_reg(VIC_REG_SCROLX, val);
        }
    }
    if (strcmp(token, "rows") == 0) {
        token = strtok(NULL, " ");
        if (strcmp(token, "25") == 0) {
            uint8_t val = get_vic_reg(VIC_REG_SCROLY);
            val |= VIC_SCROLY_ROW_25;
            set_vic_reg(VIC_REG_SCROLY, val);
        }
        else if (strcmp(token, "24") == 0) {
            uint8_t val = get_vic_reg(VIC_REG_SCROLY);
            val &= ~VIC_SCROLY_ROW_25;
            set_vic_reg(VIC_REG_SCROLY, val);
        }
    }
    if (strcmp(token, "display") == 0) {
        token = strtok(NULL, " ");
        if (!token || strcmp(token, "on") == 0) {
            uint8_t val = get_vic_reg(VIC_REG_SCROLY);
            val |= VIC_SCROLY_DISPLAY_EN;
            set_vic_reg(VIC_REG_SCROLY, val);
        }
        else if (strcmp(token, "off") == 0) {
            uint8_t val = get_vic_reg(VIC_REG_SCROLY);
            val &= ~VIC_SCROLY_DISPLAY_EN;
            set_vic_reg(VIC_REG_SCROLY, val);
        }
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

static void on_basic()
{
    basic_stat(STDOUT_FILENO);
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
    {
        .name    = "basic",
        .handler = on_basic,
    },
    {
        .name    = "vic",
        .handler = on_vic,
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
    trace_enable_point("CIA1", "ERROR", _log_fd);
    trace_enable_point("CIA2", "ERROR", _log_fd);
    trace_enable_point("CPU",  "ERROR", _log_fd);
    trace_enable_point("VIC",  "ERROR", _log_fd);
    trace_enable_point("VIC",  "bank", _log_fd);
    trace_enable_point("SID",  "ERROR", _log_fd);
    trace_enable_point("CIA2", "set port", _log_fd);

    return 0;
}
