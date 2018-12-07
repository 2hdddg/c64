#include <stdio.h>

#include "cia_timer.h"

struct cia_timer _timer_A;
struct cia_timer _timer_B;

static int assert_timer(struct cia_timer *timer,
                        uint8_t timer_lo, uint8_t timer_hi)
{
    if (timer->timer_lo != timer_lo ||
        timer->timer_hi != timer_hi) {
        printf("Expected timer to be %02x%02x but was "
               "%02x%02x\n",
               timer_hi, timer_lo,
               timer->timer_hi, timer->timer_lo);
        return 0;
    }
    return 1;
}

static int assert_not_underflowed(struct cia_timer *timer)
{
    if (timer->underflowed) {
        printf("Expected timer not to be underflowed!\n");
        return 0;
    }
    return 1;
}

static int assert_underflowed(struct cia_timer *timer)
{
    if (!timer->underflowed) {
        printf("Expected timer to be underflowed!\n");
        return 0;
    }
    return 1;
}

static int assert_not_started(struct cia_timer *timer)
{
    if (timer->started) {
        printf("Expected timer not to be started!\n");
        return 0;
    }
    return 1;
}

static int assert_started(struct cia_timer *timer)
{
    if (!timer->started) {
        printf("Expected timer to be started!\n");
        return 0;
    }
    return 1;
}

int each_before()
{
    cia_timer_reset(&_timer_A);
    cia_timer_reset(&_timer_B);
    return 0;
}

int test_set_latch_lo_does_not_affect_timer()
{
    _timer_A.timer_hi = 10;
    _timer_A.timer_lo = 10;

    cia_timer_set_latch_lo(&_timer_A, 20);

    return assert_timer(&_timer_A, 10, 10);
}

int test_set_latch_hi_sets_timer_when_stopped()
{
    _timer_A.timer_hi = 10;
    _timer_A.timer_lo = 20;
    _timer_A.latch_lo = 30;

    cia_timer_set_latch_hi(&_timer_A, 40);

    return assert_timer(&_timer_A, 30, 40);
}

int test_set_latch_hi_does_not_affect_timer_when_started()
{
    _timer_A.timer_hi = 10;
    _timer_A.timer_lo = 20;
    _timer_A.latch_lo = 30;
    _timer_A.started = true;

    cia_timer_set_latch_hi(&_timer_A, 40);

    return assert_timer(&_timer_A, 20, 10);
}

int test_cycle_A_underflows_low()
{
    cia_timer_set_latch_lo(&_timer_A, 1);
    cia_timer_set_latch_hi(&_timer_A, 0);
    /* Start one-shot timer */
    cia_timer_control_A(&_timer_A, 0x09);

    if (!assert_not_underflowed(&_timer_A)) {
        return 0;
    }

    cia_timer_cycle(&_timer_A, &_timer_B);
    cia_timer_cycle(&_timer_A, &_timer_B);

    return assert_underflowed(&_timer_A);
}

int test_one_shot_should_stop_timer_after_underflow()
{
    cia_timer_set_latch_lo(&_timer_A, 0);
    cia_timer_set_latch_hi(&_timer_A, 0);
    /* Start one-shot timer */
    cia_timer_control_A(&_timer_A, 0x09);
    if (!assert_started(&_timer_A)) {
        return 0;
    }

    cia_timer_cycle(&_timer_A, &_timer_B);

    /* Should reload latch */
    assert_timer(&_timer_A, 0, 0);

    return assert_not_started(&_timer_A);
}

int test_continously_should_start_timer_after_underflow()
{
    cia_timer_set_latch_lo(&_timer_A, 1);
    cia_timer_set_latch_hi(&_timer_A, 0);
    /* Start continous timer */
    cia_timer_control_A(&_timer_A, 0x01);
    if (!assert_started(&_timer_A)) {
        return 0;
    }

    cia_timer_cycle(&_timer_A, &_timer_B);
    cia_timer_cycle(&_timer_A, &_timer_B);

    /* Should reload latch */
    assert_timer(&_timer_A, 1, 0);

    return assert_started(&_timer_A);
}

int test_should_not_count_cycle_when_input_is_pin_CNT()
{
    cia_timer_set_latch_lo(&_timer_A, 1);
    cia_timer_set_latch_hi(&_timer_A, 0);
    /* Start continous timer counting CNT pin */
    cia_timer_control_A(&_timer_A, 0x21);

    cia_timer_cycle(&_timer_A, &_timer_B);

    return assert_timer(&_timer_A, 1, 0);
}


