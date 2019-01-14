#include <string.h>
#include <stdio.h>
#include "mem.h"

/* Recorded from last hook call */
uint8_t _val;
uint16_t _absolute;
uint8_t *_ram;

int _num_set_hook_calls;
int _num_get_hook_calls;


#define ADDR_0x10 0x1000

int each_before()
{
    mem_init();
    mem_reset();

    /* Set some known values for cpu (assumes set works) */
    mem_set_for_cpu(0x1000, 0x10);
    mem_set_for_cpu(0x1001, 0x20);

    _val = 0;
    _absolute = 0;
    _ram = 0;

    _num_set_hook_calls = 0;
    _num_get_hook_calls = 0;

    return 0;
}

static int assert_val(uint8_t actual, uint8_t expected)
{
    if (actual != expected) {
        printf("Expected %02x but was %02x\n",
               expected, actual);
        return 0;
    }
    return 1;
}

static void set_hook(uint8_t val, uint16_t absolute, uint8_t *ram)
{
    _val = val;
    _absolute = absolute;
    _ram = ram;
    _num_set_hook_calls++;
}

static uint8_t get_hook(uint16_t absolute, uint8_t *ram)
{
    _absolute = absolute;
    _ram = ram;
    _num_get_hook_calls++;

    return _val;
}

int test_get_for_cpu()
{
    return assert_val(mem_get_for_cpu(ADDR_0x10), 0x10);
}

int test_set_for_cpu()
{
    mem_set_for_cpu(0x2000, 0x66);
    return assert_val(mem_get_for_cpu(0x2000), 0x66);
}

int test_install_set_hook_for_cpu()
{
    /* Install hook over known values */
    struct mem_hook_install install = {
        .set_hook = set_hook,
        .page_start = ADDR_0x10 >> 8,
        .num_pages = 1,
    };
    mem_install_hooks_for_cpu(&install, 1);

    /* Set a value, should trigger hook */
    mem_set_for_cpu(ADDR_0x10, 0x90);

    if (_num_set_hook_calls != 1) {
        printf("Set hook not called\n");
        return 0;
    }

    /* Check parameters to hook */
    if (_val != 0x90 || _absolute != ADDR_0x10 || *_ram != 0x10) {
        printf("Wrong parameters to set hook\n");
        return 0;
    }

    /* Should not have affected RAM */
    return assert_val(mem_get_for_cpu(ADDR_0x10), 0x10);
}

int test_install_get_hook_for_cpu()
{
    /* Install hook over known values */
    struct mem_hook_install install = {
        .get_hook = get_hook,
        .page_start = ADDR_0x10 >> 8,
        .num_pages = 1,
    };
    uint8_t val;
    mem_install_hooks_for_cpu(&install, 1);

    /* Get a known value but at the hook */
    _val = 0x88;
    val = mem_get_for_cpu(ADDR_0x10);

    if (_num_get_hook_calls != 1) {
        printf("Set hook not called\n");
        return 0;
    }

    /* Check parameters to hook */
    if (_absolute != ADDR_0x10 || *_ram != 0x10) {
        printf("Wrong parameters to get hook\n");
        return 0;
    }

    if (!assert_val(val, _val)) {
        return 0;
    }

    return 1;
}

int test_remove_set_hook_for_cpu()
{
    struct mem_hook_install install = {
        .set_hook = set_hook,
        .page_start = ADDR_0x10 >> 8,
        .num_pages = 1,
    };
    mem_install_hooks_for_cpu(&install, 1);
    /* Remove by clearing hook */
    install.set_hook = NULL;
    mem_install_hooks_for_cpu(&install, 1);

    mem_set_for_cpu(0x2000, 0x66);
    if (_num_set_hook_calls > 0) {
        printf("Set hook called\n");
        return 0;
    }
    return assert_val(mem_get_for_cpu(0x2000), 0x66);
}

int test_remove_get_hook_for_cpu()
{
    /* Install hook over known values */
    struct mem_hook_install install = {
        .get_hook = get_hook,
        .page_start = ADDR_0x10 >> 8,
        .num_pages = 1,
    };
    uint8_t val;
    mem_install_hooks_for_cpu(&install, 1);
    /* Remove by clearing hook */
    install.get_hook = NULL;
    mem_install_hooks_for_cpu(&install, 1);

    _val = 0x88;
    val = mem_get_for_cpu(ADDR_0x10);

    if (_num_get_hook_calls > 0) {
        printf("Get hook called\n");
        return 0;
    }
    return assert_val(val, 0x10);
}
