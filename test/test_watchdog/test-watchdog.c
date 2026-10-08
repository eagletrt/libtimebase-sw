/*!
 * \file test-watchdogs.c
 * \date 2026-05-08
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 *
 * \brief Unit tests for the watchdogs module
 */

#include <unity.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "min-heap-api.h"
#include "arena-allocator-api.h"
#include "fff.h"
DEFINE_FFF_GLOBALS;
#include "watchdogs-api.h"

struct WatchdogHandler watchdogs_handler;
struct Watchdog watchdog_1;
struct Watchdog watchdog_2;
struct Watchdog watchdog_3;

/* The tick source of the module under test: tests move time by writing current_tick */
static uint32_t current_tick;
static uint32_t get_tick_calls;

static uint32_t fake_get_tick(void) {
    ++get_tick_calls;
    return current_tick;
}

FAKE_VOID_FUNC(watchdog_callback_1);
FAKE_VOID_FUNC(watchdog_callback_2);
FAKE_VOID_FUNC(watchdog_callback_3);

void test_watchdogs_init_pool_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_init_pool(NULL, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_init_pool_with_null_tick_callback_returns_error(void) {
    struct WatchdogHandler handler;

    enum WatchdogReturnCode rc = watchdogs_api_init_pool(&handler, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_init_pool_stores_tick_callback(void) {
    struct WatchdogHandler handler;

    enum WatchdogReturnCode rc = watchdogs_api_init_pool(&handler, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_TRUE_MESSAGE(handler.get_tick == fake_get_tick, "The tick callback should be stored in the handler");
}

void test_watchdogs_init_pool_with_valid_handler_returns_ok(void) {
    struct WatchdogHandler handler;

    current_tick = 42U;
    enum WatchdogReturnCode rc = watchdogs_api_init_pool(&handler, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(42U, handler.prev_tick, "prev_tick should be set to current_tick");
    TEST_ASSERT_TRUE_MESSAGE(min_heap_api_is_empty(&handler.scheduled_watchdogs), "Heap should be empty after init");
}

void test_watchdogs_init_watchdog_with_null_watchdog_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_init_watchdog(NULL, 100U, watchdog_callback_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_init_watchdog_with_null_callback_returns_error(void) {
    struct Watchdog wd = { 0 };

    enum WatchdogReturnCode rc = watchdogs_api_init_watchdog(&wd, 100U, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_init_watchdog_with_zero_timeout_returns_error(void) {
    struct Watchdog wd = { 0 };

    enum WatchdogReturnCode rc = watchdogs_api_init_watchdog(&wd, 0U, watchdog_callback_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_ERROR, rc, "Expected ERROR return code");
}

void test_watchdogs_init_watchdog_already_initialized_returns_error(void) {
    struct Watchdog wd = { 0 };
    watchdogs_api_init_watchdog(&wd, 100U, watchdog_callback_1);

    enum WatchdogReturnCode rc = watchdogs_api_init_watchdog(&wd, 100U, watchdog_callback_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_ERROR, rc, "Expected ERROR return code");
}

void test_watchdogs_init_watchdog_with_valid_params_returns_ok(void) {
    struct Watchdog wd = { 0 };

    enum WatchdogReturnCode rc = watchdogs_api_init_watchdog(&wd, 100U, watchdog_callback_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_TRUE_MESSAGE(wd.is_initialized, "Watchdog should be initialized");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(100U, wd.timeout, "Timeout should be set correctly");
    TEST_ASSERT_EQUAL_PTR_MESSAGE(watchdog_callback_1, wd.watchdog_callback, "Callback should be set correctly");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_NOT_RUNNING, wd.watchdog_state, "State should be NOT_RUNNING after init");
}

void test_watchdogs_start_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(NULL, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_start_with_null_watchdog_returns_error(void) {
    current_tick = 0U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_start_uninitialized_watchdog_returns_error(void) {
    struct Watchdog wd = { 0 };

    current_tick = 0U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, &wd);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_UNINITIALIZED, rc, "Expected UNINITIALIZED return code");
}

void test_watchdogs_start_with_past_tick_returns_temporal_discontinuity(void) {
    watchdogs_handler.prev_tick = 10U;

    current_tick = 5U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_watchdogs_start_already_running_watchdog_returns_busy(void) {

    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    current_tick = 5U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_BUSY, rc, "Expected BUSY return code");
}

void test_watchdogs_start_timed_out_watchdog_returns_error(void) {

    watchdog_1.watchdog_state = WATCHDOG_STATE_TIMED_OUT;

    current_tick = 0U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TIMED_OUT, rc, "Expected TIMED_OUT return code");
}

void test_watchdogs_start_valid_watchdog_returns_ok_and_sets_state(void) {

    current_tick = 5U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_RUNNING, watchdog_1.watchdog_state, "State should be RUNNING");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(5U + watchdog_1.timeout, watchdog_1.next_trigger, "next_trigger should be current_tick + timeout");
}

void test_watchdogs_start_valid_watchdog_inserts_into_heap(void) {

    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_FALSE_MESSAGE(min_heap_api_is_empty(&watchdogs_handler.scheduled_watchdogs), "Heap should not be empty after start");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, watchdogs_handler.scheduled_watchdogs.size, "Heap should contain 1 watchdog");
}

void test_watchdogs_stop_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_stop(NULL, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_stop_with_null_watchdog_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_stop(&watchdogs_handler, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_stop_uninitialized_watchdog_returns_error(void) {
    struct Watchdog wd = { 0 };

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_stop(&watchdogs_handler, &wd);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_UNINITIALIZED, rc, "Expected UNINITIALIZED return code");
}

void test_watchdogs_stop_not_running_watchdog_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_stop(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NOT_RUNNING, rc, "Expected NOT_RUNNING return code");
}

void test_watchdogs_stop_timed_out_watchdog_returns_error(void) {
    watchdog_1.watchdog_state = WATCHDOG_STATE_TIMED_OUT;

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_stop(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TIMED_OUT, rc, "Expected TIMED_OUT return code");
}

void test_watchdogs_stop_running_watchdog_returns_ok_and_sets_state(void) {

    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_stop(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_NOT_RUNNING, watchdog_1.watchdog_state, "State should be NOT_RUNNING");
}

void test_watchdogs_stop_running_watchdog_removes_from_heap(void) {

    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    watchdogs_api_watchdog_stop(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_TRUE_MESSAGE(min_heap_api_is_empty(&watchdogs_handler.scheduled_watchdogs), "Heap should be empty after stop");
}

void test_watchdogs_restart_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_restart(NULL, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_restart_with_null_watchdog_returns_error(void) {
    current_tick = 0U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_restart(&watchdogs_handler, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_restart_uninitialized_watchdog_returns_error(void) {
    struct Watchdog wd = { 0 };

    current_tick = 0U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_restart(&watchdogs_handler, &wd);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_UNINITIALIZED, rc, "Expected UNINITIALIZED return code");
}

void test_watchdogs_restart_with_past_tick_returns_temporal_discontinuity(void) {
    watchdogs_handler.prev_tick = 10U;

    current_tick = 5U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_watchdogs_restart_not_running_watchdog_starts_it(void) {

    current_tick = 10U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_RUNNING, watchdog_1.watchdog_state, "State should be RUNNING");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(10U + watchdog_1.timeout, watchdog_1.next_trigger, "next_trigger should be current_tick + timeout");
}

void test_watchdogs_restart_running_watchdog_resets_timer(void) {

    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    current_tick = 10U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(10U + watchdog_1.timeout, watchdog_1.next_trigger, "next_trigger should be reset from current_tick");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, watchdogs_handler.scheduled_watchdogs.size, "Heap should still contain 1 watchdog");
}

void test_watchdogs_restart_timed_out_watchdog_restarts_it(void) {

    watchdog_1.watchdog_state = WATCHDOG_STATE_TIMED_OUT;

    current_tick = 10U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_RUNNING, watchdog_1.watchdog_state, "State should be RUNNING after restart");
}

void test_watchdogs_reset_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_reset(NULL, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_reset_with_null_watchdog_returns_error(void) {
    current_tick = 0U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_reset(&watchdogs_handler, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_reset_uninitialized_watchdog_returns_error(void) {
    struct Watchdog wd = { 0 };

    current_tick = 0U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_reset(&watchdogs_handler, &wd);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_UNINITIALIZED, rc, "Expected UNINITIALIZED return code");
}

void test_watchdogs_reset_not_running_watchdog_returns_ok(void) {
    current_tick = 0U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_reset(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_NOT_RUNNING, watchdog_1.watchdog_state, "State should stay NOT_RUNNING");
    TEST_ASSERT_TRUE_MESSAGE(min_heap_api_is_empty(&watchdogs_handler.scheduled_watchdogs), "Heap should stay empty");
}

void test_watchdogs_reset_timed_out_watchdog_returns_ok_and_sets_state(void) {
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);
    current_tick = watchdog_1.timeout;
    watchdogs_api_routine(&watchdogs_handler);

    current_tick = watchdog_1.timeout;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_reset(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_NOT_RUNNING, watchdog_1.watchdog_state, "State should be NOT_RUNNING");
    TEST_ASSERT_FALSE_MESSAGE(watchdogs_api_watchdog_is_timed_out(&watchdog_1), "Watchdog should not be timed out anymore");
}

void test_watchdogs_reset_timed_out_watchdog_can_be_started_again(void) {
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);
    current_tick = watchdog_1.timeout;
    watchdogs_api_routine(&watchdogs_handler);
    current_tick = watchdog_1.timeout;
    watchdogs_api_watchdog_reset(&watchdogs_handler, &watchdog_1);

    current_tick = 150U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_RUNNING, watchdog_1.watchdog_state, "State should be RUNNING");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(150U + watchdog_1.timeout, watchdog_1.next_trigger, "next_trigger should be current_tick + timeout");
}

void test_watchdogs_reset_running_watchdog_returns_ok_and_sets_state(void) {
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    current_tick = 5U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_reset(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_NOT_RUNNING, watchdog_1.watchdog_state, "State should be NOT_RUNNING");
    TEST_ASSERT_FALSE_MESSAGE(watchdogs_api_watchdog_is_running(&watchdog_1), "Watchdog should not be running");
}

void test_watchdogs_reset_running_watchdog_removes_from_heap(void) {
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    current_tick = 5U;
    watchdogs_api_watchdog_reset(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_TRUE_MESSAGE(min_heap_api_is_empty(&watchdogs_handler.scheduled_watchdogs), "Heap should be empty after reset");
}

void test_watchdogs_reset_running_watchdog_does_not_fire_callback(void) {
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);
    current_tick = 5U;
    watchdogs_api_watchdog_reset(&watchdogs_handler, &watchdog_1);

    current_tick = watchdog_1.timeout;
    watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, watchdog_callback_1_fake.call_count, "Callback should not be called after reset");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_NOT_RUNNING, watchdog_1.watchdog_state, "State should stay NOT_RUNNING");
}

void test_watchdogs_reset_twice_returns_ok(void) {
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);
    current_tick = 5U;
    watchdogs_api_watchdog_reset(&watchdogs_handler, &watchdog_1);

    current_tick = 6U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_reset(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code on second reset");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_NOT_RUNNING, watchdog_1.watchdog_state, "State should be NOT_RUNNING");
}

void test_watchdogs_reset_watchdog_can_be_started_again(void) {
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);
    current_tick = 5U;
    watchdogs_api_watchdog_reset(&watchdogs_handler, &watchdog_1);

    current_tick = 10U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_RUNNING, watchdog_1.watchdog_state, "State should be RUNNING");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(10U + watchdog_1.timeout, watchdog_1.next_trigger, "next_trigger should be current_tick + timeout");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, watchdogs_handler.scheduled_watchdogs.size, "Heap should contain 1 watchdog");
}

void test_watchdogs_reset_keeps_other_watchdogs_scheduled(void) {
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_2);

    current_tick = 5U;
    watchdogs_api_watchdog_reset(&watchdogs_handler, &watchdog_1);
    current_tick = watchdog_2.timeout;
    watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, watchdog_callback_1_fake.call_count, "Callback 1 should not be called");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_2_fake.call_count, "Callback 2 should be called");
}

void test_watchdogs_pet_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pet(NULL, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_pet_with_null_watchdog_returns_error(void) {
    current_tick = 0U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pet(&watchdogs_handler, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_pet_uninitialized_watchdog_returns_error(void) {
    struct Watchdog wd = { 0 };

    current_tick = 0U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pet(&watchdogs_handler, &wd);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_UNINITIALIZED, rc, "Expected UNINITIALIZED return code");
}

void test_watchdogs_pet_with_past_tick_returns_temporal_discontinuity(void) {
    watchdogs_handler.prev_tick = 10U;
    current_tick = 10U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    current_tick = 5U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pet(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_watchdogs_pet_not_running_watchdog_returns_error(void) {
    current_tick = 0U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pet(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NOT_RUNNING, rc, "Expected NOT_RUNNING return code");
}

void test_watchdogs_pet_timed_out_watchdog_returns_error(void) {
    watchdog_1.watchdog_state = WATCHDOG_STATE_TIMED_OUT;

    current_tick = 0U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pet(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TIMED_OUT, rc, "Expected TIMED_OUT return code");
}

void test_watchdogs_pet_running_watchdog_resets_timer(void) {

    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    current_tick = 5U;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pet(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(5U + watchdog_1.timeout, watchdog_1.next_trigger, "next_trigger should be refreshed from current_tick");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_RUNNING, watchdog_1.watchdog_state, "State should remain RUNNING");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, watchdogs_handler.scheduled_watchdogs.size, "Heap should still contain 1 watchdog");
}

void test_watchdogs_is_running_null_returns_false(void) {
    TEST_ASSERT_FALSE_MESSAGE(watchdogs_api_watchdog_is_running(NULL), "NULL watchdog should return false");
}

void test_watchdogs_is_running_not_running_returns_false(void) {
    TEST_ASSERT_FALSE_MESSAGE(watchdogs_api_watchdog_is_running(&watchdog_1), "NOT_RUNNING watchdog should return false");
}

void test_watchdogs_is_running_running_returns_true(void) {

    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_TRUE_MESSAGE(watchdogs_api_watchdog_is_running(&watchdog_1), "RUNNING watchdog should return true");
}

void test_watchdogs_is_timed_out_null_returns_false(void) {
    TEST_ASSERT_FALSE_MESSAGE(watchdogs_api_watchdog_is_timed_out(NULL), "NULL watchdog should return false");
}

void test_watchdogs_is_timed_out_running_returns_false(void) {
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_FALSE_MESSAGE(watchdogs_api_watchdog_is_timed_out(&watchdog_1), "RUNNING watchdog should return false");
}

void test_watchdogs_routine_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_routine(NULL);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_routine_with_past_tick_returns_temporal_discontinuity(void) {
    watchdogs_handler.prev_tick = 10U;

    current_tick = 5U;
    enum WatchdogReturnCode rc = watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_watchdogs_routine_with_empty_heap_returns_ok(void) {

    current_tick = 5U;
    enum WatchdogReturnCode rc = watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(5U, watchdogs_handler.prev_tick, "prev_tick should be updated");
}

void test_watchdogs_routine_does_not_fire_before_timeout(void) {
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    current_tick = watchdog_1.timeout - 1U;
    watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, watchdog_callback_1_fake.call_count, "Callback should not have been called before timeout");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_RUNNING, watchdog_1.watchdog_state, "Watchdog should still be running");
}

void test_watchdogs_routine_fires_callback_at_timeout(void) {
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    current_tick = watchdog_1.timeout;
    watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_1_fake.call_count, "Callback should have been called once at timeout");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_TIMED_OUT, watchdog_1.watchdog_state, "State should be TIMED_OUT");
}

void test_watchdogs_routine_fires_multiple_expired_watchdogs(void) {
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_2);

    current_tick = watchdog_1.timeout;
    watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_1_fake.call_count, "Callback 1 should have been called");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_2_fake.call_count, "Callback 2 should have been called");
}

void test_watchdogs_routine_does_not_fire_non_expired_watchdog(void) {
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_3);

    current_tick = watchdog_1.timeout;
    watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_1_fake.call_count, "Callback 1 should have been called");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, watchdog_callback_3_fake.call_count, "Callback 3 should not have been called (longer timeout)");
}

void test_watchdogs_routine_updates_prev_tick(void) {

    current_tick = 20U;
    watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(20U, watchdogs_handler.prev_tick, "prev_tick should be updated after routine");
}

void test_watchdogs_routine_removes_timed_out_watchdogs_from_heap(void) {
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    current_tick = watchdog_1.timeout;
    watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_TRUE_MESSAGE(min_heap_api_is_empty(&watchdogs_handler.scheduled_watchdogs), "Heap should be empty after processing timed out watchdog");
}

void test_watchdogs_stop_with_shared_callback_and_same_deadline_removes_correct_watchdog(void) {
    struct Watchdog wd_a = { 0 };
    struct Watchdog wd_b = { 0 };
    watchdogs_api_init_watchdog(&wd_a, 100U, watchdog_callback_1);
    watchdogs_api_init_watchdog(&wd_b, 100U, watchdog_callback_1);
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &wd_a);
    current_tick = 0U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &wd_b);

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_stop(&watchdogs_handler, &wd_b);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, watchdogs_handler.scheduled_watchdogs.size, "Only wd_a should remain scheduled");

    current_tick = 100U;
    watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_TRUE_MESSAGE(watchdogs_api_watchdog_is_timed_out(&wd_a), "wd_a should have timed out");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_NOT_RUNNING, wd_b.watchdog_state, "wd_b should stay stopped");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_1_fake.call_count, "Callback should fire exactly once");
}

/*
 * TICK CALLBACK TESTS
 */

void test_watchdogs_routine_reads_the_tick_only_once(void) {
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_2);

    current_tick = watchdog_1.timeout;
    get_tick_calls = 0U;

    enum WatchdogReturnCode rc = watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_1_fake.call_count, "First watchdog should have fired");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_2_fake.call_count, "Second watchdog should have fired");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, get_tick_calls, "The routine should sample the tick once, no matter how many watchdogs expire");
}

void test_watchdogs_start_reads_the_tick_at_call_time(void) {
    current_tick = 30U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(30U + watchdog_1.timeout, watchdog_1.next_trigger, "next_trigger should use the tick returned by the callback when start is called");

    current_tick = 50U;
    watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(50U + watchdog_1.timeout, watchdog_1.next_trigger, "restart should read the tick again");
}

static enum WatchdogReturnCode callback_start_rc;
static enum WatchdogReturnCode callback_reset_rc;
static enum WatchdogReturnCode callback_restart_rc;

static void callback_restart_self(void) {
    callback_restart_rc = watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_1);
}

static void callback_start_self(void) {
    callback_start_rc = watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);
}

static void callback_reset_and_start_self(void) {
    callback_reset_rc = watchdogs_api_watchdog_reset(&watchdogs_handler, &watchdog_1);
    callback_start_rc = watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);
}

void test_watchdogs_routine_callback_can_restart_its_own_watchdog(void) {
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);
    watchdog_callback_1_fake.custom_fake = callback_restart_self;

    current_tick = watchdog_1.timeout;
    enum WatchdogReturnCode rc = watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, callback_restart_rc, "Restart called inside the callback should succeed");
    TEST_ASSERT_TRUE_MESSAGE(watchdogs_api_watchdog_is_running(&watchdog_1), "Watchdog should be running again");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(2U * watchdog_1.timeout, watchdog_1.next_trigger, "next_trigger should be the tick of the callback plus the timeout");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, watchdogs_handler.scheduled_watchdogs.size, "Watchdog should be in the heap exactly once");

    current_tick = 2U * watchdog_1.timeout - 1U;
    watchdogs_api_routine(&watchdogs_handler);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_1_fake.call_count, "Watchdog should not fire before the new timeout");

    current_tick = 2U * watchdog_1.timeout;
    watchdogs_api_routine(&watchdogs_handler);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, watchdog_callback_1_fake.call_count, "Watchdog should fire again at the new timeout");
}

void test_watchdogs_routine_callback_start_of_its_own_timed_out_watchdog_fails(void) {
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);
    watchdog_callback_1_fake.custom_fake = callback_start_self;

    current_tick = watchdog_1.timeout;
    watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TIMED_OUT, callback_start_rc, "A timed out watchdog cannot be started, not even from its own callback");
    TEST_ASSERT_TRUE_MESSAGE(watchdogs_api_watchdog_is_timed_out(&watchdog_1), "Watchdog should stay timed out");
}

void test_watchdogs_routine_callback_can_reset_and_start_its_own_watchdog(void) {
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1);
    watchdog_callback_1_fake.custom_fake = callback_reset_and_start_self;

    current_tick = watchdog_1.timeout;
    enum WatchdogReturnCode rc = watchdogs_api_routine(&watchdogs_handler);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, callback_reset_rc, "Reset called inside the callback should succeed");
    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, callback_start_rc, "Start after the reset should succeed");
    TEST_ASSERT_TRUE_MESSAGE(watchdogs_api_watchdog_is_running(&watchdog_1), "Watchdog should be running again");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(2U * watchdog_1.timeout, watchdog_1.next_trigger, "next_trigger should be the tick of the callback plus the timeout");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, watchdogs_handler.scheduled_watchdogs.size, "Watchdog should be in the heap exactly once");
}

void setUp(void) {
    memset(&watchdogs_handler, 0, sizeof(watchdogs_handler));
    memset(&watchdog_1, 0, sizeof(watchdog_1));
    memset(&watchdog_2, 0, sizeof(watchdog_2));
    memset(&watchdog_3, 0, sizeof(watchdog_3));

    current_tick = 0U;
    watchdogs_api_init_pool(&watchdogs_handler, fake_get_tick);
    get_tick_calls = 0U;
    callback_start_rc = callback_reset_rc = callback_restart_rc = WATCHDOG_RC_ERROR;
    watchdogs_api_init_watchdog(&watchdog_1, 100U, watchdog_callback_1);
    watchdogs_api_init_watchdog(&watchdog_2, 100U, watchdog_callback_2);
    watchdogs_api_init_watchdog(&watchdog_3, 200U, watchdog_callback_3);

    RESET_FAKE(watchdog_callback_1);
    RESET_FAKE(watchdog_callback_2);
    RESET_FAKE(watchdog_callback_3);
}

void tearDown(void) {
}

int main(void) {
    UNITY_BEGIN();

    // INIT MODULE TESTS
    RUN_TEST(test_watchdogs_init_pool_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_init_pool_with_null_tick_callback_returns_error);
    RUN_TEST(test_watchdogs_init_pool_stores_tick_callback);
    RUN_TEST(test_watchdogs_init_pool_with_valid_handler_returns_ok);

    // INIT WATCHDOG TESTS
    RUN_TEST(test_watchdogs_init_watchdog_with_null_watchdog_returns_error);
    RUN_TEST(test_watchdogs_init_watchdog_with_null_callback_returns_error);
    RUN_TEST(test_watchdogs_init_watchdog_with_zero_timeout_returns_error);
    RUN_TEST(test_watchdogs_init_watchdog_already_initialized_returns_error);
    RUN_TEST(test_watchdogs_init_watchdog_with_valid_params_returns_ok);

    // START TESTS
    RUN_TEST(test_watchdogs_start_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_start_with_null_watchdog_returns_error);
    RUN_TEST(test_watchdogs_start_uninitialized_watchdog_returns_error);
    RUN_TEST(test_watchdogs_start_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_watchdogs_start_already_running_watchdog_returns_busy);
    RUN_TEST(test_watchdogs_start_timed_out_watchdog_returns_error);
    RUN_TEST(test_watchdogs_start_valid_watchdog_returns_ok_and_sets_state);
    RUN_TEST(test_watchdogs_start_valid_watchdog_inserts_into_heap);

    // STOP TESTS
    RUN_TEST(test_watchdogs_stop_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_stop_with_null_watchdog_returns_error);
    RUN_TEST(test_watchdogs_stop_uninitialized_watchdog_returns_error);
    RUN_TEST(test_watchdogs_stop_not_running_watchdog_returns_error);
    RUN_TEST(test_watchdogs_stop_timed_out_watchdog_returns_error);
    RUN_TEST(test_watchdogs_stop_running_watchdog_returns_ok_and_sets_state);
    RUN_TEST(test_watchdogs_stop_running_watchdog_removes_from_heap);

    // RESTART TESTS
    RUN_TEST(test_watchdogs_restart_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_restart_with_null_watchdog_returns_error);
    RUN_TEST(test_watchdogs_restart_uninitialized_watchdog_returns_error);
    RUN_TEST(test_watchdogs_restart_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_watchdogs_restart_not_running_watchdog_starts_it);
    RUN_TEST(test_watchdogs_restart_running_watchdog_resets_timer);
    RUN_TEST(test_watchdogs_restart_timed_out_watchdog_restarts_it);

    // RESET TESTS
    RUN_TEST(test_watchdogs_reset_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_reset_with_null_watchdog_returns_error);
    RUN_TEST(test_watchdogs_reset_uninitialized_watchdog_returns_error);
    RUN_TEST(test_watchdogs_reset_not_running_watchdog_returns_ok);
    RUN_TEST(test_watchdogs_reset_timed_out_watchdog_returns_ok_and_sets_state);
    RUN_TEST(test_watchdogs_reset_timed_out_watchdog_can_be_started_again);
    RUN_TEST(test_watchdogs_reset_running_watchdog_returns_ok_and_sets_state);
    RUN_TEST(test_watchdogs_reset_running_watchdog_removes_from_heap);
    RUN_TEST(test_watchdogs_reset_running_watchdog_does_not_fire_callback);
    RUN_TEST(test_watchdogs_reset_twice_returns_ok);
    RUN_TEST(test_watchdogs_reset_watchdog_can_be_started_again);
    RUN_TEST(test_watchdogs_reset_keeps_other_watchdogs_scheduled);

    // PET TESTS
    RUN_TEST(test_watchdogs_pet_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_pet_with_null_watchdog_returns_error);
    RUN_TEST(test_watchdogs_pet_uninitialized_watchdog_returns_error);
    RUN_TEST(test_watchdogs_pet_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_watchdogs_pet_not_running_watchdog_returns_error);
    RUN_TEST(test_watchdogs_pet_timed_out_watchdog_returns_error);
    RUN_TEST(test_watchdogs_pet_running_watchdog_resets_timer);

    // IS_RUNNING / IS_TIMED_OUT TESTS
    RUN_TEST(test_watchdogs_is_running_null_returns_false);
    RUN_TEST(test_watchdogs_is_running_not_running_returns_false);
    RUN_TEST(test_watchdogs_is_running_running_returns_true);
    RUN_TEST(test_watchdogs_is_timed_out_null_returns_false);
    RUN_TEST(test_watchdogs_is_timed_out_running_returns_false);

    // ROUTINE TESTS
    RUN_TEST(test_watchdogs_routine_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_routine_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_watchdogs_routine_with_empty_heap_returns_ok);
    RUN_TEST(test_watchdogs_routine_does_not_fire_before_timeout);
    RUN_TEST(test_watchdogs_routine_fires_callback_at_timeout);
    RUN_TEST(test_watchdogs_routine_fires_multiple_expired_watchdogs);
    RUN_TEST(test_watchdogs_routine_does_not_fire_non_expired_watchdog);
    RUN_TEST(test_watchdogs_routine_updates_prev_tick);
    RUN_TEST(test_watchdogs_routine_removes_timed_out_watchdogs_from_heap);

    // TICK CALLBACK TESTS
    RUN_TEST(test_watchdogs_routine_reads_the_tick_only_once);
    RUN_TEST(test_watchdogs_start_reads_the_tick_at_call_time);
    RUN_TEST(test_watchdogs_routine_callback_can_restart_its_own_watchdog);
    RUN_TEST(test_watchdogs_routine_callback_start_of_its_own_timed_out_watchdog_fails);
    RUN_TEST(test_watchdogs_routine_callback_can_reset_and_start_its_own_watchdog);

    // GENERAL TESTS
    RUN_TEST(test_watchdogs_stop_with_shared_callback_and_same_deadline_removes_correct_watchdog);

    return UNITY_END();
}