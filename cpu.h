#include <stdint.h>

struct cpu_h;

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

typedef uint8_t (*cpu_mem_get)(void*, uint16_t addr);
typedef void (*cpu_mem_set)(void*, uint16_t addr, uint8_t val);

int cpu_create(int execution_fd,
               cpu_mem_get mem_get,
               cpu_mem_set mem_set,
               void *mem_handle,
               struct cpu_h **cpu);
void cpu_destroy(struct cpu_h *cpu);

int cpu_poweron(struct cpu_h *cpu);
int cpu_step(struct cpu_h *cpu,
             struct cpu_state *state_out);

/* For interactive use */
void cpu_disassembly_at(struct cpu_h *cpu,
                        int fd,
                        uint16_t address,
                        int num_instructions);

/* For debug */
int cpu_set_state(struct cpu_h *cpu, struct cpu_state *state);
