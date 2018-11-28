#include <stdint.h>

struct mem_h;

int mem_create(struct mem_h **mem);
int mem_reset(struct mem_h *mem);

/* Read memory from CPU perspective */
uint8_t mem_get_cpu(struct mem_h *mem, uint16_t addr);
/* Set memory from CPU */
void mem_set_cpu(struct mem_h *mem, uint16_t addr, uint8_t val);

void mem_destroy(struct mem_h *mem);

