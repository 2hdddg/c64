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

    /* Known values at 0x4000 */
    _ram[0x4000] = 0;
    _ram[0x4001] = 1;
    _ram[0x4002] = 2;
    _ram[0x4003] = 3;
    _ram[0x4004] = 4;
    _ram[0x4005] = 5;
    /* Known values at 0x0000 */
    _ram[0x0000] = 0x10;
    _ram[0x0001] = 0x20;
    _ram[0x0002] = 0x30;
    _ram[0x0003] = 0x40;
    _ram[0x0004] = 0x50;
    _ram[0x0005] = 0x60;
    /* Zero page vector pointing to known values at 0x4000 */
    _ram[0x0010] = 0x00;
    _ram[0x0011] = 0x40;
    /* And another vector pointing to known values at 0x4002 */
    _ram[0x0012] = 0x02;
    _ram[0x0013] = 0x40;



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
        printf("Testing %s\n", tests[i].name);
        fflush(stdout);
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

int test_load_a_instructions()
{
    struct test tests[] = {
        {
            .name = "Immediate LDA",
            .instructions = { 0xa9, 0x40 },
            .num_steps = 1,
            .state = {
                .reg_a = 0x40,
            },
            .check_reg_a = true,
        },
        {
            .name = "Zero page LDA",
            .instructions = { 0xa5, 0x02 },
            .num_steps = 1,
            .state = {
                .reg_a = 0x30,
            },
            .check_reg_a = true,
        },
        {
            .name = "Zero page, X LDA",
            .instructions = { 0xa2, 0x03, 0xb5, 0x01 },
            .num_steps = 2,
            .state = {
                .reg_a = 0x50,
            },
            .check_reg_a = true,
        },
        {
            .name = "Absolute LDA",
            .instructions = { 0xad, 0x01, 0x40 },
            .num_steps = 1,
            .state = {
                .reg_a = 0x01,
            },
            .check_reg_a = true,
        },
        {
            .name = "Absolute,X LDA",
            .instructions = { 0xa2, 0x03, 0xbd, 0x00, 0x40 },
            .num_steps = 2,
            .state = {
                .reg_a = 0x03,
            },
            .check_reg_a = true,
        },
        {
            .name = "Absolute,Y LDA",
            .instructions = { 0xa0, 0x04, 0xb9, 0x00, 0x40 },
            .num_steps = 2,
            .state = {
                .reg_a = 0x04,
            },
            .check_reg_a = true,
        },
        {
            .name = "Indirect, X LDA",
            .instructions = { 0xa2, 0x02, 0xa1, 0x10 },
            .num_steps = 2,
            .state = {
                .reg_a = 0x02,
            },
            .check_reg_a = true,
        },
        {
            .name = "Indirect, Y LDA",
            .instructions = { 0xa0, 0x03, 0xb1, 0x10 },
            .num_steps = 2,
            .state = {
                .reg_a = 0x03,
            },
            .check_reg_a = true,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

