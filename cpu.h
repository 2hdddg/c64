#include "mem.h"

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

int cpu_create(struct mem_h *mem,
               int execution_fd,
               struct cpu_h **cpu);
void cpu_destroy(struct cpu_h *cpu);

int cpu_poweron(struct cpu_h *cpu);
int cpu_step(struct cpu_h *cpu,
             struct cpu_state *state_out);

/* For debug */
int cpu_set_state(struct cpu_h *cpu, struct cpu_state *state);
