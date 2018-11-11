#include <stdint.h>

struct mem_h;

int mem_create(struct mem_h **mem_h);
int mem_reset(struct mem_h *mem_h);
void mem_set(struct mem_h *mem_h, uint16_t addr, uint8_t val);
uint8_t mem_get(struct mem_h *mem_h, uint16_t addr);
void mem_destroy(struct mem_h *mem_h);

