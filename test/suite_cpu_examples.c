#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "cpu.h"

#define RAM_SIZE 65536
#define CODE 0x1000

char _ram[RAM_SIZE];

struct cpu_h     *_cpu;
struct cpu_state _state;

void mem_set(void *m, uint16_t addr, uint8_t val)
{
    _ram[addr] = val;
}

uint8_t mem_get(void *m, uint16_t addr)
{
    return _ram[addr];
}

int each_before()
{
    memset(_ram, 0, RAM_SIZE);
    memset(&_state, 0, sizeof(_state));
    return cpu_create(STDOUT_FILENO, mem_get, mem_set, NULL, &_cpu) == 0;
}

int each_after()
{
    cpu_destroy(_cpu);
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
    const int num_numbers = sizeof(numbers)/sizeof(numbers[0]);

    int n = 0;
    for (int i = 0; i < num_numbers - 1; i++) {
        uint16_t op1 = numbers[i];
        _ram[0x5000] = op1 & 0xff;
        _ram[0x5002] = op1 >> 8;
        for (int j = i + 1; j < num_numbers; j++) {
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

int test_subtract_with_carry_big_positive_numbers()
{
    int failures = 0;

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
        65535, 30431, 15999, 9000, 2132, 1024, 500, 319, 50, 30, 7,
    };
    const int num_numbers = sizeof(numbers)/sizeof(numbers[0]);

    int n = 0;
    for (int i = 0; i < num_numbers - 1; i++) {
        uint16_t op1 = numbers[i];
        _ram[0x5000] = op1 & 0xff;
        _ram[0x5002] = op1 >> 8;
        for (int j = i + 1; j < num_numbers; j++) {
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
                printf("%d, %04x - %04x => %04x "
                       "should be %04x\n",
                       n, op1, op2, res, real_res);
                failures++;
            }
            n++;
        }
    }

    printf("Failed %d out of %d\n", failures, n);

    return failures==0;
}

int test_double_integer_of_four_bytes()
{
    const uint8_t  program[] = {
        /* Input numbers at 0x5000,
         * Result at 0x5100 */

        /* ASL $5000 */ 0x0e, 0x00, 0x50,
        /* ROL $5001 */ 0x2e, 0x01, 0x50,
        /* ROL $5002 */ 0x2e, 0x02, 0x50,
        /* ROL $5003 */ 0x2e, 0x03, 0x50,
    };
    const uint32_t val = 0x00818181;
    const uint8_t num_program_steps = 4;
    uint32_t doubled = 0;

    _ram[0x5000] = val & 0xff;
    _ram[0x5001] = (val >> 8) & 0xff;
    _ram[0x5002] = (val >> 16) & 0xff;
    _ram[0x5003] = (val >> 24) & 0xff;
    memcpy(_ram + CODE, program, sizeof(program));
    _state.pc = CODE;
    _state.sp = 0xff;
    _state.flags = 0x00;
    cpu_set_state(_cpu, &_state);
    for (int s = 0; s < num_program_steps; s++) {
        cpu_step(_cpu, NULL);
    }

    doubled  = (uint8_t)_ram[0x5003] << 24;
    doubled |= (uint8_t)_ram[0x5002] << 16;
    doubled |= (uint8_t)_ram[0x5001] << 8;
    doubled |= (uint8_t)_ram[0x5000];

    if (doubled != val * 2) {
        printf("Failed to double %04x, got %04x\n should be %04x",
               val, doubled, val * 2);
        return 0;
    }
    return 1;
}

int test_half_integer_of_four_bytes()
{
    const uint8_t  program[] = {
        /* Input numbers at 0x5000,
         * Result at 0x5100 */

        /* LSR $5003 */ 0x4e, 0x03, 0x50,
        /* ROR $5002 */ 0x6e, 0x02, 0x50,
        /* ROR $5001 */ 0x6e, 0x01, 0x50,
        /* ROR $5000 */ 0x6e, 0x00, 0x50,
    };
    const uint32_t val = 0x02814121;
    const uint8_t num_program_steps = 4;
    uint32_t halfed = 0;

    _ram[0x5000] = val & 0xff;
    _ram[0x5001] = (val >> 8) & 0xff;
    _ram[0x5002] = (val >> 16) & 0xff;
    _ram[0x5003] = (val >> 24) & 0xff;
    memcpy(_ram + CODE, program, sizeof(program));
    _state.pc = CODE;
    _state.sp = 0xff;
    _state.flags = 0x00;
    cpu_set_state(_cpu, &_state);
    for (int s = 0; s < num_program_steps; s++) {
        cpu_step(_cpu, NULL);
    }

    halfed  = (uint8_t)_ram[0x5003] << 24;
    halfed |= (uint8_t)_ram[0x5002] << 16;
    halfed |= (uint8_t)_ram[0x5001] << 8;
    halfed |= (uint8_t)_ram[0x5000];

    if (halfed != val / 2) {
        printf("Failed to halfe %04x, got %04x should be %04x\n",
               val, halfed, val / 2);
        return 0;
    }
    return 1;
}
