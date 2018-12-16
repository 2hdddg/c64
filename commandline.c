#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "trace.h"
#include "commandline.h"

static int log_fd;

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

void commandline_loop(bool *exit, struct cpu_state *state)
{
    char   *line = NULL;
    size_t size;

    while (true) {
    printf(">>");
    fflush(NULL);

        getline(&line, &size, stdin);
        if (strcmp("exit\n", line) == 0) {
            *exit = true;
            return;
        }
        if (strcmp(line, "c64\n") == 0) {
            return;
        }
    }
}

int commandline_init()
{
    trace_init();
    log_fd = open("./log", O_CREAT|O_TRUNC|O_WRONLY);

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
