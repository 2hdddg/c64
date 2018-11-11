#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "cpu_ops.h"
#include "cpu.h"


/* Processor status flags */
#define FLAG_CARRY        0x01
#define FLAG_ZERO         0x02
#define FLAG_IRQ_DISABLE  0x04
#define FLAG_DECIMAL_MODE 0x08
#define FLAG_BRK          0x10
#define FLAG_OVERFLOW     0x40
#define FLAG_NEGATIVE     0x80

/* Needed for cycle calculation and edge behaviours */
#define PAGE_SIZE           0x100

/* CPU hardwired addresses */
#define ADDR_STACK_START    0x0100
#define ADDR_IRQ_VECTOR     0xfffe


struct cpu_h {
    /* Memory access/io control */
    struct mem_h     *mem;
    /* Actual registers and status */
    struct cpu_state state;

    /* For debugging */
    int              execution_fd;
    bool             stack_overflow;
    bool             stack_underflow;
    struct cpu_state state_before;
};

struct instruction {
    const struct operation *operation;
    uint8_t                operands[2];
};

static void stack_push(struct cpu_h *cpu,
                       uint8_t val)
{
    uint16_t address;

    address = ADDR_STACK_START + cpu->state.sp;
    mem_set(cpu->mem, address, val);
    cpu->state.sp--;
    cpu->stack_overflow |= cpu->state.sp == 0xff;
}

static uint8_t stack_pop(struct cpu_h *cpu)
{
    uint16_t address;
    uint8_t  val;

    cpu->state.sp++;
    cpu->stack_underflow |= cpu->state.sp == 0x00;
    address = ADDR_STACK_START + cpu->state.sp;
    val = mem_get(cpu->mem, address);

    return val;
}

static void stack_push_address(struct cpu_h *cpu,
                               uint16_t addr)
{
    stack_push(cpu, addr & 0xff);
    stack_push(cpu, addr >> 8);
}

static uint8_t get_page(uint16_t addr)
{
    return addr / PAGE_SIZE;
}

static uint16_t get_page_address(uint8_t page)
{
    return page * PAGE_SIZE;
}

static uint16_t make_address(uint8_t hi, uint8_t lo)
{
    return hi << 8 | lo;
}

static void read_address(struct mem_h *mem,
                         uint16_t addr, uint16_t *out)
{
    uint8_t lo = mem_get(mem, addr);
    uint8_t hi = mem_get(mem, addr + 1);

    *out = make_address(hi, lo);
}

static inline void clear_flag(struct cpu_state *state, uint8_t flag)
{
    state->flags &= ~flag;
}

static inline void set_flag(struct cpu_state *state, uint8_t flag)
{
    state->flags |= flag;
}

static void eval_zero_flag(struct cpu_state *state,
                           uint8_t val)
{
    clear_flag(state, FLAG_ZERO);
    set_flag(state, val == 0 ? FLAG_ZERO : 0);
}

static void eval_neg_flag(struct cpu_state *state,
                          uint8_t val)
{
    clear_flag(state, FLAG_NEGATIVE);
    set_flag(state, val & 0x80 ? FLAG_NEGATIVE : 0);
}

static void trace_register(int fd, char name,
                           uint8_t val0, uint8_t val1)
{
    char trace[10];

    if (val0 != val1) {
        sprintf(trace, "%c=$%02x ", name, val1);
        write(fd, trace, 6);
    }
}

static void trace_flags(int fd, uint8_t s)
{
    char trace[8];

    trace[7] = s & FLAG_CARRY        ? 'C' : ' ';
    trace[6] = s & FLAG_ZERO         ? 'Z' : ' ';
    trace[5] = s & FLAG_IRQ_DISABLE  ? 'I' : ' ';
    trace[4] = s & FLAG_DECIMAL_MODE ? 'D' : ' ';
    trace[3] = s & FLAG_BRK          ? 'B' : ' ';
    trace[2] = ' '; /* Unused */
    trace[1] = s & FLAG_OVERFLOW     ? 'V' : ' ';
    trace[0] = s & FLAG_NEGATIVE     ? 'N' : ' ';
    write(fd, trace, 8);
}

static void trace_execution(int fd, struct instruction *instr,
                            struct cpu_state *s0,
                            struct cpu_state *s1)
{
    char    trace[20];
    uint8_t ops_max_len = 10;
    uint8_t ops_len = 0;

    if (fd < 0)
        return;

    sprintf(trace, "$%04x %s ",
            s0->pc,
            mnemonics_strings[instr->operation->mnem]);
    write(fd, trace, strlen(trace));
    trace[0] = 0;

    write(fd, instr->operation->undocumented ? "*" : " ", 1);

    switch (instr->operation->mode) {
    case Absolute_X:
        sprintf(trace, "$%02x%02x, X ",
                instr->operands[1], instr->operands[0]);
        break;
    case Immediate:
        sprintf(trace, "#$%02x", instr->operands[0]);
        break;
    case Implied:
        break;
    case Indirect:
        sprintf(trace, "($%02x%02x) ",
                instr->operands[1], instr->operands[0]);
        break;
    case Relative:
        if (instr->operands[0] & 0x80) {
            sprintf(trace, "-$%02x", (instr->operands[0] - 0x80));
        } else {
            sprintf(trace, "+$%02x", instr->operands[0] + 2);
        }
        break;
    default:
        trace[0] = '?';
        trace[1] = 0;
        break;
    }
    ops_len = strlen(trace);
    write(fd, trace, ops_len);
    while (ops_len < ops_max_len) {
        write(fd, " ", 1);
        ops_len++;
    }

    trace_flags(fd, s1->flags);

    trace_register(fd, 'A', s0->reg_a, s1->reg_a);
    trace_register(fd, 'X', s0->reg_x, s1->reg_x);
    trace_register(fd, 'Y', s0->reg_y, s1->reg_y);
    trace_register(fd, 'S', s0->sp, s1->sp);

    write(fd, "\n", 1);
}

static void interrupt_request(struct cpu_h *cpu)
{
    uint16_t handler_address;

    /* Push program counter and status register on stack */
    stack_push_address(cpu, cpu->state.pc);
    stack_push(cpu, cpu->state.flags);

    clear_flag(&cpu->state, FLAG_BRK);

    /* Retrieve handler at cpu hardwired address */
    read_address(cpu->mem, ADDR_IRQ_VECTOR, &handler_address);
    /* Point program counter to IRQ handler routine */
    cpu->state.pc = handler_address;
}

static void transfer(struct cpu_h *cpu,
                     uint8_t *to,
                     uint8_t from)
{
    struct cpu_state *state = &cpu->state;

    *to = from;

    /* Set flags for new value */
    eval_zero_flag(state, from);
    eval_neg_flag(state, from);
}

static void lda(struct cpu_h *cpu,
                struct instruction *instr)
{
    uint16_t         address;
    uint8_t          *ops = instr->operands;
    uint8_t          reg_a;
    struct cpu_state *state = &cpu->state;

    switch (instr->operation->mode) {
    case Absolute_X:
        // TODO: Calculate page boundary !
        address = make_address(ops[1], ops[0]);
        address += state->reg_x;
        reg_a = mem_get(cpu->mem, address);
        break;
    case Immediate:
        reg_a = ops[0];
        break;
    default:
        printf("Unhandled address mode\n");
        break;
    }

    /* Store new value */
    state->reg_a = reg_a;
    /* Set flags for new value of reg_a */
    eval_zero_flag(state, reg_a);
    eval_neg_flag(state, reg_a);
}

static void and(struct cpu_h *cpu,
                struct instruction *instr)
{
    uint8_t          operand = 0;
    struct cpu_state *state = &cpu->state;

    switch (instr->operation->mode) {
    case Immediate:
        operand = instr->operands[0];
        break;
    default:
        printf("Unhandled address mode\n");
        break;
    }

    /* Store the new value */
    state->reg_a &= operand;
    /* Set flags for new value of reg_a */
    eval_zero_flag(state, state->reg_a);
    eval_neg_flag(state, state->reg_a);
}

static void branch(struct cpu_h *cpu,
                   struct instruction *instr,
                   int do_branch)
{
    uint8_t offset;

    if (!do_branch) {
        return;
    }

    /* Branch instructions: BPL, ... a branch not taken: 2 cycles,
     *  branch taken: 2 cycles + 1 if crosses page boundary */

    switch (instr->operation->mode) {
    case Relative:
        /* Jumps relative to current address */
        offset = instr->operands[0];
        if (offset & 0x80) {
            cpu->state.pc -= offset - 0x80;
        }
        else {
            cpu->state.pc += offset;
        }
        break;
    default:
        printf("Unhandled address mode\n");
        break;
    }
}

static void jmp(struct cpu_h *cpu,
                struct instruction *instr)
{
    uint16_t address;
    uint8_t  lo, hi, page;
    uint8_t  *ops = instr->operands;

    switch (instr->operation->mode) {
    case Absolute:
        hi = ops[1];
        lo = ops[0];
        break;
    case Indirect:
        address = make_address(ops[1], ops[0]);
        page = get_page(address);
        lo = mem_get(cpu->mem, address);
        address++;
        if (page != get_page(address)) {
            /* Illegal case, indirect jump with vector starting
             * on the last byte of a page. */
            address = get_page_address(page);
        }
        hi = mem_get(cpu->mem, address);
        break;
    default:
        printf("Unhandled address mode\n");
        break;
    }

    address = make_address(hi, lo);
    cpu->state.pc = address;
}

static int execute(struct cpu_h *cpu,
                   struct instruction *instr)
{
    struct cpu_state *state = &cpu->state;
    bool             irq = false;

    switch (instr->operation->mnem) {
    case BRK:
        /* Signals that IRQ is due to break */
        set_flag(state, FLAG_BRK);
        /* Exception on how program counter is counted */
        state->pc++;
        /* BRK is non-interruptable but still handled by an
         * interrupt request.
         * TODO: Clear irq_disable instead? */
        irq = true;
        break;

    /* Stack instructions */
    case PHA:
        stack_push(cpu, state->reg_a);
        break;

    /* Transfer instructions */
    case TAX:
        transfer(cpu, &state->reg_x, state->reg_a);
        break;
    case TXA:
        transfer(cpu, &state->reg_a, state->reg_x);
        break;
    case TYA:
        transfer(cpu, &state->reg_a, state->reg_y);
        break;
    case TSX:
        transfer(cpu, &state->reg_x, state->sp);
        break;

    /* Load instructions */
    case LDA:
        lda(cpu, instr);
        break;

    /* ALU instructions */
    case AND:
        and(cpu, instr);
        break;

    /* Branch instructions */
    case BEQ:
        branch(cpu, instr,
               cpu->state.flags & FLAG_ZERO);
        break;
    case BNE:
        branch(cpu, instr,
               (cpu->state.flags & FLAG_ZERO) == 0);
        break;

    /* Jump instructions */
    case JMP:
        jmp(cpu, instr);
        break;

    default:
        printf("Unknown mnem\n");
    }

    /* Writes instruction and registers to debug fd */
    trace_execution(cpu->execution_fd, instr,
                    &cpu->state_before, &cpu->state);

    /* After trace to be able to show B flag */
    if (irq) {
        interrupt_request(cpu);
    }
    return 0;
}

static int fetch_and_decode(struct cpu_h *cpu,
                            struct instruction *instr)
{
    struct mem_h *mem = cpu->mem;
    uint8_t      op_code;
    int          num_operands;
    uint8_t      *operands = instr->operands;

    op_code = mem_get(mem, cpu->state.pc++);
    instr->operation = &operations[op_code];

    num_operands = get_num_operands(instr->operation->mode);
    switch (num_operands) {
    case 0:
        break;
    case 1:
        operands[0] = mem_get(mem, cpu->state.pc++);
        break;
    case 2:
        operands[0] = mem_get(mem, cpu->state.pc++);
        operands[1] = mem_get(mem, cpu->state.pc++);
        break;
    default:
        printf("Unknown number of operands\n");
        break;
    }

    return 0;
}

int cpu_create(struct mem_h *mem,
               int execution_fd,
               struct cpu_h **cpu_out)
{
    struct cpu_h *cpu;

    *cpu_out = NULL;

    cpu = calloc(1, sizeof(*cpu));
    if (!cpu) {
        return -1;
    }

    /* Initialize self */
    cpu->mem = mem;
    cpu->execution_fd = execution_fd;

    *cpu_out = cpu;
    return 0;
}

int cpu_poweron(struct cpu_h *cpu)
{
    mem_reset(cpu->mem);

    /* Empty stack */
    cpu->state.sp = 0xff;

    return 0;
}

int cpu_set_pc(struct cpu_h *cpu, uint16_t pc)
{
    cpu->state.pc = pc;
    return 0;
}

int cpu_step(struct cpu_h *cpu,
             struct cpu_state *state_out)
{
    struct instruction instr;

    /* Copy for debugging purposes. */
    cpu->state_before = cpu->state;

    fetch_and_decode(cpu, &instr);
    execute(cpu, &instr);

    if (state_out) {
        *state_out = cpu->state;
    }

    return 0;
}

void cpu_destroy(struct cpu_h *cpu)
{
    free(cpu);
}

