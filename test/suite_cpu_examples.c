#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

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
    memset(&_state, 0, sizeof(_state));
    return cpu_create(_mem, STDOUT_FILENO, &_cpu) == 0;
}

int each_after()
{
    cpu_destroy(_cpu);
    mem_destroy(_mem);
    return 0;
}

int test_add_with_carry_big_numbers()
{
    /* Sums combination of 16 bit numbers.
     *
     */
    const uint8_t  program[] = {
        /* Input numbers at 0x5000,
         * Result at 0x5100 */

        /* LDA $5000 */ 0xad, 0x00, 0x50,
        /* CLC       */ 0x18,
        /* ADC $5001 */ 0x6d, 0x01, 0x50,
        /* STA $5100 */ 0x8d, 0x00, 0x51,

        /* LDA $5002 */ 0xad, 0x02, 0x50,
        /* ADC $5003 */ 0x6d, 0x03, 0x50,
        /* STA $5101 */ 0x8d, 0x01, 0x51,
    };
    const uint8_t num_program_steps = 7;
    memcpy(_ram + CODE, program, sizeof(program));
    const uint16_t numbers[] = {
        0, 100, 65535, 78, 290, 1100, 1001, 7321,
    };

    int n = 0;
    for (int i = 0; i < sizeof(numbers); i++) {
        uint16_t op1 = numbers[i];
        _ram[0x5000] = op1 & 0xff;
        _ram[0x5002] = op1 >> 8;
        for (int j = i + 1; j < sizeof(numbers); j++) {
            uint16_t op2 = numbers[j];
            _ram[0x5001] = op2 & 0xff;
            _ram[0x5003] = op2 >> 8;
            _ram[0x5100] = 0x00;
            _ram[0x5101] = 0x00;
            _state.pc = CODE;
            _state.sp = 0xff;
            cpu_set_state(_cpu, &_state);
            for (int s = 0; s < num_program_steps; s++) {
                cpu_step(_cpu, NULL);
            }
            uint16_t res = (_ram[0x5100] & 0xff) | (_ram[0x5101] << 8);
            uint16_t real_res = (op1 + op2) & 0xffff;
            if (res != real_res) {
                printf("%d, %04x + %04x => %04x "
                       "should be %04x\n",
                       n, op1, op2, res, real_res);
                return 0;
            }
            printf("\n");
            n++;
        }
    }

    return 1;
}

int test_subtract_with_carry_big_numbers()
{
    /* Subtracts combination of 16 bit numbers.
     *
     */
    const uint8_t  program[] = {
        /* Input numbers at 0x5000,
         * Result at 0x5100 */

        /* LDA $5000 */ 0xad, 0x00, 0x50,
        /* SEC       */ 0x38,
        /* SBC $5001 */ 0xed, 0x01, 0x50,
        /* STA $5100 */ 0x8d, 0x00, 0x51,

        /* LDA $5002 */ 0xad, 0x02, 0x50,
        /* SBC $5003 */ 0xed, 0x03, 0x50,
        /* STA $5101 */ 0x8d, 0x01, 0x51,
    };
    const uint8_t num_program_steps = 7;
    memcpy(_ram + CODE, program, sizeof(program));
    const uint16_t numbers[] = {
        0, 100, 65535, 78, 290, 1100, 1001, 7321,
    };

    int n = 0;
    for (int i = 0; i < sizeof(numbers); i++) {
        uint16_t op1 = numbers[i];
        _ram[0x5000] = op1 & 0xff;
        _ram[0x5002] = op1 >> 8;
        for (int j = i + 1; j < sizeof(numbers); j++) {
            uint16_t op2 = numbers[j];
            _ram[0x5001] = op2 & 0xff;
            _ram[0x5003] = op2 >> 8;
            _ram[0x5100] = 0x00;
            _ram[0x5101] = 0x00;
            _state.pc = CODE;
            _state.sp = 0xff;
            cpu_set_state(_cpu, &_state);
            for (int s = 0; s < num_program_steps; s++) {
                cpu_step(_cpu, NULL);
            }
            uint16_t res = (_ram[0x5100] & 0xff) | (_ram[0x5101] << 8);
            uint16_t real_res = (op1 - op2);
            if (res != real_res) {
                printf("%d, %04x + %04x => %04x "
                       "should be %04x\n",
                       n, op1, op2, res, real_res);
                return 0;
            }
            n++;
        }
    }

    return 1;
}