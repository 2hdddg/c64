#pragma once

#include <stdint.h>
#include <stdbool.h>

void cpu_instr_transfer(uint8_t from,
                        uint8_t *to_out,
                        uint8_t *flags_out);
void cpu_instr_and(uint8_t *op1,
                   uint8_t op2,
                   uint8_t *flags_out);
void cpu_instr_asl(uint8_t operand,
                   uint8_t *shift_out,
                   uint8_t *flags_out);
void cpu_instr_lsr(uint8_t operand,
                   uint8_t *shift_out,
                   uint8_t *flags_out);
void cpu_instr_rol(uint8_t operand,
                   uint8_t *shift_out,
                   uint8_t *flags_out);
void cpu_instr_ror(uint8_t operand,
                   uint8_t *shift_out,
                   uint8_t *flags_out);
void cpu_instr_bit(uint8_t operand,
                   uint8_t other,
                   uint8_t *flags);
void cpu_instr_add(uint8_t op1,
                   uint8_t op2,
                   uint8_t *added,
                   uint8_t *flags);
void cpu_instr_sub(uint8_t op1,
                   uint8_t op2,
                   uint8_t *subtracted,
                   uint8_t *flags);
void cpu_instr_compare(uint8_t op1,
                       uint8_t op2,
                       uint8_t *flags);
void cpu_instr_inc_dec(uint8_t operand,
                       int8_t  delta,
                       uint8_t *increased,
                       uint8_t *flags);
