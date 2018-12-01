#include <unistd.h>
#include <stdio.h>

#include "mem.h"
#include "cpu.h"
#include "cpu_port.h"
#include "pla.h"


int main(int argc, char **argv)
{
    struct cpu_state state = {
        .pc = 0x2000,
    };

    mem_init();
    pla_init();
    cpu_port_init();
    cpu_init(mem_get_for_cpu, mem_set_for_cpu,
             STDOUT_FILENO);
    mem_reset();
    printf("Powering on..\n");
    cpu_poweron();

    int num = 12;
    cpu_set_state(&state);
    while (num--) {
        cpu_step(&state);
    }

    cpu_disassembly_at(STDOUT_FILENO, 0xff48, 10);

    return 0;
}

