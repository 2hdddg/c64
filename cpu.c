#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "cpu_ops.h"
#include "cpu.h"
#include "cpu_instr.h"


/* Needed for cycle calculation and edge behaviours */
#define PAGE_SIZE           0x100

/* CPU hardwired addresses */
#define ADDR_STACK_START    0x0100
#define ADDR_IRQ_VECTOR     0xfffe


struct cpu_h {
    /* Memory access/io control */
    cpu_mem_get  mem_get;
    cpu_mem_set  mem_set;
    /* Actual registers and status */
    struct cpu_state state;

    bool             irq_pending;

    /* For debugging */
    int              disassemble_fd;
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
    cpu->mem_set(address, val);
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
    val = cpu->mem_get(address);

    return val;
}

static void stack_push_address(struct cpu_h *cpu,
                               uint16_t addr)
{
    stack_push(cpu, addr & 0xff);
    stack_push(cpu, addr >> 8);
}

static uint16_t stack_pop_address(struct cpu_h *cpu)
{
    uint8_t  lo = stack_pop(cpu);
    uint8_t  hi = stack_pop(cpu);

    return hi << 8 | lo;
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

static void read_address(struct cpu_h *cpu,
                         uint16_t addr, uint16_t *out)
{
    uint8_t lo = cpu->mem_get(addr);
    uint8_t hi = cpu->mem_get(addr + 1);

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

static void trace_register(int fd, char name,
                           uint8_t val0, uint8_t val1)
{
    char trace[10];

    if (val0 != val1) {
        sprintf(trace, " %c=$%02x ", name, val1);
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

static void trace_instruction(int fd,
                              uint16_t address,
                              struct instruction *instr,
                              int pad)
{
    char    trace[20];
    uint8_t ops_len = 0;

    sprintf(trace, "$%04x %s ",
            address,
            mnemonics_strings[instr->operation->mnem]);
    write(fd, trace, strlen(trace));
    trace[0] = 0;

    write(fd, instr->operation->undocumented ? "*" : " ", 1);

    switch (instr->operation->mode) {
    case Absolute:
        sprintf(trace, "$%02x%02x",
                instr->operands[1], instr->operands[0]);
        break;
    case Absolute_X:
        sprintf(trace, "$%02x%02x,X",
                instr->operands[1], instr->operands[0]);
        break;
    case Absolute_Y:
        sprintf(trace, "$%02x%02x,Y",
                instr->operands[1], instr->operands[0]);
        break;
    case Accumulator:
        sprintf(trace, "A");
        break;
    case Immediate:
        sprintf(trace, "#$%02x", instr->operands[0]);
        break;
    case Implied:
        break;
    case Indirect:
        sprintf(trace, "($%02x%02x)",
                instr->operands[1], instr->operands[0]);
        break;
    case Indirect_X:
        sprintf(trace, "($%02x,X)",
                instr->operands[0]);
        break;
    case Indirect_Y:
        sprintf(trace, "($%02x),Y",
                instr->operands[0]);
        break;
    case Relative:
        if (instr->operands[0] & 0x80) {
            sprintf(trace, "-$%02x", (0x100 - instr->operands[0]));
        } else {
            sprintf(trace, "+$%02x", instr->operands[0] + 2);
        }
        break;
    case Zeropage:
        sprintf(trace, "$%02x", instr->operands[0]);
        break;
    case Zeropage_X:
        sprintf(trace, "$%02x, X", instr->operands[0]);
        break;
    case Zeropage_Y:
        sprintf(trace, "$%02x, Y", instr->operands[0]);
        break;
    default:
        trace[0] = '?';
        trace[1] = 0;
        break;
    }
    ops_len = strlen(trace);
    write(fd, trace, ops_len);
    while (ops_len < pad) {
        write(fd, " ", 1);
        ops_len++;
    }
}

static void trace_execution(int fd, struct instruction *instr,
                            struct cpu_state *s0,
                            struct cpu_state *s1)
{
    if (fd < 0)
        return;

    trace_instruction(fd, s0->pc, instr, 10);
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
    read_address(cpu, ADDR_IRQ_VECTOR, &handler_address);
    /* Point program counter to IRQ handler routine */
    cpu->state.pc = handler_address;
}

static uint16_t get_address_from_mode(struct cpu_h *cpu,
                                      struct instruction *instr)
{
    uint16_t         address;
    uint8_t          *ops = instr->operands;
    struct cpu_state *state = &cpu->state;

    switch (instr->operation->mode) {
    case Absolute:
        address = make_address(ops[1], ops[0]);
        break;
    case Absolute_X:
        address = make_address(ops[1], ops[0]);
        address += state->reg_x;
        break;
    case Absolute_Y:
        address = make_address(ops[1], ops[0]);
        address += state->reg_y;
        break;
    case Indirect_X:
        address = ops[0];
        address += state->reg_x;
        break;
    case Indirect_Y:
        address = ops[0];
        address = make_address(cpu->mem_get(address + 1),
                               cpu->mem_get(address));
        address += state->reg_y;
        break;
    case Zeropage:
        address = ops[0];
        break;
    case Zeropage_X:
        address = ops[0] + state->reg_x;
        break;
    case Zeropage_Y:
        address = ops[0] + state->reg_y;
        break;
    default:
        printf("Unhandled address mode\n");
        break;
    }
    return address;
}

static void load(struct cpu_h *cpu,
                 struct instruction *instr,
                 uint8_t *reg_out)
{
    uint8_t  operand;
    uint16_t address;

    if (instr->operation->mode == Immediate) {
        operand = instr->operands[0];
    }
    else {
        address = get_address_from_mode(cpu, instr);
        operand = cpu->mem_get(address);
    }

    cpu_instr_transfer(operand, reg_out, &cpu->state.flags);
}

static void store(struct cpu_h *cpu,
                  struct instruction *instr,
                  uint8_t reg)
{
    uint16_t address;

    address = get_address_from_mode(cpu, instr);
    cpu->mem_set(address, reg);
}

static void and(struct cpu_h *cpu,
                struct instruction *instr)
{
    uint8_t          operand = 0;
    struct cpu_state *state = &cpu->state;
    uint16_t         address;

    if (instr->operation->mode == Immediate) {
        operand = instr->operands[0];
    }
    else {
        address = get_address_from_mode(cpu, instr);
        operand = cpu->mem_get(address);
    }

    cpu_instr_and(&state->reg_a, operand, &state->flags);
}

static void or(struct cpu_h *cpu,
               struct instruction *instr)
{
    uint8_t          operand = 0;
    struct cpu_state *state = &cpu->state;
    uint16_t         address;

    if (instr->operation->mode == Immediate) {
        operand = instr->operands[0];
    }
    else {
        address = get_address_from_mode(cpu, instr);
        operand = cpu->mem_get(address);
    }

    cpu_instr_or(&state->reg_a, operand, &state->flags);
}

static void asl(struct cpu_h *cpu,
                struct instruction *instr)
{
    uint8_t  operand;
    uint16_t address;
    uint8_t  shifted;

    if (instr->operation->mode == Accumulator) {
        operand = cpu->state.reg_a;
    }
    else {
        address = get_address_from_mode(cpu, instr);
        operand = cpu->mem_get(address);
    }

    cpu_instr_asl(operand, &shifted, &cpu->state.flags);

    if (instr->operation->mode == Accumulator) {
        cpu->state.reg_a = shifted;
    }
    else {
        cpu->mem_set(address, shifted);
    }
}

static void lsr(struct cpu_h *cpu,
                struct instruction *instr)
{
    uint8_t operand;
    uint8_t shifted;
    uint16_t address;

    if (instr->operation->mode == Accumulator) {
        operand = cpu->state.reg_a;
    }
    else {
        address = get_address_from_mode(cpu, instr);
        operand = cpu->mem_get(address);
    }

    cpu_instr_lsr(operand, &shifted, &cpu->state.flags);

    if (instr->operation->mode == Accumulator) {
        cpu->state.reg_a = shifted;
    }
    else {
        cpu->mem_set(address, shifted);
    }
}

static void rol(struct cpu_h *cpu,
                struct instruction *instr)
{
    uint8_t  operand;
    uint16_t address;
    uint8_t  shifted;

    if (instr->operation->mode == Accumulator) {
        operand = cpu->state.reg_a;
    }
    else {
        address = get_address_from_mode(cpu, instr);
        operand = cpu->mem_get(address);
    }

    cpu_instr_rol(operand, &shifted, &cpu->state.flags);

    if (instr->operation->mode == Accumulator) {
        cpu->state.reg_a = shifted;
    }
    else {
        cpu->mem_set(address, shifted);
    }
}

static void ror(struct cpu_h *cpu,
                struct instruction *instr)
{
    uint8_t  operand;
    uint16_t address;
    uint8_t  shifted;

    if (instr->operation->mode == Accumulator) {
        operand = cpu->state.reg_a;
    }
    else {
        address = get_address_from_mode(cpu, instr);
        operand = cpu->mem_get(address);
    }

    cpu_instr_ror(operand, &shifted, &cpu->state.flags);

    if (instr->operation->mode == Accumulator) {
        cpu->state.reg_a = shifted;
    }
    else {
        cpu->mem_set(address, shifted);
    }
}

static void inc_dec(struct cpu_h *cpu,
                    struct instruction *instr,
                    int8_t delta)
{
    uint8_t  operand;
    uint16_t address;
    uint8_t  increased;

    address = get_address_from_mode(cpu, instr);
    operand = cpu->mem_get(address);

    cpu_instr_inc_dec(operand, delta, &increased, &cpu->state.flags);

    cpu->mem_set(address, increased);
}

static void bit(struct cpu_h *cpu,
                struct instruction *instr)
{
    uint8_t  operand;
    uint16_t address;

    address = get_address_from_mode(cpu, instr);
    operand = cpu->mem_get(address);

    cpu_instr_bit(operand, cpu->state.reg_a, &cpu->state.flags);
}

static void add(struct cpu_h *cpu,
                struct instruction *instr)
{
    uint8_t          operand = 0;
    struct cpu_state *state = &cpu->state;
    uint8_t          carry = state->flags & FLAG_CARRY ? 1 : 0;
    uint16_t         address;

    if (instr->operation->mode == Immediate) {
        operand = instr->operands[0];
    }
    else {
        address = get_address_from_mode(cpu, instr);
        operand = cpu->mem_get(address);
    }

    if (state->flags & FLAG_DECIMAL_MODE) {
        /* Copied from:
         * http://www.zimmers.net/anonftp/pub/cbm/documents/chipdata/64doc
         */
        uint8_t a_lo = (state->reg_a & 0x0f) +
                       (operand & 0x0f) + carry;
        uint8_t a_hi = (state->reg_a >> 4) +
                       (operand >> 4) + a_lo > 15 ? 1 : 0;
        a_lo += a_lo > 9 ? 6 : 0;

        clear_flag(state, FLAG_NEGATIVE);
        clear_flag(state, FLAG_CARRY);
        clear_flag(state, FLAG_OVERFLOW);
        clear_flag(state, FLAG_ZERO);

        if (((state->reg_a + operand + carry) & 0xff) == 0) {
            set_flag(state, FLAG_ZERO);
        }
        if ((a_hi & 0x08) != 0) {
            set_flag(state, FLAG_NEGATIVE);
        }
        if (((a_hi << 4) ^ state->reg_a) & 0x80 &&
            !((state->reg_a ^ operand) & 0x80)) {
            set_flag(state, FLAG_OVERFLOW);
        }

        a_hi += a_hi > 9 ? 6 : 0;

        if (a_hi > 15) {
            set_flag(state, FLAG_CARRY);
        }

        /* Store the new value */
        state->reg_a = ((a_hi << 4) | (a_lo & 0x0f)) & 0xff;
    }
    else {
        cpu_instr_add(state->reg_a, operand,
                      &state->reg_a, &state->flags);
    }
}

static void subtract(struct cpu_h *cpu,
                     struct instruction *instr)
{
    uint8_t          operand = 0;
    struct cpu_state *state = &cpu->state;
    uint16_t         address;

    if (instr->operation->mode == Immediate) {
        operand = instr->operands[0];
    }
    else {
        address = get_address_from_mode(cpu, instr);
        operand = cpu->mem_get(address);
    }

    if (state->flags & FLAG_DECIMAL_MODE) 
        printf("Decimal mode not supported on sub\n");
    else {
        cpu_instr_sub(state->reg_a, operand,
                      &state->reg_a, &state->flags);
    }
}

static void compare(struct cpu_h *cpu,
                   struct instruction *instr,
                   uint8_t compare_to)
{
    uint8_t          operand = 0;
    struct cpu_state *state = &cpu->state;
    uint16_t         address;

    if (instr->operation->mode == Immediate) {
        operand = instr->operands[0];
    }
    else {
        address = get_address_from_mode(cpu, instr);
        operand = cpu->mem_get(address);
    }

    cpu_instr_compare(compare_to, operand, &state->flags);
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
            cpu->state.pc -= 0x100 - offset;
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

static void jump(struct cpu_h *cpu,
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
        lo = cpu->mem_get(address);
        address++;
        if (page != get_page(address)) {
            /* Illegal case, indirect jump with vector starting
             * on the last byte of a page. */
            address = get_page_address(page);
        }
        hi = cpu->mem_get(address);
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

    switch (instr->operation->mnem) {
    case BRK:
        /* Signals that IRQ is due to break */
        set_flag(state, FLAG_BRK);
        /* Exception on how program counter is counted */
        state->pc++;
        cpu->irq_pending = true;
        break;

    /* Stack instructions */
    case PHA:
        stack_push(cpu, state->reg_a);
        break;
    case PHP:
        stack_push(cpu, state->flags);
        break;
    case PLA:
        cpu_instr_transfer(stack_pop(cpu),
                           &state->reg_a,
                           &state->flags);
        break;
    case PLP:
        state->flags = stack_pop(cpu);
        break;
    case TXS:
        state->sp = state->reg_x;
        break;
    case TSX:
        cpu_instr_transfer(state->sp,
                           &state->reg_x,
                           &state->flags);
        break;

    /* Transfer instructions */
    case TAX:
        cpu_instr_transfer(state->reg_a,
                           &state->reg_x,
                           &state->flags);
        break;
    case TXA:
        cpu_instr_transfer(state->reg_x,
                           &state->reg_a,
                           &state->flags);
        break;
    case TAY:
        cpu_instr_transfer(state->reg_a,
                           &state->reg_y,
                           &state->flags);
    case TYA:
        cpu_instr_transfer(state->reg_y,
                           &state->reg_a,
                           &state->flags);
        break;

    /* Load instructions */
    case LDA:
        load(cpu, instr, &cpu->state.reg_a);
        break;
    case LDX:
        load(cpu, instr, &cpu->state.reg_x);
        break;
    case LDY:
        load(cpu, instr, &cpu->state.reg_y);
        break;

    /* Store instructions */
    case STA:
        store(cpu, instr, cpu->state.reg_a);
        break;
    case STX:
        store(cpu, instr, cpu->state.reg_x);
        break;
    case STY:
        store(cpu, instr, cpu->state.reg_y);
        break;

    /* ALU instructions */
    case ADC:
        add(cpu, instr);
        break;
    case SBC:
        subtract(cpu, instr);
        break;
    case AND:
        and(cpu, instr);
        break;
    case ORA:
        or(cpu, instr);
        break;
    case ASL:
        asl(cpu, instr);
        break;
    case ROL:
        rol(cpu, instr);
        break;
    case LSR:
        lsr(cpu, instr);
        break;
    case ROR:
        ror(cpu, instr);
        break;
    case BIT:
        bit(cpu, instr);
        break;
    case CMP:
        compare(cpu, instr, cpu->state.reg_a);
        break;
    case CPX:
        compare(cpu, instr, cpu->state.reg_x);
        break;
    case CPY:
        compare(cpu, instr, cpu->state.reg_y);
        break;
    case INC:
        inc_dec(cpu, instr, 1);
        break;
    case DEC:
        inc_dec(cpu, instr, -1);
        break;
    case INX:
        cpu_instr_inc_dec(cpu->state.reg_x, 1,
                          &cpu->state.reg_x, &cpu->state.flags);
        break;
    case INY:
        cpu_instr_inc_dec(cpu->state.reg_y, 1,
                          &cpu->state.reg_y, &cpu->state.flags);
        break;
    case DEX:
        cpu_instr_inc_dec(cpu->state.reg_x, -1,
                          &cpu->state.reg_x, &cpu->state.flags);
        break;
    case DEY:
        cpu_instr_inc_dec(cpu->state.reg_y, -1,
                          &cpu->state.reg_y, &cpu->state.flags);
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
    case BPL:
        branch(cpu, instr,
               (cpu->state.flags & FLAG_NEGATIVE) == 0);
        break;
    case BMI:
        branch(cpu, instr,
               cpu->state.flags & FLAG_NEGATIVE);
        break;
    case BVC:
        branch(cpu, instr,
               (cpu->state.flags & FLAG_OVERFLOW) == 0);
        break;
    case BVS:
        branch(cpu, instr,
               cpu->state.flags & FLAG_OVERFLOW);
        break;
    case BCC:
        branch(cpu, instr,
               (cpu->state.flags & FLAG_CARRY) == 0);
        break;
    case BCS:
        branch(cpu, instr,
               cpu->state.flags & FLAG_CARRY);
        break;

    /* Jump instructions */
    case JMP:
        jump(cpu, instr);
        break;
    case RTS:
        cpu->state.pc = stack_pop_address(cpu);
        break;

    /* Status register instuctions */
    case CLC:
        clear_flag(&cpu->state, FLAG_CARRY);
        break;
    case SEC:
        set_flag(&cpu->state, FLAG_CARRY);
        break;
    case CLI:
        clear_flag(&cpu->state, FLAG_IRQ_DISABLE);
        break;
    case SEI:
        set_flag(&cpu->state, FLAG_IRQ_DISABLE);
        break;
    case SED:
        set_flag(&cpu->state, FLAG_DECIMAL_MODE);
        break;
    case CLD:
        clear_flag(&cpu->state, FLAG_DECIMAL_MODE);
        break;
    case CLV:
        clear_flag(&cpu->state, FLAG_OVERFLOW);
        break;

    default:
        printf("Unknown mnem\n");
    }

    /* Writes instruction and registers to debug fd */
    trace_execution(cpu->disassemble_fd, instr,
                    &cpu->state_before, &cpu->state);

    return 0;
}

static int get_num_operands(addressing_modes mode)
{
    switch (mode) {
    case Absolute:
    case Absolute_Y:
    case Absolute_X:
        return 2;
    case Accumulator:
        return 0;
    case Immediate:
        return 1;
    case Implied:
        return 0;
    case Indirect:
        return 2;
    case Indirect_X:
    case Indirect_Y:
        return 1;
    case Relative:
        return 1;
    case Zeropage:
    case Zeropage_X:
    case Zeropage_Y:
        return 1;
        return 1;
    case Undefined:
    default:
        return 0;
    }
}

static int fetch_and_decode(struct cpu_h *cpu,
                            struct instruction *instr)
{
    uint8_t      op_code;
    int          num_operands;
    uint8_t      *operands = instr->operands;

    op_code = cpu->mem_get(cpu->state.pc++);
    instr->operation = &opcodes[op_code];

    num_operands = get_num_operands(instr->operation->mode);
    switch (num_operands) {
    case 0:
        break;
    case 1:
        operands[0] = cpu->mem_get(cpu->state.pc++);
        break;
    case 2:
        operands[0] = cpu->mem_get(cpu->state.pc++);
        operands[1] = cpu->mem_get(cpu->state.pc++);
        break;
    default:
        printf("Unknown number of operands for mode: %d\n",
               instr->operation->mode);
        break;
    }

    return 0;
}

int cpu_create(int disassemble_fd,
               cpu_mem_get mem_get,
               cpu_mem_set mem_set,
               struct cpu_h **cpu_out)
{
    struct cpu_h *cpu;

    *cpu_out = NULL;

    cpu = calloc(1, sizeof(*cpu));
    if (!cpu) {
        return -1;
    }

    /* Initialize self */
    cpu->mem_get = mem_get;
    cpu->mem_set = mem_set;
    cpu->disassemble_fd = disassemble_fd;

    *cpu_out = cpu;
    return 0;
}

int cpu_poweron(struct cpu_h *cpu)
{
    /* Empty stack */
    cpu->state.sp = 0xff;

    return 0;
}

int cpu_set_state(struct cpu_h *cpu, struct cpu_state *state)
{
    cpu->state = *state;
    return 0;
}

int cpu_step(struct cpu_h *cpu,
             struct cpu_state *state_out)
{
    struct instruction instr;

    if (cpu->irq_pending) {
        interrupt_request(cpu);
        cpu->irq_pending = false;
    }

    /* Copy for debugging purposes. */
    cpu->state_before = cpu->state;

    fetch_and_decode(cpu, &instr);
    execute(cpu, &instr);

    if (state_out) {
        *state_out = cpu->state;
    }

    return 0;
}

static void get_instruction(struct cpu_h *cpu,
                            uint16_t address,
                            struct instruction *instr,
                            uint8_t *offset)
{
    uint8_t op_code;
    int     num_operands;
    uint8_t *operands = instr->operands;

    op_code = cpu->mem_get(address++);
    instr->operation = &opcodes[op_code];

    num_operands = get_num_operands(instr->operation->mode);
    if (num_operands > 0) {
        operands[0] = cpu->mem_get(address++);
    }
    if (num_operands > 1) {
        operands[1] = cpu->mem_get(address++);
    }
    *offset = num_operands + 1;
}

void cpu_disassembly_at(struct cpu_h *cpu,
                        int fd,
                        uint16_t address,
                        int num_instructions)
{
    struct instruction instr;
    uint8_t offset;

    while (num_instructions--) {
        get_instruction(cpu, address, &instr, &offset);
        trace_instruction(fd, address, &instr, 0);
        address += offset;
        write(fd, "\n", 1);
    }
}

void cpu_destroy(struct cpu_h *cpu)
{
    free(cpu);
}

