#include <unistd.h>
#include <stdio.h>

#include "mem.h"
#include "cpu.h"


int main(int argc, char **argv)
{
    struct cpu_state state = {
        .pc = 0x2000,
    };

    cpu_init(mem_get_for_cpu, mem_set_for_cpu,
             STDOUT_FILENO);
    printf("Powering on..\n");
    mem_reset();
    cpu_poweron();

    int num = 12;
    cpu_set_state(&state);
    while (num--) {
        cpu_step(&state);
    }

    cpu_disassembly_at(STDOUT_FILENO, 0xff48, 10);

    return 0;
}

