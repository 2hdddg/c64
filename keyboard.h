#pragma once

#include <stdint.h>


#define KEY_INST            0x00fe
#define KEY_DEL             0x00fe
#define KEY_NUMBER          0x01fe
#define KEY_3               0x01fe
#define KEY_PERCENT         0x02fe
#define KEY_5               0x02fe
#define KEY_APOSTROPHE      0x03fe
#define KEY_7               0x03fe
#define KEY_PARAN_CLOSE     0x04fe
#define KEY_9               0x04fe
#define KEY_PLUS            0x05fe
#define KEY_POUND           0x06fe
#define KEY_EXCLAMATION     0x07fe
#define KEY_1               0x07fe

#define KEY_RETURN          0x00fd
#define KEY_W               0x01fd
#define KEY_R               0x02fd
#define KEY_Y               0x03fd
#define KEY_I               0x04fd
#define KEY_P               0x05fd
#define KEY_STAR            0x06fd
#define KEY_LEFT            0x07fd

#define KEY_CRSR_           0x00fb
#define KEY_A               0x01fb
#define KEY_D               0x02fb
#define KEY_G               0x03fb
#define KEY_J               0x04fb
#define KEY_L               0x05fb
#define KEY_BRACK_CLOSE_SQ  0x06fb
#define KEY_SEMICOLON       0x06fb
#define KEY_CTRL            0x07fb

#define KEY_F7              0x00f7
#define KEY_DOLLAR          0x01f7
#define KEY_4               0x01f7
#define KEY_AMP             0x02f7
#define KEY_6               0x02f7
#define KEY_PARAN_OPEN      0x03f7
#define KEY_8               0x03f7
#define KEY_0               0x04f7
#define KEY_MINUS           0x05f7
#define KEY_CLR             0x06f7
#define KEY_HOME            0x06f7
#define KEY_QUOTE           0x07f7
#define KEY_2               0x07f7

#define KEY_F1              0x00ef
#define KEY_Z               0x01ef
#define KEY_C               0x02ef
#define KEY_B               0x03ef
#define KEY_M               0x04ef
#define KEY_GT              0x05ef
#define KEY_DOT             0x05ef
#define KEY_RIGHT_SHIFT     0x06ef
#define KEY_SPACE           0x07ef

#define KEY_F3              0x00df
#define KEY_S               0x01df
#define KEY_F               0x02df
#define KEY_H               0x03df
#define KEY_K               0x04df
#define KEY_BRACK_OPEN_SQ   0x05df
#define KEY_COLON           0x05df
#define KEY_EQ              0x06df
#define KEY_COMMODORE       0x07df

#define KEY_F5              0x00bf
#define KEY_E               0x01bf
#define KEY_T               0x02bf
#define KEY_U               0x03bf
#define KEY_O               0x04bf
#define KEY_AT              0x05bf
#define KEY_CARET           0x06bf
#define KEY_Q               0x07bf

#define KEY_CRSR            0x007f
#define KEY_LEFT_SHIFT      0x017f
#define KEY_SHIFT_LOCK      0x017f
#define KEY_X               0x027f
#define KEY_V               0x037f
#define KEY_N               0x047f
#define KEY_LT              0x057f
#define KEY_COMMA           0x057f
#define KEY_QUESTION_MARK   0x067f
#define KEY_FORWARD_SLASH   0x067f
#define KEY_STOP            0x077f

void keyboard_reset();
void keyboard_down(uint16_t key);
void keyboard_up(uint16_t key);

uint8_t keyboard_get_port_A();
uint8_t keyboard_get_port_B();

void keyboard_set_port_A(uint8_t lines);
void keyboard_set_port_B(uint8_t lines);

void keyboard_trace_keys(int fd);
void keyboard_trace_port_set(int fd);
void keyboard_trace_port_get(int fd);

