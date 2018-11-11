#include "mem.h"

struct cpu_h;

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
int cpu_set_pc(struct cpu_h *cpu, uint16_t pc);
int cpu_step(struct cpu_h *cpu,
             struct cpu_state *state_out);
