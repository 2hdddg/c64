/* Implementation of 6510 integrated I/O port.
 * Accessed through address 0x00 & 0x01 by the CPU.
 * Controls how the CPU accesses memory through banks.
 * */

#pragma once

#define CPU_PORT_DIR_IN  0
#define CPU_PORT_DIR_OUT 1

/* Name and bit value of lines */
#define CPU_PORT_LORAM  0x01
#define CPU_PORT_HIRAM  0x02
#define CPU_PORT_CHAREN 0x04
#define CPU_PORT_CASSETTE_WRITE 0x08
#define CPU_PORT_CASSETTE_SENSE 0x10
#define CPU_PORT_CASSETTE_MOTOR 0x20

void cpu_port_init();

//void cpu_port_set_cassette_sense(bool play_pressed);

