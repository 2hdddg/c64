#include <unistd.h>
#include <stdio.h>

#include "mem.h"
#include "cpu.h"


int main(int argc, char **argv)
{
    struct mem_h *mem_h = NULL;
    struct cpu_h *cpu_h = NULL;

    if (mem_create(&mem_h) < 0) {
        return -1;
    }

    if (cpu_create(mem_h, STDOUT_FILENO, &cpu_h) < 0) {
        mem_destroy(mem_h);
        return -1;
    }

    printf("Powering on..\n");
    cpu_poweron(cpu_h);

    int num = 12;
    cpu_set_pc(cpu_h, 0x2000);
    while (num--) {
        cpu_step(cpu_h, NULL);
    }

    cpu_destroy(cpu_h);
    mem_destroy(mem_h);

    return 0;
}

