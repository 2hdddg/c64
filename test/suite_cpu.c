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
    bool             check_reg_x;
    bool             check_reg_y;
    bool             check_pc;
    bool             check_sp;
    bool             check_flags;
    uint16_t         check_ram_at;
    uint8_t          check_ram_val;
    uint8_t          init_flags;
    uint8_t          init_reg_a;
    uint8_t          init_reg_x;
    uint8_t          init_reg_y;
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
    /* And another vector pointing to scrap area at 0x5000 */
    _ram[0x0014] = 0x00;
    _ram[0x0015] = 0x50;

    /* Known value on the stack */
    _ram[0x0110] = 0x80;
    _ram[0x0180] = FLAG_DECIMAL_MODE|FLAG_BRK;

    /* Use 0x5000 for whatever */

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
    if (test->check_reg_x) {
        if (test->state.reg_x != state->reg_x) {
            printf("%s: failed. Expected reg_x:%02x but was %02x\n",
                   test->name, test->state.reg_x, state->reg_x);
            return 0;
        }
    }
    if (test->check_reg_y) {
        if (test->state.reg_y != state->reg_y) {
            printf("%s: failed. Expected reg_y:%02x but was %02x\n",
                   test->name, test->state.reg_y, state->reg_y);
            return 0;
        }
    }
    if (test->check_sp) {
        if (test->state.sp != state->sp) {
            printf("%s: failed. Expected sp:%02x but was %02x\n",
                   test->name, test->state.sp, state->sp);
            return 0;
        }
    }
    if (test->check_pc) {
        if (test->state.pc != state->pc) {
            printf("%s: failed. Expected pc:%02x but was %02x\n",
                   test->name, test->state.pc, state->pc);
            return 0;
        }
    }
    if (test->check_flags) {
        if (test->state.flags != state->flags) {
            printf("%s: failed. Expected flags:%02x but was %02x\n",
                   test->name, test->state.flags, state->flags);
            return 0;
        }
    }
    if (test->check_ram_at) {
        if (test->check_ram_val != _ram[test->check_ram_at]) {
            printf("%s: failed. Expected ram at %04x to be %02x "
                   "but was %02x",
                   test->name, test->check_ram_at, test->check_ram_val,
                   _ram[test->check_ram_at]);
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
        _state.pc = CODE;
        _state.reg_a = tests[i].init_reg_a;;
        _state.reg_x = tests[i].init_reg_x;
        _state.reg_y = tests[i].init_reg_y;
        _state.flags = tests[i].init_flags;
        _state.sp = 0xff;
        cpu_set_state(_cpu, &_state);
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
            .instructions = { 0xb5, 0x01 },
            .num_steps = 1,
            .state = {
                .reg_a = 0x50,
            },
            .init_reg_x = 0x03,
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
            .instructions = { 0xbd, 0x00, 0x40 },
            .num_steps = 1,
            .state = {
                .reg_a = 0x03,
            },
            .init_reg_x = 0x03,
            .check_reg_a = true,
        },
        {
            .name = "Absolute,Y LDA",
            .instructions = { 0xb9, 0x00, 0x40 },
            .num_steps = 1,
            .state = {
                .reg_a = 0x04,
            },
            .init_reg_y = 0x04,
            .check_reg_a = true,
        },
        {
            .name = "Indirect, X LDA",
            .instructions = { 0xa1, 0x10 },
            .num_steps = 1,
            .state = {
                .reg_a = 0x02,
            },
            .check_reg_a = true,
            .init_reg_x = 0x02,
        },
        {
            .name = "Indirect, Y LDA",
            .instructions = { 0xb1, 0x10 },
            .num_steps = 1,
            .state = {
                .reg_a = 0x03,
            },
            .check_reg_a = true,
            .init_reg_y = 0x03,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

int test_load_x_instructions()
{
    struct test tests[] = {
        {
            .name = "Immediate LDX",
            .instructions = { 0xa2, 0x77 },
            .num_steps = 1,
            .state = {
                .reg_x = 0x77,
            },
            .check_reg_x = true,
        },
        {
            .name = "Zero page LDX",
            .instructions = { 0xa6, 0x02 },
            .num_steps = 1,
            .state = {
                .reg_x = 0x30,
            },
            .check_reg_x = true,
        },
        {
            .name = "Zero page, Y LDX",
            .instructions = { 0xb6, 0x01 },
            .num_steps = 1,
            .state = {
                .reg_x = 0x50,
            },
            .check_reg_x = true,
            .init_reg_y = 0x03,
        },
        {
            .name = "Absolute LDX",
            .instructions = { 0xae, 0x01, 0x40 },
            .num_steps = 1,
            .state = {
                .reg_x = 0x01,
            },
            .check_reg_x = true,
        },
        {
            .name = "Absolute,Y LDX",
            .instructions = { 0xbe, 0x00, 0x40 },
            .num_steps = 1,
            .state = {
                .reg_x = 0x03,
            },
            .check_reg_x = true,
            .init_reg_y = 0x03,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

int test_load_y_instructions()
{
    struct test tests[] = {
        {
            .name = "Immediate LDY",
            .instructions = { 0xa0, 0x77 },
            .num_steps = 1,
            .state = {
                .reg_y = 0x77,
            },
            .check_reg_y = true,
        },
        {
            .name = "Zero page LDY",
            .instructions = { 0xa4, 0x02 },
            .num_steps = 1,
            .state = {
                .reg_y = 0x30,
            },
            .check_reg_y = true,
        },
        {
            .name = "Zero page, X LDY",
            .instructions = { 0xb4, 0x01 },
            .num_steps = 1,
            .state = {
                .reg_y = 0x50,
            },
            .check_reg_y = true,
            .init_reg_x = 0x03,
        },
        {
            .name = "Absolute LDY",
            .instructions = { 0xac, 0x01, 0x40 },
            .num_steps = 1,
            .state = {
                .reg_y = 0x01,
            },
            .check_reg_y = true,
        },
        {
            .name = "Absolute,X LDY",
            .instructions = { 0xbc, 0x00, 0x40 },
            .num_steps = 1,
            .state = {
                .reg_y = 0x03,
            },
            .check_reg_y = true,
            .init_reg_x = 0x03,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

int test_store_a_instructions()
{
    struct test tests[] = {
        {
            .name = "Zero page",
            .instructions = { 0x85, 0x20, },
            .num_steps = 1,
            .check_ram_at = 0x20,
            .check_ram_val = 0x01,
            .init_reg_a = 0x01,
        },
        {
            .name = "Zero page, X",
            .instructions = { 0xa2, 0x01, 0x95, 0x20, },
            .num_steps = 2,
            .check_ram_at = 0x21,
            .check_ram_val = 0x02,
            .init_reg_a = 0x02,
        },
        {
            .name = "Absolute",
            .instructions = { 0x8d, 0x00, 0x50, },
            .num_steps = 1,
            .check_ram_at = 0x5000,
            .check_ram_val = 0x01,
            .init_reg_a = 0x01,
        },
        {
            .name = "Absolute, X",
            .instructions = { 0x9d, 0x00, 0x50 },
            .num_steps = 2,
            .check_ram_at = 0x5001,
            .check_ram_val = 0x02,
            .init_reg_a = 0x02,
            .init_reg_x = 0x01,
        },
        {
            .name = "Absolute, Y",
            .instructions = { 0x99, 0x00, 0x50 },
            .num_steps = 1,
            .check_ram_at = 0x5002,
            .check_ram_val = 0x03,
            .init_reg_a = 0x03,
            .init_reg_y = 0x02,
        },
        {
            .name = "Indirect, X",
            .instructions = { 0x81, 0x50 },
            .num_steps = 1,
            .check_ram_at = 0x54,
            .check_ram_val = 0x03,
            .init_reg_a = 0x03,
            .init_reg_x = 0x04,
        },
        {
            .name = "Indirect, Y",
            .instructions = { 0x91, 0x14 },
            .num_steps = 3,
            .check_ram_at = 0x5004,
            .check_ram_val = 0x03,
            .init_reg_a = 0x03,
            .init_reg_y = 0x04,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

int test_stack_instructions()
{
    struct test tests[] = {
        {
            .name = "TXS, transfer X to SP",
            .instructions = { 0x9a },
            .num_steps = 1,
            .check_sp = true,
            .check_flags = true,
            .state = {
                .sp = 0x10,
                .flags = FLAG_ZERO,
            },
            .init_flags = FLAG_ZERO,
            .init_reg_x = 0x10,
        },
        {
            .name = "TSX, transfer SP to X",
            .instructions = { 0xba, },
            .num_steps = 1,
            .check_flags = true,
            .check_reg_x = true,
            /* Relies on stack default to 0xff */
            .state = {
                .reg_x = 0xff,
                .flags = FLAG_NEGATIVE,
            },
            .init_flags = FLAG_ZERO,
        },
        {
            .name = "PHA, push accumulator",
            .instructions = { 0x48 },
            .num_steps = 1,
            .check_sp = true,
            .check_flags = true,
            .state = {
                .sp = 0xfe,
                .flags = FLAG_ZERO,
            },
            .init_flags = FLAG_ZERO,
            .init_reg_a = 0x10,
            .check_ram_at = 0x01ff,
            .check_ram_val = 0x10,
        },
        {
            .name = "PLA, pull accumulator",
            /* Transfer X to SP first */
            .instructions = { 0x9a, 0x68 },
            .num_steps = 2,
            .check_sp = true,
            .check_flags = true,
            .check_reg_a = true,
            .state = {
                .sp = 0x10,
                .reg_a = 0x80,
                .flags = FLAG_NEGATIVE,
            },
            .init_flags = FLAG_ZERO,
            .init_reg_x = 0x0f,
        },
        {
            .name = "PHP, push status",
            .instructions = { 0x08 },
            .num_steps = 1,
            .check_sp = true,
            .check_flags = true,
            .state = {
                .sp = 0xfe,
                .flags = FLAG_ZERO|FLAG_IRQ_DISABLE,
            },
            .init_flags = FLAG_ZERO|FLAG_IRQ_DISABLE,
            .check_ram_at = 0x01ff,
            .check_ram_val = FLAG_ZERO|FLAG_IRQ_DISABLE,
        },
        {
            /* Transfer X to SP first */
            .name = "PLP, pull status",
            .instructions = { 0x9a, 0x28 },
            .num_steps = 2,
            .check_sp = true,
            .check_flags = true,
            .state = {
                .sp = 0x80,
                .flags = FLAG_DECIMAL_MODE|FLAG_BRK,
            },
            .init_reg_x = 0x7f,
            .init_flags = 0,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

int test_branch_instructions()
{
    struct test tests[] = {
        {
            .name = "BEQ, branch on equal, jump",
            .instructions = { 0xf0, 0x06 },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x08,
            },
            .init_flags = FLAG_ZERO,
        },
        {
            .name = "BEQ, branch on equal, no jump",
            .instructions = { 0xf0, 0x06 },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x02,
            },
            .init_flags = 0,
        },
        {
            .name = "BNE, branch on not equal, jump",
            .instructions = { 0xd0, 0xfa },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE - 0x04,
            },
            .init_flags = 0,
        },
        {
            .name = "BNE, branch on not equal, no jump",
            .instructions = { 0xd0, 0xfa },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x02,
            },
            .init_flags = FLAG_ZERO,
        },
        {
            .name = "BPL, branch on plus, jump",
            .instructions = { 0x10, 0x06 },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x08,
            },
            .init_flags = 0,
        },
        {
            .name = "BPL, branch on plus, no jump",
            .instructions = { 0x10, 0x06 },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x02,
            },
            .init_flags = FLAG_NEGATIVE,
        },
        {
            .name = "BMI, branch on minus, jump",
            .instructions = { 0x30, 0x06 },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x08,
            },
            .init_flags = FLAG_NEGATIVE,
        },
        {
            .name = "BMI, branch on minus, no jump",
            .instructions = { 0x30, 0x06 },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x02,
            },
            .init_flags = 0,
        },
        {
            .name = "BVC, branch on overflow clear, jump",
            .instructions = { 0x50, 0x20 },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x22,
            },
            .init_flags = 0,
        },
        {
            .name = "BVC, branch on overflow clear, no jump",
            .instructions = { 0x50, 0x20 },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x02,
            },
            .init_flags = FLAG_OVERFLOW,
        },
        {
            .name = "BVS, branch on overflow set, jump",
            .instructions = { 0x70, 0x20 },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x22,
            },
            .init_flags = FLAG_OVERFLOW,
        },
        {
            .name = "BVS, branch on overflow set, no jump",
            .instructions = { 0x70, 0x20 },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x02,
            },
            .init_flags = FLAG_NEGATIVE,
        },
        {
            .name = "BCC, branch on carry clear, jump",
            .instructions = { 0x90, 0x20 },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x22,
            },
            .init_flags = FLAG_OVERFLOW,
        },
        {
            .name = "BCC, branch on carry clear, no jump",
            .instructions = { 0x90, 0x20 },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x02,
            },
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "BCS, branch on carry set, jump",
            .instructions = { 0xb0, 0x20 },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x22,
            },
            .init_flags = FLAG_CARRY|FLAG_OVERFLOW,
        },
        {
            .name = "BCS, branch on carry set, no jump",
            .instructions = { 0xb0, 0x20 },
            .num_steps = 1,
            .check_pc = true,
            .state = {
                .pc = CODE + 0x02,
            },
            .init_flags = 0,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
