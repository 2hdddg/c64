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

/* Memory access */
cpu_mem_get _mem_get;
cpu_mem_set _mem_set;

/* Actual registers and status */
struct cpu_state _state;

/* Interrupt handling */
bool             _irq_pending;

/* For debugging */
int              _trace_fd;
bool             _stack_overflow;
bool             _stack_underflow;
struct cpu_state _state_before;

struct instruction {
    const struct operation *operation;
    uint8_t                operands[2];
};

static void stack_push(uint8_t val)
{
    uint16_t address;

    address = ADDR_STACK_START + _state.sp;
    _mem_set(address, val);
    _state.sp--;
    _stack_overflow |= _state.sp == 0xff;
}

static uint8_t stack_pop()
{
    uint16_t address;
    uint8_t  val;

    _state.sp++;
    _stack_underflow |= _state.sp == 0x00;
    address = ADDR_STACK_START + _state.sp;
    val = _mem_get(address);

    return val;
}

static void stack_push_address(uint16_t addr)
{
    stack_push(addr >> 8);
    stack_push(addr & 0xff);
}

static uint16_t stack_pop_address()
{
    uint8_t  lo = stack_pop();
    uint8_t  hi = stack_pop();

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

static void read_address(uint16_t addr, uint16_t *out)
{
    uint8_t lo = _mem_get(addr);
    uint8_t hi = _mem_get(addr + 1);

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

static void interrupt_request()
{
    uint16_t handler_address;

    /* Push program counter and status register on stack */
    stack_push_address(_state.pc);
    stack_push(_state.flags);

    clear_flag(&_state, FLAG_BRK);

    /* Retrieve handler at cpu hardwired address */
    read_address(ADDR_IRQ_VECTOR, &handler_address);
    /* Point program counter to IRQ handler routine */
    _state.pc = handler_address;
}

static uint16_t get_address_from_mode(struct instruction *instr)
{
    uint16_t         address;
    uint8_t          *ops = instr->operands;
    struct cpu_state *state = &_state;

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
        address = make_address(_mem_get(address + 1),
                               _mem_get(address));
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

static void load(struct instruction *instr,
                 uint8_t *reg_out)
{
    uint8_t  operand;
    uint16_t address;

    if (instr->operation->mode == Immediate) {
        operand = instr->operands[0];
    }
    else {
        address = get_address_from_mode(instr);
        operand = _mem_get(address);
    }

    cpu_instr_transfer(operand, reg_out, &_state.flags);
}

static void store(struct instruction *instr,
                  uint8_t reg)
{
    uint16_t address;

    address = get_address_from_mode(instr);
    _mem_set(address, reg);
}

static void and(struct instruction *instr)
{
    uint8_t          operand = 0;
    struct cpu_state *state = &_state;
    uint16_t         address;

    if (instr->operation->mode == Immediate) {
        operand = instr->operands[0];
    }
    else {
        address = get_address_from_mode(instr);
        operand = _mem_get(address);
    }

    cpu_instr_and(&state->reg_a, operand, &state->flags);
}

static void or(struct instruction *instr)
{
    uint8_t          operand = 0;
    struct cpu_state *state = &_state;
    uint16_t         address;

    if (instr->operation->mode == Immediate) {
        operand = instr->operands[0];
    }
    else {
        address = get_address_from_mode(instr);
        operand = _mem_get(address);
    }

    cpu_instr_or(&state->reg_a, operand, &state->flags);
}

static void xor(struct instruction *instr)
{
    uint8_t          operand = 0;
    struct cpu_state *state = &_state;
    uint16_t         address;

    if (instr->operation->mode == Immediate) {
        operand = instr->operands[0];
    }
    else {
        address = get_address_from_mode(instr);
        operand = _mem_get(address);
    }

    cpu_instr_xor(&state->reg_a, operand, &state->flags);
}

static void asl(struct instruction *instr)
{
    uint8_t  operand;
    uint16_t address;
    uint8_t  shifted;

    if (instr->operation->mode == Accumulator) {
        operand = _state.reg_a;
    }
    else {
        address = get_address_from_mode(instr);
        operand = _mem_get(address);
    }

    cpu_instr_asl(operand, &shifted, &_state.flags);

    if (instr->operation->mode == Accumulator) {
        _state.reg_a = shifted;
    }
    else {
        _mem_set(address, shifted);
    }
}

static void lsr( struct instruction *instr)
{
    uint8_t operand;
    uint8_t shifted;
    uint16_t address;

    if (instr->operation->mode == Accumulator) {
        operand = _state.reg_a;
    }
    else {
        address = get_address_from_mode(instr);
        operand = _mem_get(address);
    }

    cpu_instr_lsr(operand, &shifted, &_state.flags);

    if (instr->operation->mode == Accumulator) {
        _state.reg_a = shifted;
    }
    else {
        _mem_set(address, shifted);
    }
}

static void rol(
                struct instruction *instr)
{
    uint8_t  operand;
    uint16_t address;
    uint8_t  shifted;

    if (instr->operation->mode == Accumulator) {
        operand = _state.reg_a;
    }
    else {
        address = get_address_from_mode(instr);
        operand = _mem_get(address);
    }

    cpu_instr_rol(operand, &shifted, &_state.flags);

    if (instr->operation->mode == Accumulator) {
        _state.reg_a = shifted;
    }
    else {
        _mem_set(address, shifted);
    }
}

static void ror(struct instruction *instr)
{
    uint8_t  operand;
    uint16_t address;
    uint8_t  shifted;

    if (instr->operation->mode == Accumulator) {
        operand = _state.reg_a;
    }
    else {
        address = get_address_from_mode(instr);
        operand = _mem_get(address);
    }

    cpu_instr_ror(operand, &shifted, &_state.flags);

    if (instr->operation->mode == Accumulator) {
        _state.reg_a = shifted;
    }
    else {
        _mem_set(address, shifted);
    }
}

static void inc_dec(struct instruction *instr,
                    int8_t delta)
{
    uint8_t  operand;
    uint16_t address;
    uint8_t  increased;

    address = get_address_from_mode(instr);
    operand = _mem_get(address);

    cpu_instr_inc_dec(operand, delta, &increased, &_state.flags);

    _mem_set(address, increased);
}

static void bit(struct instruction *instr)
{
    uint8_t  operand;
    uint16_t address;

    address = get_address_from_mode(instr);
    operand = _mem_get(address);

    cpu_instr_bit(operand, _state.reg_a, &_state.flags);
}

static void add(struct instruction *instr)
{
    uint8_t          operand = 0;
    uint16_t         address;

    if (instr->operation->mode == Immediate) {
        operand = instr->operands[0];
    }
    else {
        address = get_address_from_mode(instr);
        operand = _mem_get(address);
    }

    if (_state.flags & FLAG_DECIMAL_MODE) {
        cpu_instr_add_decimal(_state.reg_a, operand,
                              &_state.reg_a, &_state.flags);
    }
    else {
        cpu_instr_add(_state.reg_a, operand,
                      &_state.reg_a, &_state.flags);
    }
}

static void subtract(struct instruction *instr)
{
    uint8_t          operand = 0;
    struct cpu_state *state = &_state;
    uint16_t         address;

    if (instr->operation->mode == Immediate) {
        operand = instr->operands[0];
    }
    else {
        address = get_address_from_mode(instr);
        operand = _mem_get(address);
    }

    if (state->flags & FLAG_DECIMAL_MODE) 
        printf("Decimal mode not supported on sub\n");
    else {
        cpu_instr_sub(state->reg_a, operand,
                      &state->reg_a, &state->flags);
    }
}

static void compare(struct instruction *instr,
                   uint8_t compare_to)
{
    uint8_t          operand = 0;
    struct cpu_state *state = &_state;
    uint16_t         address;

    if (instr->operation->mode == Immediate) {
        operand = instr->operands[0];
    }
    else {
        address = get_address_from_mode(instr);
        operand = _mem_get(address);
    }

    cpu_instr_compare(compare_to, operand, &state->flags);
}

static void branch(struct instruction *instr,
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
            _state.pc -= 0x100 - offset;
        }
        else {
            _state.pc += offset;
        }
        break;
    default:
        printf("Unhandled address mode\n");
        break;
    }
}

static void jump(struct instruction *instr)
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
        lo = _mem_get(address);
        address++;
        if (page != get_page(address)) {
            /* Illegal case, indirect jump with vector starting
             * on the last byte of a page. */
            address = get_page_address(page);
        }
        hi = _mem_get(address);
        break;
    default:
        printf("Unhandled address mode\n");
        break;
    }

    address = make_address(hi, lo);
    _state.pc = address;
}

static void jump_to_subroutine(struct instruction *instr)
{
    uint8_t  *ops    = instr->operands;
    uint16_t address = make_address(ops[1], ops[0]);

    stack_push_address(_state.pc);
    _state.pc = address;
}

static void return_from_interrupt()
{
    _state.flags = stack_pop();
    _state.pc = stack_pop_address();
}

static int execute(struct instruction *instr)
{
    struct cpu_state *state = &_state;

    switch (instr->operation->mnem) {

    /* Interrupt instructions */
    case BRK:
        /* Signals that IRQ is due to break */
        set_flag(state, FLAG_BRK);
        /* Exception on how program counter is counted */
        state->pc++;
        _irq_pending = true;
        break;
    case RTI:
        return_from_interrupt();
        break;

    /* Stack instructions */
    case PHA:
        stack_push(state->reg_a);
        break;
    case PHP:
        stack_push(state->flags);
        break;
    case PLA:
        cpu_instr_transfer(stack_pop(),
                           &state->reg_a,
                           &state->flags);
        break;
    case PLP:
        state->flags = stack_pop();
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
        load(instr, &_state.reg_a);
        break;
    case LDX:
        load(instr, &_state.reg_x);
        break;
    case LDY:
        load(instr, &_state.reg_y);
        break;

    /* Store instructions */
    case STA:
        store(instr, _state.reg_a);
        break;
    case STX:
        store(instr, _state.reg_x);
        break;
    case STY:
        store(instr, _state.reg_y);
        break;

    /* ALU instructions */
    case ADC:
        add(instr);
        break;
    case SBC:
        subtract(instr);
        break;
    case AND:
        and(instr);
        break;
    case ORA:
        or(instr);
        break;
    case EOR:
        xor(instr);
        break;
    case ASL:
        asl(instr);
        break;
    case ROL:
        rol(instr);
        break;
    case LSR:
        lsr(instr);
        break;
    case ROR:
        ror(instr);
        break;
    case BIT:
        bit(instr);
        break;
    case CMP:
        compare(instr, _state.reg_a);
        break;
    case CPX:
        compare(instr, _state.reg_x);
        break;
    case CPY:
        compare(instr, _state.reg_y);
        break;
    case INC:
        inc_dec(instr, 1);
        break;
    case DEC:
        inc_dec(instr, -1);
        break;
    case INX:
        cpu_instr_inc_dec(_state.reg_x, 1,
                          &_state.reg_x, &_state.flags);
        break;
    case INY:
        cpu_instr_inc_dec(_state.reg_y, 1,
                          &_state.reg_y, &_state.flags);
        break;
    case DEX:
        cpu_instr_inc_dec(_state.reg_x, -1,
                          &_state.reg_x, &_state.flags);
        break;
    case DEY:
        cpu_instr_inc_dec(_state.reg_y, -1,
                          &_state.reg_y, &_state.flags);
        break;

    /* Branch instructions */
    case BEQ:
        branch(instr, _state.flags & FLAG_ZERO);
        break;
    case BNE:
        branch(instr, (_state.flags & FLAG_ZERO) == 0);
        break;
    case BPL:
        branch(instr, (_state.flags & FLAG_NEGATIVE) == 0);
        break;
    case BMI:
        branch(instr, _state.flags & FLAG_NEGATIVE);
        break;
    case BVC:
        branch(instr, (_state.flags & FLAG_OVERFLOW) == 0);
        break;
    case BVS:
        branch(instr, _state.flags & FLAG_OVERFLOW);
        break;
    case BCC:
        branch(instr, (_state.flags & FLAG_CARRY) == 0);
        break;
    case BCS:
        branch(instr, _state.flags & FLAG_CARRY);
        break;

    /* Jump instructions */
    case JMP:
        jump(instr);
        break;
    case JSR:
        jump_to_subroutine(instr);
        break;
    case RTS:
        _state.pc = stack_pop_address();
        break;

    /* Status register instuctions */
    case CLC:
        clear_flag(&_state, FLAG_CARRY);
        break;
    case SEC:
        set_flag(&_state, FLAG_CARRY);
        break;
    case CLI:
        clear_flag(&_state, FLAG_IRQ_DISABLE);
        break;
    case SEI:
        set_flag(&_state, FLAG_IRQ_DISABLE);
        break;
    case SED:
        set_flag(&_state, FLAG_DECIMAL_MODE);
        break;
    case CLD:
        clear_flag(&_state, FLAG_DECIMAL_MODE);
        break;
    case CLV:
        clear_flag(&_state, FLAG_OVERFLOW);
        break;

    /* Other */
    case NOP:
        break;

    default:
        printf("Unknown mnem: %s\n",
               mnemonics_strings[instr->operation->mnem]);
    }

    /* Writes instruction and registers to debug fd */
    trace_execution(_trace_fd, instr,
                    &_state_before, &_state);

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

static int fetch_and_decode(struct instruction *instr)
{
    uint8_t      op_code;
    int          num_operands;
    uint8_t      *operands = instr->operands;

    op_code = _mem_get(_state.pc++);
    instr->operation = &opcodes[op_code];

    num_operands = get_num_operands(instr->operation->mode);
    switch (num_operands) {
    case 0:
        break;
    case 1:
        operands[0] = _mem_get(_state.pc++);
        break;
    case 2:
        operands[0] = _mem_get(_state.pc++);
        operands[1] = _mem_get(_state.pc++);
        break;
    default:
        printf("Unknown number of operands for mode: %d\n",
               instr->operation->mode);
        break;
    }

    return 0;
}

void cpu_init(cpu_mem_get mem_get,
              cpu_mem_set mem_set,
              int trace_fd)
{
    _mem_get = mem_get;
    _mem_set = mem_set;
    _trace_fd = trace_fd;
    memset(&_state, 0, sizeof(_state));
    _irq_pending = false;
    _stack_overflow = false;
    _stack_underflow = false;
}

void cpu_poweron()
{
    /* Empty stack */
    _state.sp = 0xff;
}

void cpu_set_state(struct cpu_state *state)
{
    _state = *state;
}

void cpu_step(struct cpu_state *state_out)
{
    struct instruction instr;

    if (_irq_pending) {
        interrupt_request();
        _irq_pending = false;
    }

    /* Copy for debugging purposes. */
    _state_before = _state;

    fetch_and_decode(&instr);
    execute(&instr);

    if (state_out) {
        *state_out = _state;
    }
}

static void get_instruction(uint16_t address,
                            struct instruction *instr,
                            uint8_t *offset)
{
    uint8_t op_code;
    int     num_operands;
    uint8_t *operands = instr->operands;

    op_code = _mem_get(address++);
    instr->operation = &opcodes[op_code];

    num_operands = get_num_operands(instr->operation->mode);
    if (num_operands > 0) {
        operands[0] = _mem_get(address++);
    }
    if (num_operands > 1) {
        operands[1] = _mem_get(address++);
    }
    *offset = num_operands + 1;
}

void cpu_disassembly_at(int fd,
                        uint16_t address,
                        int num_instructions)
{
    struct instruction instr;
    uint8_t offset;

    while (num_instructions--) {
        get_instruction(address, &instr, &offset);
        trace_instruction(fd, address, &instr, 0);
        address += offset;
        write(fd, "\n", 1);
    }
}

