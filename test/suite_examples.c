#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "mem.h"
#include "cpu.h"

#define MAX_SIZE (10000)
uint8_t _buffer[MAX_SIZE];

struct mem_h     *_mem;
struct cpu_h     *_cpu;
struct cpu_state _state;

int each_before()
{
    return mem_create(&_mem) == 0 &&
           cpu_create(_mem, STDOUT_FILENO, &_cpu) == 0;
}


int test_add_with_carry_decimals()
{
    FILE *f = fopen("../test/dadc", "rb");
    if (f == NULL) {
        printf("Unable to open file with testcode\n");
        return -1;
    }
    int size = fread(_buffer, 1, MAX_SIZE, f);
    printf("Size is %d\n", size);


    for (int i = 0; i < size; i++) {
        mem_set(_mem, 0x4000 + i, _buffer[i]);
    }

    _state.pc = 0x4000;
    cpu_set_state(_cpu, &_state);
    for (int i = 0; i < 50; i++) {
        cpu_step(_cpu, &_state);
    }

    return 1;
}
