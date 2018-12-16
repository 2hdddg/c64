#pragma once

#include <stdbool.h>
#include "cpu.h"

int commandline_init();
void commandline_loop(bool *exit, struct cpu_state *state);
