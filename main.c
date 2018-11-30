#include <unistd.h>
#include <stdio.h>

#include "mem.h"
#include "cpu.h"


int main(int argc, char **argv)
{
    struct cpu_h *cpu = NULL;
    struct cpu_state state = {
        .pc = 0x2000,
    };

    if (cpu_create(STDOUT_FILENO,
                   mem_get_for_cpu,
                   mem_set_for_cpu,
                   &cpu) < 0) {
        return -1;
    }

    printf("Powering on..\n");
    mem_reset();
    cpu_poweron(cpu);

    int num = 12;
    cpu_set_state(cpu, &state);
    while (num--) {
        cpu_step(cpu, &state);
    }

    cpu_disassembly_at(cpu, STDOUT_FILENO, 0xff48, 10);

    cpu_destroy(cpu);

    return 0;
}

