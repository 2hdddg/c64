#pragma once

#include <stdint.h>


#define KEYB_INST            0x00fe
#define KEYB_DEL             0x00fe
#define KEYB_NUMBER          0x01fe
#define KEYB_3               0x01fe
#define KEYB_PERCENT         0x02fe
#define KEYB_5               0x02fe
#define KEYB_APOSTROPHE      0x03fe
#define KEYB_7               0x03fe
#define KEYB_PARAN_CLOSE     0x04fe
#define KEYB_9               0x04fe
#define KEYB_PLUS            0x05fe
#define KEYB_POUND           0x06fe
#define KEYB_EXCLAMATION     0x07fe
#define KEYB_1               0x07fe

#define KEYB_RETURN          0x00fd
#define KEYB_W               0x01fd
#define KEYB_R               0x02fd
#define KEYB_Y               0x03fd
#define KEYB_I               0x04fd
#define KEYB_P               0x05fd
#define KEYB_STAR            0x06fd
#define KEYB_LEFT            0x07fd

#define KEYB_CRSR_           0x00fb
#define KEYB_A               0x01fb
#define KEYB_D               0x02fb
#define KEYB_G               0x03fb
#define KEYB_J               0x04fb
#define KEYB_L               0x05fb
#define KEYB_BRACK_CLOSE_SQ  0x06fb
#define KEYB_SEMICOLON       0x06fb
#define KEYB_CTRL            0x07fb

#define KEYB_F7              0x00f7
#define KEYB_DOLLAR          0x01f7
#define KEYB_4               0x01f7
#define KEYB_AMP             0x02f7
#define KEYB_6               0x02f7
#define KEYB_PARAN_OPEN      0x03f7
#define KEYB_8               0x03f7
#define KEYB_0               0x04f7
#define KEYB_MINUS           0x05f7
#define KEYB_CLR             0x06f7
#define KEYB_HOME            0x06f7
#define KEYB_QUOTE           0x07f7
#define KEYB_2               0x07f7

#define KEYB_F1              0x00ef
#define KEYB_Z               0x01ef
#define KEYB_C               0x02ef
#define KEYB_B               0x03ef
#define KEYB_M               0x04ef
#define KEYB_GT              0x05ef
#define KEYB_DOT             0x05ef
#define KEYB_RIGHT_SHIFT     0x06ef
#define KEYB_SPACE           0x07ef

#define KEYB_F3              0x00df
#define KEYB_S               0x01df
#define KEYB_F               0x02df
#define KEYB_H               0x03df
#define KEYB_K               0x04df
#define KEYB_BRACK_OPEN_SQ   0x05df
#define KEYB_COLON           0x05df
#define KEYB_EQ              0x06df
#define KEYB_COMMODORE       0x07df

#define KEYB_F5              0x00bf
#define KEYB_E               0x01bf
#define KEYB_T               0x02bf
#define KEYB_U               0x03bf
#define KEYB_O               0x04bf
#define KEYB_AT              0x05bf
#define KEYB_CARET           0x06bf
#define KEYB_Q               0x07bf

#define KEYB_CRSR            0x007f
#define KEYB_LEFT_SHIFT      0x017f
#define KEYB_SHIFT_LOCK      0x017f
#define KEYB_X               0x027f
#define KEYB_V               0x037f
#define KEYB_N               0x047f
#define KEYB_LT              0x057f
#define KEYB_COMMA           0x057f
#define KEYB_QUESTION_MARK   0x067f
#define KEYB_FORWARD_SLASH   0x067f
#define KEYB_STOP            0x077f

void keyboard_init();
void keyboard_reset();
void keyboard_down(uint16_t key);
void keyboard_up(uint16_t key);

uint8_t keyboard_get_port_A(uint8_t interesting_bits);
uint8_t keyboard_get_port_B(uint8_t interesting_bits);

void keyboard_set_port_A(uint8_t lines, uint8_t valid_lines);
void keyboard_set_port_B(uint8_t lines, uint8_t valid_lines);

void keyboard_trace_keys(int fd);
void keyboard_trace_port_set(int fd);
void keyboard_trace_port_get(int fd);

