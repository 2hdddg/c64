#pragma once
#include <stdint.h>
#include <stdbool.h>

/* Utilities for implementing a commandline command */

bool cmd_parse_address(char *token, uint16_t *address);

bool cmd_parse_uint16(char *token, uint16_t *num);
bool cmd_parse_uint8(char *token, uint8_t *num);

uint16_t cmd_dump_mem(uint8_t *addr, uint16_t vaddr, int lines, int width);
