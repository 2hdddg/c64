#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "mem.h"
#include "cpu.h"

#define RAM_SIZE 65536
#define CODE 0x1000

char _ram[RAM_SIZE];

struct mem_h {
};

struct mem_h     *_mem;
struct cpu_h     *_cpu;
struct cpu_state _state;

struct test {
    char             *name;
    char             instructions[10];
    int              num_steps;
    struct cpu_state state;
    bool             check_reg_a;
    bool             check_flags;
};

/* Stubbed implementation of mem */

int mem_create(struct mem_h **mem_out)
{
    *mem_out = (struct mem_h*)_ram;
    return 0;
}

int mem_reset(struct mem_h *mem_h)
{
    return 0;
}

void mem_set(struct mem_h *mem, uint16_t addr, uint8_t val)
{
    _ram[addr] = val;
}

uint8_t mem_get(struct mem_h *mem, uint16_t addr)
{
    return _ram[addr];
}

void mem_destroy(struct mem_h *mem)
{
}


int each_before()
{
    mem_create(&_mem);
    memset(_ram, 0, RAM_SIZE);
    return cpu_create(_mem, STDOUT_FILENO, &_cpu) == 0;
}

int each_after()
{
    cpu_destroy(_cpu);
    mem_destroy(_mem);
    return 0;
}

int check_test(struct test *test, struct cpu_state *state)
{
    if (test->check_reg_a) {
        if (test->state.reg_a != state->reg_a) {
            printf("%s: failed. Expected reg_a:%02x but was %02x\n",
                   test->name, test->state.reg_a, state->reg_a);
            return 0;
        }
    }
    return 1;
}

int run_tests(struct test *tests, int num)
{
    int success = 1;

    for (int i=0; i<num; i++) {
        memcpy(_ram+CODE, tests[i].instructions, 10);
        cpu_set_pc(_cpu, CODE);
        for (int j=0; j < tests[i].num_steps; j++) {
            cpu_step(_cpu, &_state);
        }
        if (!check_test(&tests[i], &_state)) {
            success = 0;
        }
    }
    return success;
}

int test_load_instructions()
{
    struct test tests[] = {
        {
            .name = "Immediate LDA #$40",
            .instructions = { 0xa9, 0x40 },
            .num_steps = 1,
            .state = {
                .reg_a = 0x40,
                .flags = 0x00,
            },
            .check_reg_a = true,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

