#pragma once

#include <stdint.h>


#define KEY_PLUS            0x05fe

#define KEY_W               0x01fd

#define KEY_A               0x01fb

#define KEY_4               0x01f7

#define KEY_B               0x03ef

#define KEY_F3              0x00df
#define KEY_S               0x01df
#define KEY_F               0x02df
#define KEY_H               0x03df
#define KEY_K               0x04df
#define KEY_BRACKET_LEFT_SQ 0x05df
#define KEY_COLON           0x05df
#define KEY_EQ              0x06df
#define KEY_COMMODORE       0x07df

#define KEY_E               0x01bf

#define KEY_LEFT_SHIFT      0x017f
#define KEY_X               0x027f

void keyboard_reset();
void keyboard_down(uint16_t key);
void keyboard_up(uint16_t key);

uint8_t keyboard_get_port_A();
uint8_t keyboard_get_port_B();

void keyboard_set_port_A(uint8_t lines);
void keyboard_set_port_B(uint8_t lines);
