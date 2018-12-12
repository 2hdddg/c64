#include <stdio.h>

#include "cia1.h"
#include "cia_timer.h"


int _num_interrupt_requests;

/* Mocking CPU */
void cpu_interrupt_request()
{
    _num_interrupt_requests++;
}

int assert_num_interrupt_requests(int n)
{
    if (n != _num_interrupt_requests) {
        printf("Expected number of interrupt requests to be %d "
               "but was %d\n", n, _num_interrupt_requests);
        return 0;
    }
    return 1;
}

int assert_interrupt_status(int expected, int actual)
{
    if (expected != actual) {
        printf("Got the wrong interrupt status, expected %02x "
               "but was %02x\n", expected, actual);
        return 0;
    }
    return 1;
}

int once_before()
{
    cia1_init();
    return 0;
}

int each_before()
{
    cia1_reset();
    _num_interrupt_requests = 0;

    return 0;
}

int test_timer_A_underflow_interrupt_request()
{
    /* Set timer latch */
    cia1_mem_set(0x00, CIA1_ADDRESS + CIA_REG_TIMER_A_LO, 0, NULL);
    cia1_mem_set(0x00, CIA1_ADDRESS + CIA_REG_TIMER_A_HI, 0, NULL);
    /* Enable timer A underflow interrupt */
    cia1_mem_set(CIA_INT_MASK_SET|CIA_INT_UNDERFLOW_TIMER_A,
                 CIA1_ADDRESS + CIA_REG_INTERRUPT_CONTROL, 0, NULL);
    /* Start one-shot timer */
    cia1_mem_set(CIA_TIMER_CTRL_START|CIA_TIMER_CTRL_ONE_SHOT,
                 CIA1_ADDRESS + CIA_REG_TIMER_A_CONTROL, 0, NULL);
    /* Cycle once, should generate underflow interrupt */
    cia1_cycle();
    if (!assert_num_interrupt_requests(1)) {
        return 0;
    }
    /* Cycle again, should still generate underflow interrupt */
    cia1_cycle();
    if (!assert_num_interrupt_requests(2)) {
        return 0;
    }
    /* Reading interrupt control register clears interrupt */
    uint8_t int_status =
        cia1_mem_get(CIA1_ADDRESS + CIA_REG_INTERRUPT_CONTROL, 0, NULL);
    /* Should see that interrupt occured and type of it */
    if (!assert_interrupt_status(
        CIA_INT_OCCURED|CIA_INT_UNDERFLOW_TIMER_A,
        int_status)) {
        return 0;
    }
    /* Cycle now should NOT trigger any interrupt since timer was
     * one-shot. */
    cia1_cycle();
    if (!assert_num_interrupt_requests(2)) {
        return 0;
    }
    /* Interrupt control register now should be empty */
    int_status = cia1_mem_get(CIA1_ADDRESS + CIA_REG_INTERRUPT_CONTROL,
                              0, NULL);
    if (!assert_interrupt_status( 0, int_status)) {
        return 0;
    }

    return 1;
}
