
#include "cpu_instr.h"
#include "cpu.h"

static inline uint8_t set_flags(uint8_t names,
                                uint8_t values,
                                uint8_t current)
{
    current &= ~names;
    current |= values;

    return current;
}

static inline void eval_zero_and_neg(uint8_t val,
                                     uint8_t *flags)
{
    uint8_t eval;

    eval = val == 0 ? FLAG_ZERO : 0;
    eval |= val & 0x80 ? FLAG_NEGATIVE : 0;
    *flags= set_flags(FLAG_ZERO|FLAG_NEGATIVE,
                      eval, *flags);
}

static inline void eval_overflow(uint8_t op1,
                                 uint8_t op2,
                                 uint8_t res,
                                 uint8_t *flags)
{
    uint8_t overflow;

    /* Only care about sign bits */
    op1 &= 0x80;
    op2 &= 0x80;
    res &= 0x80;

    /* Check if sign change */
    overflow = op1 == op2 && op1 != res ? FLAG_OVERFLOW : 0;
    *flags= set_flags(FLAG_OVERFLOW, overflow, *flags);
}

void cpu_instr_transfer(uint8_t from,
                        uint8_t *to_out,
                        uint8_t *flags)
{
    eval_zero_and_neg(from, flags);
    *to_out = from;
}

void cpu_instr_and(uint8_t *op1,
                   uint8_t op2,
                   uint8_t *flags)
{
    *op1 &= op2;
    eval_zero_and_neg(*op1, flags);
}

void cpu_instr_or(uint8_t *op1,
                  uint8_t op2,
                  uint8_t *flags)
{
    *op1 |= op2;
    eval_zero_and_neg(*op1, flags);
}

void cpu_instr_asl(uint8_t operand,
                   uint8_t *shifted,
                   uint8_t *flags)
{
    uint8_t res;
    uint8_t carry_out;

    res = operand << 1;
    carry_out = operand & 0x80 ? FLAG_CARRY : 0;
    *flags = set_flags(FLAG_CARRY, carry_out,
                           *flags);
    eval_zero_and_neg(res, flags);
    *shifted = res;
}

void cpu_instr_lsr(uint8_t operand,
                   uint8_t *shifted,
                   uint8_t *flags)
{
    uint8_t res;
    uint8_t carry_out;

    res = operand >> 1;
    carry_out = operand & 0x01 ? FLAG_CARRY : 0;
    *flags = set_flags(FLAG_CARRY, carry_out,
                           *flags);
    eval_zero_and_neg(res, flags);
    *shifted = res;
}

void cpu_instr_rol(uint8_t operand,
                   uint8_t *shifted,
                   uint8_t *flags)
{
    uint8_t res;
    uint8_t carry_in;
    uint8_t carry_out;

    carry_in = (*flags) & FLAG_CARRY ? 1 : 0;
    carry_out = operand & 0x80 ? FLAG_CARRY : 0;

    res = (operand << 1) | carry_in;

    *flags = set_flags(FLAG_CARRY, carry_out, *flags);
    eval_zero_and_neg(res, flags);
    *shifted = res;
}

void cpu_instr_ror(uint8_t operand,
                   uint8_t *shifted,
                   uint8_t *flags)
{
    uint8_t res;
    uint8_t carry_in;
    uint8_t carry_out;

    carry_in = (*flags) & FLAG_CARRY ? 0x80 : 0;
    carry_out = operand & 0x01 ? FLAG_CARRY : 0;

    res = (operand >> 1) | carry_in;

    *flags = set_flags(FLAG_CARRY, carry_out, *flags);
    eval_zero_and_neg(res, flags);
    *shifted = res;
}

void cpu_instr_inc_dec(uint8_t operand,
                       int8_t  delta,
                       uint8_t *increased,
                       uint8_t *flags)
{
    uint8_t res;

    res = operand + delta;
    eval_zero_and_neg(res, flags);
    *increased = res;
}

void cpu_instr_bit(uint8_t operand,
                   uint8_t other,
                   uint8_t *flags)
{
    uint8_t eval;

    eval  = operand & 0x80 ? FLAG_NEGATIVE : 0;
    eval |= operand & 0x40 ? FLAG_OVERFLOW : 0;
    eval |= (operand & other) == 0 ? FLAG_ZERO : 0;

    *flags = set_flags(FLAG_NEGATIVE|FLAG_OVERFLOW|FLAG_ZERO,
                       eval, *flags);
}

void cpu_instr_add(uint8_t op1,
                   uint8_t op2,
                   uint8_t *added,
                   uint8_t *flags)
{
    uint8_t  carry_in;
    uint8_t  carry_out;
    uint16_t untruncated;
    uint8_t  result;

    carry_in = (*flags) & FLAG_CARRY ? 1 : 0;
    untruncated = op1 + op2 + carry_in;
    carry_out = untruncated > 0xff ? FLAG_CARRY : 0;
    result = untruncated & 0xff;

    *flags = set_flags(FLAG_CARRY, carry_out, *flags);
    eval_zero_and_neg(result, flags);
    eval_overflow(op1, op2, result, flags);

    *added = result;
}

static void diff(uint8_t op1,
                 uint8_t op2,
                 uint8_t borrow,
                 uint8_t *diffed,
                 uint8_t *flags)
{
    uint8_t two_compl;
    uint8_t result;
    uint8_t eval;

    two_compl = ~op2 + borrow;
    result    = (op1 + two_compl) & 0xff;

    eval  = op1 >= op2 ? FLAG_CARRY : 0;
    eval |= result == 0 ? FLAG_ZERO : 0;
    eval |= result & 0x80 ? FLAG_NEGATIVE : 0;
    *flags = set_flags(FLAG_CARRY|FLAG_ZERO|FLAG_NEGATIVE ,
                       eval, *flags);

    *diffed = result;
}


void cpu_instr_sub(uint8_t op1,
                   uint8_t op2,
                   uint8_t *subtracted,
                   uint8_t *flags)
{
    uint8_t result;
    uint8_t borrow;

    borrow = *flags & FLAG_CARRY ? 1 : 0;
    diff(op1, op2, borrow, &result, flags);
    eval_overflow(op1, op2, result, flags);

    *subtracted = result;
}

void cpu_instr_compare(uint8_t op1,
                       uint8_t op2,
                       uint8_t *flags)
{
    uint8_t result;

    diff(op1, op2, 1, &result, flags);
}


