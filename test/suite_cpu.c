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

static void set_known_mem_values()
{
    /* Known values at 0x4000 */
    _ram[0x4000] = 0;
    _ram[0x4001] = 1;
    _ram[0x4002] = 2;
    _ram[0x4003] = 3;
    _ram[0x4004] = 4;
    _ram[0x4005] = 5;
    _ram[0x4006] = 0xff;
    _ram[0x4007] = 0x81;
    /* Known values at 0x0000 */
    _ram[0x0000] = 0x10;
    _ram[0x0001] = 0x20;
    _ram[0x0002] = 0x30;
    _ram[0x0003] = 0x40;
    _ram[0x0004] = 0x50;
    _ram[0x0005] = 0x60;
    _ram[0x0006] = 0x80; /* MSB, bit 7 */
    _ram[0x0007] = 0x40; /* Bit 6 */
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
}

int each_before()
{
    mem_create(&_mem);
    memset(_ram, 0, RAM_SIZE);
    memset(&_state, 0, sizeof(_state));

    set_known_mem_values();

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
        if (test->check_ram_val != (uint8_t)_ram[test->check_ram_at]) {
            printf("%s: failed. Expected ram at %04x to be %02x "
                   "but was %02x\n",
                   test->name, test->check_ram_at,
                   (uint8_t)test->check_ram_val,
                   (uint8_t)_ram[test->check_ram_at]);
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
        set_known_mem_values();
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

/* LDA */
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

/* LDX */
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

/* LDY */
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

/* STA */
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
            .instructions = { 0x95, 0x20, },
            .num_steps = 1,
            .check_ram_at = 0x21,
            .check_ram_val = 0x02,
            .init_reg_a = 0x02,
            .init_reg_x = 0x01,
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
            .num_steps = 1,
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
            .num_steps = 1,
            .check_ram_at = 0x5004,
            .check_ram_val = 0x03,
            .init_reg_a = 0x03,
            .init_reg_y = 0x04,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* STX */
int test_store_x_instructions()
{
    struct test tests[] = {
        {
            .name = "Zero page",
            .instructions = { 0x86, 0x20, },
            .num_steps = 1,
            .check_ram_at = 0x20,
            .check_ram_val = 0x01,
            .init_reg_x = 0x01,
        },
        {
            .name = "Zero page, Y",
            .instructions = { 0x96, 0x20, },
            .num_steps = 1,
            .check_ram_at = 0x21,
            .check_ram_val = 0x02,
            .init_reg_x = 0x02,
            .init_reg_y = 0x01,
        },
        {
            .name = "Absolute",
            .instructions = { 0x8e, 0x00, 0x50, },
            .num_steps = 1,
            .check_ram_at = 0x5000,
            .check_ram_val = 0x01,
            .init_reg_x = 0x01,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* STY */
int test_store_y_instructions()
{
    struct test tests[] = {
        {
            .name = "Zero page",
            .instructions = { 0x84, 0x20, },
            .num_steps = 1,
            .check_ram_at = 0x20,
            .check_ram_val = 0x01,
            .init_reg_y = 0x01,
        },
        {
            .name = "Zero page, X",
            .instructions = { 0x94, 0x20, },
            .num_steps = 1,
            .check_ram_at = 0x21,
            .check_ram_val = 0x02,
            .init_reg_x = 0x01,
            .init_reg_y = 0x02,
        },
        {
            .name = "Absolute",
            .instructions = { 0x8c, 0x00, 0x50, },
            .num_steps = 1,
            .check_ram_at = 0x5000,
            .check_ram_val = 0x01,
            .init_reg_y = 0x01,
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

/* AND */
int test_and_instructions()
{
    struct test tests[] = {
        {
            .name = "Immediate",
            .instructions = { 0x29, 0x80 },
            .num_steps = 1,
            .check_flags = true,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x80,
                .flags = FLAG_NEGATIVE,
            },
            .init_reg_a = 0xff,
            .init_flags = 0x00,
        },
        {
            .name = "Zero page",
            .instructions = { 0x25, 0x01, },
            .num_steps = 1,
            .check_flags = true,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x20,
                .flags = 0x00,
            },
            .init_reg_a = 0xff,
            .init_flags = 0x00,
        },
        {
            .name = "Zero page, X",
            .instructions = { 0x35, 0x00, },
            .num_steps = 1,
            .check_flags = true,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x20,
                .flags = 0x00,
            },
            .init_reg_x = 0x01,
            .init_reg_a = 0xff,
            .init_flags = 0x00,
        },
        {
            .name = "Absolute",
            .instructions = { 0x2d, 0x00, 0x40, },
            .num_steps = 1,
            .check_flags = true,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x00,
                .flags = FLAG_ZERO,
            },
            .init_reg_a = 0xff,
            .init_flags = 0x00,
        },
        {
            .name = "Absolute, X",
            .instructions = { 0x3d, 0x00, 0x40 },
            .num_steps = 1,
            .check_flags = true,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x01,
                .flags = 0,
            },
            .init_reg_x = 0x01,
            .init_reg_a = 0xff,
            .init_flags = 0x00,
        },
        {
            .name = "Absolute, Y",
            .instructions = { 0x39, 0x00, 0x40 },
            .num_steps = 1,
            .check_flags = true,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x02,
                .flags = 0x00,
            },
            .init_reg_a = 0xff,
            .init_reg_y = 0x02,
            .init_flags = 0x00,
        },
        {
            .name = "Indirect, X",
            .instructions = { 0x21, 0x10 },
            .num_steps = 1,
            .check_flags = true,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x02,
                .flags = 0x00,
            },
            .init_reg_a = 0x03,
            .init_reg_x = 0x02,
            .init_flags = 0x00,
        },
        {
            .name = "Indirect, Y",
            .instructions = { 0x31, 0x10 },
            .num_steps = 1,
            .check_flags = true,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x03,
                .flags = 0x00,
            },
            .init_reg_a = 0xff,
            .init_flags = 0x00,
            .init_reg_y = 0x03,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* ORA */
int test_or_instrucion()
{
    return 0;
}

/* ADC */
int test_add_with_carry_instructions()
{
    struct test tests[] = {
        /* Tests all different supported addressing modes */
        {
            .name = "Immediate",
            .instructions = { 0x69, 0x10 },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x11,
            },
            .init_reg_a = 0x01,
        },
        {
            .name = "Zero page",
            .instructions = { 0x65, 0x01, },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x21,
            },
            .init_reg_a = 0x01,
        },
        {
            .name = "Zero page, X",
            .instructions = { 0x75, 0x00, },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x22,
            },
            .init_reg_x = 0x01,
            .init_reg_a = 0x02,
        },
        {
            .name = "Absolute",
            .instructions = { 0x6d, 0x00, 0x40, },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x05,
            },
            .init_reg_a = 0x05,
        },
        {
            .name = "Absolute, X",
            .instructions = { 0x7d, 0x00, 0x40 },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x04,
            },
            .init_reg_x = 0x01,
            .init_reg_a = 0x03,
        },
        {
            .name = "Absolute, Y",
            .instructions = { 0x79, 0x00, 0x40 },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x62,
            },
            .init_reg_a = 0x60,
            .init_reg_y = 0x02,
        },
        {
            .name = "Indirect, X",
            .instructions = { 0x61, 0x10 },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x05,
            },
            .init_reg_a = 0x03,
            .init_reg_x = 0x02,
        },
        {
            .name = "Indirect, Y",
            .instructions = { 0x71, 0x10 },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x0a,
            },
            .init_reg_a = 0x07,
            .init_reg_y = 0x03,
        },

        /* All tests below assumes implementation of add is same
         * regardless of address mode. */

        /* Test adds with carry. */
        {
            .name = "Carry is set",
            .instructions = { 0x69, 0b11110000 },
            .num_steps = 1,
            .check_reg_a = true,
            .check_flags = true,
            .state = {
                .reg_a = 0x00,
                .flags = FLAG_CARRY|FLAG_ZERO,
            },
            .init_reg_a = 0b00010000,
        },
        {
            .name = "Carry is used",
            .instructions = { 0x69, 0x00 },
            .num_steps = 1,
            .check_reg_a = true,
            .check_flags = true,
            .state = {
                .reg_a = 0x01,
                .flags = 0x00,
            },
            .init_reg_a = 0x00,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Overflow is set",
            .instructions = { 0x69, 0b01000000 },
            .num_steps = 1,
            .check_reg_a = true,
            .check_flags = true,
            .state = {
                .reg_a = 0x80,
                .flags = FLAG_OVERFLOW|FLAG_NEGATIVE,
            },
            .init_reg_a = 0b01000000,
        },

        /* Test adds with FLAG_DECIMAL_MODE set. */
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* SBC */
int test_subract_with_carry_instructions()
{
    struct test tests[] = {
        /* Tests all different supported addressing modes */
        {
            .name = "Immediate",
            .instructions = { 0xe9, 0x10 },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x00,
            },
            .init_reg_a = 0x10,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Zero page",
            .instructions = { 0xe5, 0x01, },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x20,
            },
            .init_reg_a = 0x40,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Zero page, X",
            .instructions = { 0xf5, 0x00, },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x22,
            },
            .init_reg_x = 0x01,
            .init_reg_a = 0x42,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Absolute",
            .instructions = { 0xed, 0x00, 0x40, },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x05,
            },
            .init_reg_a = 0x05,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Absolute, X",
            .instructions = { 0xfd, 0x00, 0x40 },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x02,
            },
            .init_reg_x = 0x01,
            .init_reg_a = 0x03,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Absolute, Y",
            .instructions = { 0xf9, 0x00, 0x40 },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x5e,
            },
            .init_reg_a = 0x60,
            .init_reg_y = 0x02,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Indirect, X",
            .instructions = { 0xe1, 0x10 },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x01,
            },
            .init_reg_a = 0x03,
            .init_reg_x = 0x02,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Indirect, Y",
            .instructions = { 0xf1, 0x10 },
            .num_steps = 1,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x04,
            },
            .init_reg_a = 0x07,
            .init_reg_y = 0x03,
            .init_flags = FLAG_CARRY,
        },
        /* All tests below assumes implementation of add is same
         * regardless of address mode. */

        /* Test adds with carry. */
/*
        {
            .name = "Carry is set",
            .instructions = { 0x69, 0b11110000 },
            .num_steps = 1,
            .check_reg_a = true,
            .check_flags = true,
            .state = {
                .reg_a = 0x00,
                .flags = FLAG_CARRY|FLAG_ZERO,
            },
            .init_reg_a = 0b00010000,
        },
        {
            .name = "Carry is used",
            .instructions = { 0x69, 0x00 },
            .num_steps = 1,
            .check_reg_a = true,
            .check_flags = true,
            .state = {
                .reg_a = 0x01,
                .flags = 0x00,
            },
            .init_reg_a = 0x00,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Overflow is set",
            .instructions = { 0x69, 0b01000000 },
            .num_steps = 1,
            .check_reg_a = true,
            .check_flags = true,
            .state = {
                .reg_a = 0x80,
                .flags = FLAG_OVERFLOW|FLAG_NEGATIVE,
            },
            .init_reg_a = 0b01000000,
        },
*/
        /* Test adds with FLAG_DECIMAL_MODE set. */
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* ASL */
int test_arithmetic_shift_left()
{
    struct test tests[] = {
        {
            .name = "Accumulator",
            .instructions = { 0x0a /* ASL A */ },
            .num_steps = 1,
            .check_flags = true,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x82,
                .flags = FLAG_NEGATIVE|FLAG_CARRY,
            },
            /* Checks that MSB goes into carry and that zero
             * is padded to the right. */
            .init_reg_a = 0xc1,
            .init_flags = 0x00,
        },
        {
            .name = "Zero page",
            .instructions = { 0x06, 0x01, },
            .num_steps = 1,
            .check_ram_at = 0x0001,
            .check_ram_val = 0x40,
        },
        {
            .name = "Zero page, X",
            .instructions = { 0x16, 0x00, },
            .num_steps = 1,
            .check_ram_at = 0x0001,
            .check_ram_val = 0x40,
            .init_reg_x = 0x01,
        },
        {
            .name = "Absolute",
            .instructions = { 0x0e, 0x03, 0x40, },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x4003,
            .check_ram_val = 0x06,
            .state = {
                .flags = 0x00,
            },
            .init_flags = 0x00,
        },
        {
            .name = "Absolute, X",
            .instructions = { 0x1e, 0x00, 0x40 },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x4001,
            .check_ram_val = 0x02,
            .state = {
                .flags = 0,
            },
            .init_reg_x = 0x01,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* ROL */
int test_roll_left()
{
    struct test tests[] = {
        {
            .name = "Accumulator",
            .instructions = { 0x2a /* ROL A */ },
            .num_steps = 1,
            .check_flags = true,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x83,
                .flags = FLAG_NEGATIVE|FLAG_CARRY,
            },
            /* Checks that MSB goes into carry and that carry
             * is padded to the right. */
            .init_reg_a = 0xc1,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Zero page",
            .instructions = { 0x26, 0x01, },
            .num_steps = 1,
            .check_ram_at = 0x0001,
            .check_ram_val = 0x41,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Zero page, X",
            .instructions = { 0x36, 0x00, },
            .num_steps = 1,
            .check_ram_at = 0x0001,
            .check_ram_val = 0x40,
            .init_reg_x = 0x01,
            .init_flags = 0x00,
        },
        {
            .name = "Absolute",
            .instructions = { 0x2e, 0x03, 0x40, },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x4003,
            .check_ram_val = 0x06,
            .state = {
                .flags = 0x00,
            },
            .init_flags = 0x00,
        },
        {
            .name = "Absolute, X",
            .instructions = { 0x3e, 0x00, 0x40 },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x4001,
            .check_ram_val = 0x03,
            .state = {
                .flags = 0,
            },
            .init_reg_x = 0x01,
            .init_flags = FLAG_CARRY,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* LSR */
int test_logical_shift_right()
{
    struct test tests[] = {
        {
            .name = "Accumulator",
            .instructions = { 0x4a /* LSR A */ },
            .num_steps = 1,
            .check_flags = true,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x60,
                .flags = FLAG_CARRY,
            },
            /* Checks that LSB goes into carry and that zero
             * is padded to the left. */
            .init_reg_a = 0xc1,
            .init_flags = 0x00,
        },
        {
            .name = "Zero page",
            .instructions = { 0x46, 0x01, },
            .num_steps = 1,
            .check_ram_at = 0x0001,
            .check_ram_val = 0x10,
        },
        {
            .name = "Zero page, X",
            .instructions = { 0x56, 0x00, },
            .num_steps = 1,
            .check_ram_at = 0x0001,
            .check_ram_val = 0x10,
            .init_reg_x = 0x01,
        },
        {
            .name = "Absolute",
            .instructions = { 0x4e, 0x03, 0x40, },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x4003,
            .check_ram_val = 0x01,
            .state = {
                .flags = FLAG_CARRY,
            },
            .init_flags = 0x00,
        },
        {
            .name = "Absolute, X",
            .instructions = { 0x5e, 0x00, 0x40 },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x4001,
            .check_ram_val = 0x00,
            .state = {
                .flags = FLAG_CARRY|FLAG_ZERO,
            },
            .init_reg_x = 0x01,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* ROR */
int test_roll_right()
{
    struct test tests[] = {
        {
            .name = "Accumulator",
            .instructions = { 0x6a /* ROL A */ },
            .num_steps = 1,
            .check_flags = true,
            .check_reg_a = true,
            .state = {
                .reg_a = 0x88,
                .flags = FLAG_NEGATIVE|FLAG_CARRY,
            },
            /* Checks that MSB goes into carry and that carry
             * is padded to the right. */
            .init_reg_a = 0x11,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Zero page",
            .instructions = { 0x66, 0x01, },
            .num_steps = 1,
            .check_ram_at = 0x0001,
            .check_ram_val = 0x10,
            .init_flags = 0,
        },
        {
            .name = "Zero page, X",
            .instructions = { 0x76, 0x00, },
            .num_steps = 1,
            .check_ram_at = 0x0001,
            .check_ram_val = 0x10,
            .init_reg_x = 0x01,
            .init_flags = 0x00,
        },
        {
            .name = "Absolute",
            .instructions = { 0x6e, 0x03, 0x40, },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x4003,
            .check_ram_val = 0x01,
            .state = {
                .flags = FLAG_CARRY,
            },
            .init_flags = 0x00,
        },
        {
            .name = "Absolute, X",
            .instructions = { 0x7e, 0x00, 0x40 },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x4001,
            .check_ram_val = 0x00,
            .state = {
                .flags = FLAG_CARRY|FLAG_ZERO,
            },
            .init_reg_x = 0x01,
            .init_flags = 0,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* BIT */
int test_bit_instruction()
{
    struct test tests[] = {
        /* Tests below tests bit settings using zeropage */
        {
            .name = "MSB should set negative",
            .instructions = { 0x24, 0x06 }, /* 0x0006: 0x80 */
            .num_steps = 1,
            .state = {
                .flags = FLAG_NEGATIVE,
            },
            .check_flags = true,
            .init_reg_a = 0xff,
        },
        {
            .name = "Bit 6 should set overflow",
            .instructions = { 0x24, 0x07 }, /* 0x0007: 0x40 */
            .num_steps = 1,
            .state = {
                .flags = FLAG_OVERFLOW,
            },
            .check_flags = true,
            .init_reg_a = 0xff,
        },
        {
            .name = "Accumulator AND with op evals to ZERO",
            .instructions = { 0x24, 0x06 }, /* 0x0006: 0x80 */
            .num_steps = 1,
            .state = {
                .flags = FLAG_ZERO|FLAG_NEGATIVE,
            },
            .check_flags = true,
            .init_reg_a = (uint8_t)~0x80,
        },
        /* Check on absolute */
        {
            .name = "Absolute",
            .instructions = { 0x2c, 0x06, 0x00 }, /* 0x0006: 0x80 */
            .num_steps = 1,
            .state = {
                .flags = FLAG_NEGATIVE,
            },
            .check_flags = true,
            .init_reg_a = 0xff,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

int test_status_instructions()
{
    struct test tests[] = {
        {
            .name = "CLC, clear carry",
            .instructions = { 0x18 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_NEGATIVE,
            },
            .check_flags = true,
            .init_flags = FLAG_CARRY|FLAG_NEGATIVE,
        },
        {
            .name = "SEC, set carry",
            .instructions = { 0x38 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_CARRY,
            },
            .check_flags = true,
            .init_flags = 0x00,
        },
        {
            .name = "CLI, clear interrupt",
            .instructions = { 0x58 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_CARRY,
            },
            .check_flags = true,
            .init_flags = FLAG_CARRY|FLAG_IRQ_DISABLE,
        },
        {
            .name = "SEI, set interrupt",
            .instructions = { 0x78 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_IRQ_DISABLE,
            },
            .check_flags = true,
            .init_flags = 0x00,
        },
        {
            .name = "CLV, clear overflow",
            .instructions = { 0xb8 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_IRQ_DISABLE,
            },
            .check_flags = true,
            .init_flags = FLAG_OVERFLOW|FLAG_IRQ_DISABLE,
        },
        {
            .name = "SED, set decimal",
            .instructions = { 0xf8 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_DECIMAL_MODE,
            },
            .check_flags = true,
            .init_flags = 0x00,
        },
        {
            .name = "CLD, clear decimal",
            .instructions = { 0xd8 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_OVERFLOW,
            },
            .check_flags = true,
            .init_flags = FLAG_OVERFLOW|FLAG_DECIMAL_MODE,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* CMP */
int test_compare_acc()
{
    struct test tests[] = {
        /* Test flag settings with immediate */
        /* Carry used for unsigned comparisons */
        /* Negative used for signed comparisons */
        {
            .name = "Carry set, negative cleared on equal",
            .instructions = { 0xc9, 0x30 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_CARRY|FLAG_ZERO,
            },
            .check_flags = true,
            .init_flags = FLAG_NEGATIVE,
            .init_reg_a = 0x30,
        },
        {
            .name = "Carry set, negative cleared on larger",
            .instructions = { 0xc9, 0x30 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_CARRY,
            },
            .check_flags = true,
            .init_flags = FLAG_NEGATIVE,
            .init_reg_a = 0x40,
        },
        {
            .name = "Carry clear, negative set on less",
            .instructions = { 0xc9, 0x30 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_NEGATIVE,
            },
            .check_flags = true,
            .init_flags = FLAG_CARRY,
            .init_reg_a = 0x10,
        },
        /* Test address modes */
        {
            .name = "Zero page ",
            .instructions = { 0xc5, 0x02 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_NEGATIVE,
            },
            .check_flags = true,
            .init_reg_a = 0x10,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Zero page, X ",
            .instructions = { 0xd5, 0x01 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_NEGATIVE,
            },
            .init_reg_x = 0x03,
            .check_flags = true,
            .init_flags = FLAG_CARRY,
            .init_reg_a = 0x10,
        },
        {
            .name = "Absolute ",
            .instructions = { 0xcd, 0x01, 0x40 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_CARRY,
            },
            .check_flags = true,
            .init_reg_a = 0x10,
        },
        {
            .name = "Absolute,X ",
            .instructions = { 0xdd, 0x00, 0x40 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_CARRY,
            },
            .init_reg_x = 0x03,
            .check_flags = true,
            .init_reg_a = 0x10,
        },
        {
            .name = "Absolute,Y ",
            .instructions = { 0xd9, 0x00, 0x40 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_CARRY|FLAG_ZERO,
            },
            .init_reg_y = 0x04,
            .check_flags = true,
            .init_reg_a = 0x04,
        },
        {
            .name = "Indirect, X ",
            .instructions = { 0xc1, 0x10 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_CARRY,
            },
            .init_reg_x = 0x02,
            .check_flags = true,
            .init_reg_a = 0x10,
        },
        {
            .name = "Indirect, Y ",
            .instructions = { 0xd1, 0x10 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_CARRY,
            },
            .init_reg_y = 0x03,
            .check_flags = true,
            .init_flags = FLAG_CARRY,
            .init_reg_a = 0x10,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* CPX */
int test_compare_x()
{
    struct test tests[] = {
        {
            .name = "Immediate",
            .instructions = { 0xe0, 0x30 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_CARRY|FLAG_ZERO,
            },
            .check_flags = true,
            .init_flags = FLAG_NEGATIVE,
            .init_reg_x = 0x30,
        },
        {
            .name = "Zero page ",
            .instructions = { 0xe4, 0x02 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_NEGATIVE,
            },
            .check_flags = true,
            .init_reg_x = 0x10,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Absolute ",
            .instructions = { 0xec, 0x01, 0x40 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_CARRY,
            },
            .check_flags = true,
            .init_reg_x = 0x10,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* CPY */
int test_compare_y()
{
    struct test tests[] = {
        {
            .name = "Immediate",
            .instructions = { 0xc0, 0x30 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_CARRY|FLAG_ZERO,
            },
            .check_flags = true,
            .init_flags = FLAG_NEGATIVE,
            .init_reg_y = 0x30,
        },
        {
            .name = "Zero page ",
            .instructions = { 0xc4, 0x02 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_NEGATIVE,
            },
            .check_flags = true,
            .init_reg_y = 0x10,
            .init_flags = FLAG_CARRY,
        },
        {
            .name = "Absolute ",
            .instructions = { 0xcc, 0x01, 0x40 },
            .num_steps = 1,
            .state = {
                .flags = FLAG_CARRY,
            },
            .check_flags = true,
            .init_reg_y = 0x10,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* INC */
int test_increase()
{
    struct test tests[] = {
        {
            .name = "Zero page",
            .instructions = { 0xe6, 0x01 },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x0001,
            .check_ram_val = 0x21,
            .init_flags = FLAG_NEGATIVE|FLAG_ZERO,
            .state = {
                .flags = 0x00,
            },
        },
        {
            .name = "Zero page, X",
            .instructions = { 0xf6, 0x00, },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x0001,
            .check_ram_val = 0x21,
            .init_reg_x = 0x01,
            .init_flags = FLAG_NEGATIVE|FLAG_ZERO,
            .state = {
                .flags = 0x00,
            },
        },
        {
            .name = "Absolute",
            .instructions = { 0xee, 0x06, 0x40, },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x4006,
            .check_ram_val = 0x00,
            .state = {
                .flags = FLAG_ZERO,
            },
            .init_flags = 0x00,
        },
        {
            .name = "Absolute, X",
            .instructions = { 0xfe, 0x00, 0x40 },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x4007,
            .check_ram_val = 0x82,
            .state = {
                .flags = FLAG_NEGATIVE,
            },
            .init_reg_x = 0x07,
            .init_flags = 0,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* DEC */
int test_decrease()
{
    struct test tests[] = {
        {
            .name = "Zero page",
            .instructions = { 0xc6, 0x01 },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x0001,
            .check_ram_val = 0x1f,
            .init_flags = FLAG_NEGATIVE|FLAG_ZERO,
            .state = {
                .flags = 0x00,
            },
        },
        {
            .name = "Zero page, X",
            .instructions = { 0xd6, 0x00, },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x0001,
            .check_ram_val = 0x1f,
            .init_reg_x = 0x01,
            .init_flags = FLAG_NEGATIVE|FLAG_ZERO,
            .state = {
                .flags = 0x00,
            },
        },
        {
            .name = "Absolute",
            .instructions = { 0xce, 0x01, 0x40, },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x4001,
            .check_ram_val = 0x00,
            .state = {
                .flags = FLAG_ZERO,
            },
            .init_flags = 0x00,
        },
        {
            .name = "Absolute, X",
            .instructions = { 0xde, 0x00, 0x40 },
            .num_steps = 1,
            .check_flags = true,
            .check_ram_at = 0x4007,
            .check_ram_val = 0x80,
            .state = {
                .flags = FLAG_NEGATIVE,
            },
            .init_reg_x = 0x07,
            .init_flags = 0,
        },
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* JMP */
int test_jump()
{
    return 0;
}

/* JSR */
int test_jump_to_subroutine()
{
    return 0;
}

/* RTS */
int test_return_from_subroutine()
{
    struct test tests[] = {
        {
            .name = "Returns to address on stack",
            .instructions = { 0xa9, 0x10, /* LDA #$10 */
                              0x48,       /* PHA */
                              0xa9, 0x20, /* LDA #$20 */
                              0x48,       /* PHA */
                              0x60,       /* RTS */
                            },
            .num_steps = 5,
            .check_pc = true,
            .state = {
                .pc = 0x1020,
            },
        },
    };
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}

/* RTI */
int test_return_from_interrupt()
{
    return 0;
}

/* DEX, DEY */
int test_decrement_register()
{
    return 0;
}

/* INX, INY */
int test_increment_register()
{
    return 0;
}
