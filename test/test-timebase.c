/*!
 * \file test-timebase.c
 * \date 2026-05-7
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 *
 * \brief Unit tests for the timebase module
 */

#include "unity.h"
#include "timebase-api.h"

void setUp(void) {
    timebase_api_init(1U);
}

void tearDown(void) {
}

void test_timebase_api_init_sets_requested_resolution(void) {
    enum TimebaseReturnCode rc;

    rc = timebase_api_init(25U);

    TEST_ASSERT_EQUAL_MESSAGE(TIMEBASE_RC_OK, rc, "Initialization failed");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(25U, timebase_get_resolution(), "Resolution not set correctly");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0U, timebase_get_tick(), "Tick count not initialized to zero");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0U, timebase_get_time(), "Time not initialized to zero");
}

void test_timebase_api_init_zero_resolution_defaults_to_one(void) {
    enum TimebaseReturnCode rc;

    rc = timebase_api_init(0U);

    TEST_ASSERT_EQUAL_MESSAGE(TIMEBASE_RC_OK, rc, "Initialization failed");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, timebase_get_resolution(), "Resolution not set correctly");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0U, timebase_get_tick(), "Tick count not initialized to zero");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0U, timebase_get_time(), "Time not initialized to zero");
}

void test_timebase_inc_tick_when_disabled_returns_disabled_and_does_not_increment(void) {
    enum TimebaseReturnCode rc;

    rc = timebase_inc_tick();

    TEST_ASSERT_EQUAL_MESSAGE(TIMEBASE_RC_DISABLED, rc, "Expected disabled return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0U, timebase_get_tick(), "Tick count incremented when disabled");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0U, timebase_get_time(), "Time incremented when disabled");
}

void test_timebase_inc_tick_when_enabled_increments_ticks(void) {
    enum TimebaseReturnCode rc;

    timebase_set_enable(true);
    rc = timebase_inc_tick();

    TEST_ASSERT_EQUAL_MESSAGE(TIMEBASE_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, timebase_get_tick(), "Tick count not incremented");
}

void test_timebase_get_time_scales_with_resolution(void) {
    enum TimebaseReturnCode rc;

    timebase_api_init(5U);
    timebase_set_enable(true);
    rc = timebase_inc_tick();
    TEST_ASSERT_EQUAL_MESSAGE(TIMEBASE_RC_OK, rc, "Expected OK return code");
    rc = timebase_inc_tick();

    TEST_ASSERT_EQUAL_MESSAGE(TIMEBASE_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(2U, timebase_get_tick(), "Tick count not incremented correctly");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(10U, timebase_get_time(), "Time not scaled correctly with resolution");
}

void test_timebase_disable_after_increment_stops_future_increments(void) {
    enum TimebaseReturnCode rc;

    timebase_set_enable(true);
    rc = timebase_inc_tick();
    TEST_ASSERT_EQUAL_MESSAGE(TIMEBASE_RC_OK, rc, "Expected OK return code");
    timebase_set_enable(false);
    rc = timebase_inc_tick();

    TEST_ASSERT_EQUAL_MESSAGE(TIMEBASE_RC_DISABLED, rc, "Expected disabled return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, timebase_get_tick(), "Tick count incremented when disabled");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, timebase_get_time(), "Time incremented when disabled");
}

void test_timebase_reinit_clears_ticks_and_disables_counter(void) {
    enum TimebaseReturnCode rc;

    timebase_set_enable(true);
    rc = timebase_inc_tick();
    TEST_ASSERT_EQUAL_MESSAGE(TIMEBASE_RC_OK, rc, "Expected OK return code");
    rc = timebase_api_init(7U);
    TEST_ASSERT_EQUAL_MESSAGE(TIMEBASE_RC_OK, rc, "Expected OK return code");
    rc = timebase_inc_tick();

    TEST_ASSERT_EQUAL_MESSAGE(TIMEBASE_RC_DISABLED, rc, "Expected disabled return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(7U, timebase_get_resolution(), "Resolution not set correctly");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0U, timebase_get_tick(), "Tick count not cleared");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0U, timebase_get_time(), "Time not cleared");
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_timebase_api_init_sets_requested_resolution);
    RUN_TEST(test_timebase_api_init_zero_resolution_defaults_to_one);
    RUN_TEST(test_timebase_inc_tick_when_disabled_returns_disabled_and_does_not_increment);
    RUN_TEST(test_timebase_inc_tick_when_enabled_increments_ticks);
    RUN_TEST(test_timebase_get_time_scales_with_resolution);
    RUN_TEST(test_timebase_disable_after_increment_stops_future_increments);
    RUN_TEST(test_timebase_reinit_clears_ticks_and_disables_counter);
    return UNITY_END();
}