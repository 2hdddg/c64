#pragma once

#include <stdbool.h>
#include "cpu.h"

int commandline_init(bool *exit, struct cpu_state *state);
void commandline_loop();
