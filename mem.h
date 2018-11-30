#include <stdint.h>


int mem_reset();

/* CPU memory API */
uint8_t mem_get_for_cpu(uint16_t addr);
void mem_set_for_cpu(uint16_t addr,
                     uint8_t val);

/* VIC memory API */
uint8_t mem_get_for_vic(uint16_t addr);
void mem_set_for_vic(uint16_t addr,
                     uint8_t val);

