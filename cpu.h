#include <stdint.h>


/* Processor status flags */
#define FLAG_CARRY        0x01
#define FLAG_ZERO         0x02
#define FLAG_IRQ_DISABLE  0x04
#define FLAG_DECIMAL_MODE 0x08
#define FLAG_BRK          0x10
#define FLAG_OVERFLOW     0x40
#define FLAG_NEGATIVE     0x80

struct cpu_state {
    uint16_t pc;
    uint8_t  reg_a;
    uint8_t  reg_x;
    uint8_t  reg_y;
    uint8_t  flags;
    uint8_t  sp;
};

typedef uint8_t (*cpu_mem_get)(uint16_t addr);
typedef void (*cpu_mem_set)(uint16_t addr, uint8_t val);

void cpu_init(cpu_mem_get mem_get,
              cpu_mem_set mem_set,
              int trace_fd);

void cpu_poweron();
void cpu_step(struct cpu_state *state_out);

/* For interactive use */
void cpu_disassembly_at(int fd,
                        uint16_t address,
                        int num_instructions);

/* For debug */
void cpu_set_state(struct cpu_state *state);
