#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>

#include "trace.h"
#include "commandline.h"

static int  _log_fd;
static bool _exit_loop;
static bool *_exit_app;

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
    else if (strcmp(token, "all") == 0) {
        /* Turn on all traces */
        p = trace_enum_points(NULL);
        while (p) {
            trace_enable_point(p->sys, p->name, _log_fd);
            p = trace_enum_points(p);
        }
    }
    else if (strcmp(token, "none") == 0) {
        /* Turn off all traces */
        p = trace_enum_points(NULL);
        while (p) {
            trace_enable_point(p->sys, p->name, -1);
            p = trace_enum_points(p);
        }
    }
    else if (strcmp(token, "on") == 0 ||
             strcmp(token, "off") == 0) {
        char *sys  = strtok(NULL, " ");
        char *name = strtok(NULL, " ");

        if (sys == NULL || name == NULL) {
            printf("Needs sys and name of trace\n");
            return;
        }
    }
    else {
        printf("Unknown trace parameter\n");
    }
}

static void on_c64()
{
    _exit_loop = true;
}

static void on_exit()
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
        .handler = on_exit,
    },
    {
        .name    = "c64",
        .handler = on_c64,
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
    trace_init();
    _log_fd = open("./log", O_CREAT|O_TRUNC|O_WRONLY);
    _exit_app = exit;

/*
    trace_enable_point("VIC", "set reg", the_log);
    trace_enable_point("CIA1", "ERROR", the_log);
    trace_enable_point("CIA1", "timer", the_log);
    trace_enable_point("CIA2", "ERROR", the_log);
    trace_enable_point("CIA2", "timer", the_log);
    trace_enable_point("CIA2", "set port", the_log);
    trace_enable_point("CIA2", "get port", the_log);
    trace_enable_point("PLA", "banks", the_log);
    trace_enable_point("KBD", "set port", the_log);
    trace_enable_point("KBD", "get port", the_log);
    trace_enable_point("KBD", "key", the_log);
    trace_enable_point("CIA1", "set port", the_log);
    trace_enable_point("CIA1", "get port", the_log);
    trace_enable_point("CIA1", "timer", the_log);
    trace_enable_point("CPU", "execution", the_log);
*/

    return 0;
}
