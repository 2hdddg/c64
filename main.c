#include <unistd.h>
#include <stdio.h>

#include "mem.h"
#include "cpu.h"


int main(int argc, char **argv)
{
    struct mem_h *mem = NULL;
    struct cpu_h *cpu = NULL;
    struct cpu_state state = {
        .pc = 0x2000,
    };

    if (mem_create(&mem) < 0) {
        return -1;
    }

    if (cpu_create(STDOUT_FILENO,
                   mem_get_cpu, mem_set_cpu, mem,
                   &cpu) < 0) {
        mem_destroy(mem);
        return -1;
    }

    printf("Powering on..\n");
    cpu_poweron(cpu);
    mem_reset(mem);

    int num = 12;
    cpu_set_state(cpu, &state);
    while (num--) {
        cpu_step(cpu, &state);
    }

    cpu_disassembly_at(cpu, STDOUT_FILENO, 0xff48, 10);

    cpu_destroy(cpu);
    mem_destroy(mem);

    return 0;
}

