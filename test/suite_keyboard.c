#include <stdio.h>
#include "keyboard.h"


uint8_t _line;

int each_before()
{
    keyboard_reset();
    _line = 0xff;
    return 1;
}

int test_can_get_S()
{

    /* Push down key S. S is on line 1 */
    keyboard_down(KEY_S);
    /* Try to read line 0, should be no keys pressed */
    keyboard_set_port_A(0xfe);
    _line = keyboard_get_port_B();
    if (_line != 0xff) {
        printf("Should be no key but was: %02x!\n", _line);
        return 0;
    }
    /* Now read line 1, should have S key pressed */
    keyboard_set_port_A(0xfd);
    _line = keyboard_get_port_B();
    if (_line != 0xdf) {
        printf("Expected S but was %02x\n", _line);
        return 0;
    }
    /* Release key */
    keyboard_up(KEY_S);
    /* Read line 1 again */
    _line = keyboard_get_port_B();
    if (_line != 0xff) {
        printf("Should be no key but was: %02x!\n", _line);
        return 0;
    }

    return 1;
}

int test_can_get_multiple_keys_on_same_line()
{
    /* Push down two keys on line 1 */
    keyboard_down(KEY_A);
    keyboard_down(KEY_LEFT_SHIFT);

    /* Now read line 1 */
    keyboard_set_port_A(0xfd);
    _line = keyboard_get_port_B();

    if (_line != 0x7b) {
        printf("Should be 0x7b but was %02x\n", _line);
        return 0;
    }
    return 1;
}
