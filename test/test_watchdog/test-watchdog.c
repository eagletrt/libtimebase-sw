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

FAKE_VOID_FUNC(watchdog_callback_1);
FAKE_VOID_FUNC(watchdog_callback_2);
FAKE_VOID_FUNC(watchdog_callback_3);

void test_watchdogs_init_pool_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_init_pool(NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_init_pool_with_valid_handler_returns_ok(void) {
    struct WatchdogHandler handler;

    enum WatchdogReturnCode rc = watchdogs_api_init_pool(&handler, 42U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(42U, handler.prev_tick, "prev_tick should be set to current_tick");
    TEST_ASSERT_FALSE_MESSAGE(handler.watchdog_module_enabled, "Module should not be enabled after init");
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
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(NULL, &watchdog_1, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_start_with_null_watchdog_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_start_uninitialized_watchdog_returns_error(void) {
    struct Watchdog wd = { 0 };

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, &wd, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_UNINITIALIZED, rc, "Expected UNINITIALIZED return code");
}

void test_watchdogs_start_with_past_tick_returns_temporal_discontinuity(void) {
    watchdogs_handler.prev_tick = 10U;

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_watchdogs_start_already_running_watchdog_returns_busy(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_BUSY, rc, "Expected BUSY return code");
}

void test_watchdogs_start_timed_out_watchdog_returns_error(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdog_1.watchdog_state = WATCHDOG_STATE_TIMED_OUT;

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TIMED_OUT, rc, "Expected TIMED_OUT return code");
}

void test_watchdogs_start_valid_watchdog_returns_ok_and_sets_state(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_RUNNING, watchdog_1.watchdog_state, "State should be RUNNING");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(5U + watchdog_1.timeout, watchdog_1.next_trigger, "next_trigger should be current_tick + timeout");
}

void test_watchdogs_start_valid_watchdog_inserts_into_heap(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    TEST_ASSERT_FALSE_MESSAGE(min_heap_api_is_empty(&watchdogs_handler.scheduled_watchdogs), "Heap should not be empty after start");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, watchdogs_handler.scheduled_watchdogs.size, "Heap should contain 1 watchdog");
}

void test_watchdogs_start_paused_watchdog_resumes_with_remaining_time(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);
    watchdogs_api_watchdog_pause(&watchdogs_handler, &watchdog_1, 5U);

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 10U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(WATCHDOG_STATE_RUNNING, watchdog_1.watchdog_state, "State should be RUNNING");
    // next_trigger was (0 + timeout), paused at 5, so remaining = (timeout - 5), resumed at 10 -> next = 10 + (timeout - 5)
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(10U + (watchdog_1.timeout - 5U), watchdog_1.next_trigger, "next_trigger should account for elapsed time before pause");
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
    watchdogs_handler.watchdog_module_enabled = true;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_stop(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_NOT_RUNNING, watchdog_1.watchdog_state, "State should be NOT_RUNNING");
}

void test_watchdogs_stop_running_watchdog_removes_from_heap(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    watchdogs_api_watchdog_stop(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_TRUE_MESSAGE(min_heap_api_is_empty(&watchdogs_handler.scheduled_watchdogs), "Heap should be empty after stop");
}

void test_watchdogs_stop_paused_watchdog_returns_ok_without_heap_removal(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);
    watchdogs_api_watchdog_pause(&watchdogs_handler, &watchdog_1, 5U);

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_stop(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_NOT_RUNNING, watchdog_1.watchdog_state, "State should be NOT_RUNNING");
}

void test_watchdogs_pause_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pause(NULL, &watchdog_1, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_pause_with_null_watchdog_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pause(&watchdogs_handler, NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_pause_uninitialized_watchdog_returns_error(void) {
    struct Watchdog wd = { 0 };

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pause(&watchdogs_handler, &wd, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_UNINITIALIZED, rc, "Expected UNINITIALIZED return code");
}

void test_watchdogs_pause_with_past_tick_returns_temporal_discontinuity(void) {
    watchdogs_handler.prev_tick = 10U;

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pause(&watchdogs_handler, &watchdog_1, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_watchdogs_pause_not_running_watchdog_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pause(&watchdogs_handler, &watchdog_1, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NOT_RUNNING, rc, "Expected NOT_RUNNING return code");
}

void test_watchdogs_pause_timed_out_watchdog_returns_error(void) {
    watchdog_1.watchdog_state = WATCHDOG_STATE_TIMED_OUT;

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pause(&watchdogs_handler, &watchdog_1, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TIMED_OUT, rc, "Expected TIMED_OUT return code");
}

void test_watchdogs_pause_running_watchdog_returns_ok_and_sets_state(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pause(&watchdogs_handler, &watchdog_1, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_PAUSED, watchdog_1.watchdog_state, "State should be PAUSED");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(5U, watchdog_1.last_update, "last_update should be set to current_tick");
}

void test_watchdogs_pause_running_watchdog_removes_from_heap(void) {
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    watchdogs_api_watchdog_pause(&watchdogs_handler, &watchdog_1, 5U);

    TEST_ASSERT_TRUE_MESSAGE(min_heap_api_is_empty(&watchdogs_handler.scheduled_watchdogs), "Heap should be empty after pause");
}

void test_watchdogs_restart_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_restart(NULL, &watchdog_1, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_restart_with_null_watchdog_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_restart(&watchdogs_handler, NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_restart_uninitialized_watchdog_returns_error(void) {
    struct Watchdog wd = { 0 };

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_restart(&watchdogs_handler, &wd, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_UNINITIALIZED, rc, "Expected UNINITIALIZED return code");
}

void test_watchdogs_restart_with_past_tick_returns_temporal_discontinuity(void) {
    watchdogs_handler.prev_tick = 10U;

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_1, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_watchdogs_restart_not_running_watchdog_starts_it(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_1, 10U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_RUNNING, watchdog_1.watchdog_state, "State should be RUNNING");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(10U + watchdog_1.timeout, watchdog_1.next_trigger, "next_trigger should be current_tick + timeout");
}

void test_watchdogs_restart_running_watchdog_resets_timer(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_1, 10U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(10U + watchdog_1.timeout, watchdog_1.next_trigger, "next_trigger should be reset from current_tick");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, watchdogs_handler.scheduled_watchdogs.size, "Heap should still contain 1 watchdog");
}

void test_watchdogs_restart_timed_out_watchdog_restarts_it(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdog_1.watchdog_state = WATCHDOG_STATE_TIMED_OUT;

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_1, 10U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_RUNNING, watchdog_1.watchdog_state, "State should be RUNNING after restart");
}

void test_watchdogs_pet_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pet(NULL, &watchdog_1, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_pet_with_null_watchdog_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pet(&watchdogs_handler, NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_pet_uninitialized_watchdog_returns_error(void) {
    struct Watchdog wd = { 0 };

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pet(&watchdogs_handler, &wd, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_UNINITIALIZED, rc, "Expected UNINITIALIZED return code");
}

void test_watchdogs_pet_with_past_tick_returns_temporal_discontinuity(void) {
    watchdogs_handler.prev_tick = 10U;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 10U);

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pet(&watchdogs_handler, &watchdog_1, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_watchdogs_pet_not_running_watchdog_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pet(&watchdogs_handler, &watchdog_1, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NOT_RUNNING, rc, "Expected NOT_RUNNING return code");
}

void test_watchdogs_pet_timed_out_watchdog_returns_error(void) {
    watchdog_1.watchdog_state = WATCHDOG_STATE_TIMED_OUT;

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pet(&watchdogs_handler, &watchdog_1, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TIMED_OUT, rc, "Expected TIMED_OUT return code");
}

void test_watchdogs_pet_running_watchdog_resets_timer(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_pet(&watchdogs_handler, &watchdog_1, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(5U + watchdog_1.timeout, watchdog_1.next_trigger, "next_trigger should be refreshed from current_tick");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_RUNNING, watchdog_1.watchdog_state, "State should remain RUNNING");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, watchdogs_handler.scheduled_watchdogs.size, "Heap should still contain 1 watchdog");
}

void test_watchdogs_timeout_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_timeout(NULL, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_timeout_with_null_watchdog_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_timeout(&watchdogs_handler, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_timeout_uninitialized_watchdog_returns_error(void) {
    struct Watchdog wd = { 0 };

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_timeout(&watchdogs_handler, &wd);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_UNINITIALIZED, rc, "Expected UNINITIALIZED return code");
}

void test_watchdogs_timeout_not_running_watchdog_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_watchdog_timeout(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NOT_RUNNING, rc, "Expected NOT_RUNNING return code");
}

void test_watchdogs_timeout_running_watchdog_fires_callback_and_sets_state(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    enum WatchdogReturnCode rc = watchdogs_api_watchdog_timeout(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_TIMED_OUT, watchdog_1.watchdog_state, "State should be TIMED_OUT");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_1_fake.call_count, "Callback should have been called once");
}

void test_watchdogs_timeout_running_watchdog_removes_from_heap(void) {
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    watchdogs_api_watchdog_timeout(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_TRUE_MESSAGE(min_heap_api_is_empty(&watchdogs_handler.scheduled_watchdogs), "Heap should be empty after forced timeout");
}

void test_watchdogs_is_running_null_returns_false(void) {
    TEST_ASSERT_FALSE_MESSAGE(watchdogs_api_watchdog_is_running(NULL), "NULL watchdog should return false");
}

void test_watchdogs_is_running_not_running_returns_false(void) {
    TEST_ASSERT_FALSE_MESSAGE(watchdogs_api_watchdog_is_running(&watchdog_1), "NOT_RUNNING watchdog should return false");
}

void test_watchdogs_is_running_running_returns_true(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    TEST_ASSERT_TRUE_MESSAGE(watchdogs_api_watchdog_is_running(&watchdog_1), "RUNNING watchdog should return true");
}

void test_watchdogs_is_timed_out_null_returns_false(void) {
    TEST_ASSERT_FALSE_MESSAGE(watchdogs_api_watchdog_is_timed_out(NULL), "NULL watchdog should return false");
}

void test_watchdogs_is_timed_out_running_returns_false(void) {
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    TEST_ASSERT_FALSE_MESSAGE(watchdogs_api_watchdog_is_timed_out(&watchdog_1), "RUNNING watchdog should return false");
}

void test_watchdogs_is_timed_out_after_timeout_returns_true(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);
    watchdogs_api_watchdog_timeout(&watchdogs_handler, &watchdog_1);

    TEST_ASSERT_TRUE_MESSAGE(watchdogs_api_watchdog_is_timed_out(&watchdog_1), "TIMED_OUT watchdog should return true");
}

void test_watchdogs_routine_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_routine(NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_routine_with_past_tick_returns_temporal_discontinuity(void) {
    watchdogs_handler.prev_tick = 10U;
    watchdogs_handler.watchdog_module_enabled = true;

    enum WatchdogReturnCode rc = watchdogs_api_routine(&watchdogs_handler, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_watchdogs_routine_with_module_disabled_returns_not_running(void) {
    watchdogs_handler.watchdog_module_enabled = false;

    enum WatchdogReturnCode rc = watchdogs_api_routine(&watchdogs_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_DISABLED, rc, "Expected DISABLED return code");
}

void test_watchdogs_routine_with_empty_heap_returns_ok(void) {
    watchdogs_handler.watchdog_module_enabled = true;

    enum WatchdogReturnCode rc = watchdogs_api_routine(&watchdogs_handler, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(5U, watchdogs_handler.prev_tick, "prev_tick should be updated");
}

void test_watchdogs_routine_does_not_fire_before_timeout(void) {
    watchdogs_api_enable_pool(&watchdogs_handler, 0U);
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    watchdogs_api_routine(&watchdogs_handler, watchdog_1.timeout - 1U);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, watchdog_callback_1_fake.call_count, "Callback should not have been called before timeout");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_RUNNING, watchdog_1.watchdog_state, "Watchdog should still be running");
}

void test_watchdogs_routine_fires_callback_at_timeout(void) {
    watchdogs_api_enable_pool(&watchdogs_handler, 0U);
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    watchdogs_api_routine(&watchdogs_handler, watchdog_1.timeout);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_1_fake.call_count, "Callback should have been called once at timeout");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(WATCHDOG_STATE_TIMED_OUT, watchdog_1.watchdog_state, "State should be TIMED_OUT");
}

void test_watchdogs_routine_fires_multiple_expired_watchdogs(void) {
    watchdogs_api_enable_pool(&watchdogs_handler, 0U);
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_2, 0U);

    watchdogs_api_routine(&watchdogs_handler, watchdog_1.timeout);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_1_fake.call_count, "Callback 1 should have been called");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_2_fake.call_count, "Callback 2 should have been called");
}

void test_watchdogs_routine_does_not_fire_non_expired_watchdog(void) {
    watchdogs_api_enable_pool(&watchdogs_handler, 0U);
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_3, 0U);

    watchdogs_api_routine(&watchdogs_handler, watchdog_1.timeout);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_1_fake.call_count, "Callback 1 should have been called");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, watchdog_callback_3_fake.call_count, "Callback 3 should not have been called (longer timeout)");
}

void test_watchdogs_routine_updates_prev_tick(void) {
    watchdogs_handler.watchdog_module_enabled = true;

    watchdogs_api_routine(&watchdogs_handler, 20U);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(20U, watchdogs_handler.prev_tick, "prev_tick should be updated after routine");
}

void test_watchdogs_routine_removes_timed_out_watchdogs_from_heap(void) {
    watchdogs_api_enable_pool(&watchdogs_handler, 0U);
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    watchdogs_api_routine(&watchdogs_handler, watchdog_1.timeout);

    TEST_ASSERT_TRUE_MESSAGE(min_heap_api_is_empty(&watchdogs_handler.scheduled_watchdogs), "Heap should be empty after processing timed out watchdog");
}

void test_watchdogs_enable_pool_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_enable_pool(NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_enable_pool_with_past_tick_returns_temporal_discontinuity(void) {
    watchdogs_handler.prev_tick = 10U;

    enum WatchdogReturnCode rc = watchdogs_api_enable_pool(&watchdogs_handler, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_watchdogs_enable_pool_sets_enabled_flag(void) {
    watchdogs_handler.watchdog_module_enabled = false;

    watchdogs_api_enable_pool(&watchdogs_handler, 0U);

    TEST_ASSERT_TRUE_MESSAGE(watchdogs_handler.watchdog_module_enabled, "Module should be enabled");
}

void test_watchdogs_enable_pool_adjusts_next_trigger_times(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);
    uint32_t original_trigger = watchdog_1.next_trigger;
    watchdogs_handler.watchdog_module_enabled = false;
    watchdogs_handler.prev_tick = 0U;

    watchdogs_api_enable_pool(&watchdogs_handler, 10U);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(original_trigger + 10U, watchdog_1.next_trigger, "next_trigger should be shifted by the elapsed frozen time");
}

void test_watchdogs_disable_pool_with_null_handler_returns_error(void) {
    enum WatchdogReturnCode rc = watchdogs_api_disable_pool(NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_watchdogs_disable_pool_with_past_tick_returns_temporal_discontinuity(void) {
    watchdogs_handler.prev_tick = 10U;

    enum WatchdogReturnCode rc = watchdogs_api_disable_pool(&watchdogs_handler, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(WATCHDOG_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_watchdogs_disable_pool_clears_enabled_flag(void) {
    watchdogs_handler.watchdog_module_enabled = true;

    watchdogs_api_disable_pool(&watchdogs_handler, 0U);

    TEST_ASSERT_FALSE_MESSAGE(watchdogs_handler.watchdog_module_enabled, "Module should be disabled");
}

void test_watchdogs_disable_pool_updates_prev_tick(void) {
    watchdogs_handler.watchdog_module_enabled = true;
    watchdogs_handler.prev_tick = 0U;

    watchdogs_api_disable_pool(&watchdogs_handler, 50U);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(50U, watchdogs_handler.prev_tick, "prev_tick should be updated when module is disabled");
}

void test_watchdogs_disable_then_enable_freezes_time_correctly(void) {
    watchdogs_api_enable_pool(&watchdogs_handler, 0U);
    watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, 0U);

    watchdogs_api_disable_pool(&watchdogs_handler, 5U);
    watchdogs_api_enable_pool(&watchdogs_handler, 15U);

    watchdogs_api_routine(&watchdogs_handler, watchdog_1.timeout + 9U);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, watchdog_callback_1_fake.call_count, "Callback should not fire while time was frozen");

    watchdogs_api_routine(&watchdogs_handler, watchdog_1.timeout + 10U);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, watchdog_callback_1_fake.call_count, "Callback should fire after accounting for frozen time");
}

void setUp(void) {
    memset(&watchdogs_handler, 0, sizeof(watchdogs_handler));
    memset(&watchdog_1, 0, sizeof(watchdog_1));
    memset(&watchdog_2, 0, sizeof(watchdog_2));
    memset(&watchdog_3, 0, sizeof(watchdog_3));

    watchdogs_api_init_pool(&watchdogs_handler, 0U);
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
    RUN_TEST(test_watchdogs_start_paused_watchdog_resumes_with_remaining_time);

    // STOP TESTS
    RUN_TEST(test_watchdogs_stop_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_stop_with_null_watchdog_returns_error);
    RUN_TEST(test_watchdogs_stop_uninitialized_watchdog_returns_error);
    RUN_TEST(test_watchdogs_stop_not_running_watchdog_returns_error);
    RUN_TEST(test_watchdogs_stop_timed_out_watchdog_returns_error);
    RUN_TEST(test_watchdogs_stop_running_watchdog_returns_ok_and_sets_state);
    RUN_TEST(test_watchdogs_stop_running_watchdog_removes_from_heap);
    RUN_TEST(test_watchdogs_stop_paused_watchdog_returns_ok_without_heap_removal);

    // PAUSE TESTS
    RUN_TEST(test_watchdogs_pause_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_pause_with_null_watchdog_returns_error);
    RUN_TEST(test_watchdogs_pause_uninitialized_watchdog_returns_error);
    RUN_TEST(test_watchdogs_pause_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_watchdogs_pause_not_running_watchdog_returns_error);
    RUN_TEST(test_watchdogs_pause_timed_out_watchdog_returns_error);
    RUN_TEST(test_watchdogs_pause_running_watchdog_returns_ok_and_sets_state);
    RUN_TEST(test_watchdogs_pause_running_watchdog_removes_from_heap);

    // RESTART TESTS
    RUN_TEST(test_watchdogs_restart_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_restart_with_null_watchdog_returns_error);
    RUN_TEST(test_watchdogs_restart_uninitialized_watchdog_returns_error);
    RUN_TEST(test_watchdogs_restart_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_watchdogs_restart_not_running_watchdog_starts_it);
    RUN_TEST(test_watchdogs_restart_running_watchdog_resets_timer);
    RUN_TEST(test_watchdogs_restart_timed_out_watchdog_restarts_it);

    // PET TESTS
    RUN_TEST(test_watchdogs_pet_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_pet_with_null_watchdog_returns_error);
    RUN_TEST(test_watchdogs_pet_uninitialized_watchdog_returns_error);
    RUN_TEST(test_watchdogs_pet_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_watchdogs_pet_not_running_watchdog_returns_error);
    RUN_TEST(test_watchdogs_pet_timed_out_watchdog_returns_error);
    RUN_TEST(test_watchdogs_pet_running_watchdog_resets_timer);

    // FORCE TIMEOUT TESTS
    RUN_TEST(test_watchdogs_timeout_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_timeout_with_null_watchdog_returns_error);
    RUN_TEST(test_watchdogs_timeout_uninitialized_watchdog_returns_error);
    RUN_TEST(test_watchdogs_timeout_not_running_watchdog_returns_error);
    RUN_TEST(test_watchdogs_timeout_running_watchdog_fires_callback_and_sets_state);
    RUN_TEST(test_watchdogs_timeout_running_watchdog_removes_from_heap);

    // IS_RUNNING / IS_TIMED_OUT TESTS
    RUN_TEST(test_watchdogs_is_running_null_returns_false);
    RUN_TEST(test_watchdogs_is_running_not_running_returns_false);
    RUN_TEST(test_watchdogs_is_running_running_returns_true);
    RUN_TEST(test_watchdogs_is_timed_out_null_returns_false);
    RUN_TEST(test_watchdogs_is_timed_out_running_returns_false);
    RUN_TEST(test_watchdogs_is_timed_out_after_timeout_returns_true);

    // ROUTINE TESTS
    RUN_TEST(test_watchdogs_routine_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_routine_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_watchdogs_routine_with_module_disabled_returns_not_running);
    RUN_TEST(test_watchdogs_routine_with_empty_heap_returns_ok);
    RUN_TEST(test_watchdogs_routine_does_not_fire_before_timeout);
    RUN_TEST(test_watchdogs_routine_fires_callback_at_timeout);
    RUN_TEST(test_watchdogs_routine_fires_multiple_expired_watchdogs);
    RUN_TEST(test_watchdogs_routine_does_not_fire_non_expired_watchdog);
    RUN_TEST(test_watchdogs_routine_updates_prev_tick);
    RUN_TEST(test_watchdogs_routine_removes_timed_out_watchdogs_from_heap);

    // ENABLE MODULE TESTS
    RUN_TEST(test_watchdogs_enable_pool_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_enable_pool_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_watchdogs_enable_pool_sets_enabled_flag);
    RUN_TEST(test_watchdogs_enable_pool_adjusts_next_trigger_times);

    // DISABLE MODULE TESTS
    RUN_TEST(test_watchdogs_disable_pool_with_null_handler_returns_error);
    RUN_TEST(test_watchdogs_disable_pool_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_watchdogs_disable_pool_clears_enabled_flag);
    RUN_TEST(test_watchdogs_disable_pool_updates_prev_tick);
    RUN_TEST(test_watchdogs_disable_then_enable_freezes_time_correctly);

    return UNITY_END();
}