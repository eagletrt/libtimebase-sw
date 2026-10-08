/*!
 * \file test-tasks.c
 * \date 2026-05-8
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 *
 * \brief Unit tests for the tasks scheduler module
 */

#include <unity.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "min-heap-api.h"
#include "arena-allocator-api.h"
#include "fff.h"
DEFINE_FFF_GLOBALS;
#include "tasks-api.h"

struct TasksHandler tasks_handler;

/* The tick source of the module under test: tests move time by writing current_tick */
static uint32_t current_tick;
static uint32_t get_tick_calls;

static uint32_t fake_get_tick(void) {
    ++get_tick_calls;
    return current_tick;
}

FAKE_VOID_FUNC(function_1, uint8_t);
FAKE_VOID_FUNC(function_2, uint8_t);
FAKE_VOID_FUNC(function_3, uint8_t);
FAKE_VOID_FUNC(function_4, uint8_t);

enum TasksNames {
    TASK_1 = 0,
    TASK_2,
    TASK_3,
    TASK_4,
    TASK_COUNT
};

TaskList t_list = {
    { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
    { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
    { .task_id = TASK_3, .state = TASKS_STATE_ENABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
    { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
};

// Function definition for private functions to be tested
enum TasksReturnCode prv_handle_task_transition(struct TasksHandler *tasks_handler, int8_t task_id, uint32_t tick);
enum TasksReturnCode prv_tasks_update_heap(struct TasksHandler *tasks_handler, uint32_t current_tick);

void test_tasks_init_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_init(NULL, t_list, TASK_COUNT, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_init_with_null_tick_callback_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_init(&tasks_handler, t_list, TASK_COUNT, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_init_stores_tick_callback(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_init(&tasks_handler, t_list, TASK_COUNT, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_TRUE_MESSAGE(tasks_handler.get_tick == fake_get_tick, "The tick callback should be stored in the handler");
}

void test_tasks_init_uses_the_tick_returned_by_the_callback(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 5U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 100U;
    rc = tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(100U, tasks_handler.prev_tick, "prev_tick should be the tick returned by the callback");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(105U, tasks_handler.task_list[TASK_1].next_trigger, "Enabled tasks should be scheduled from the tick returned by the callback");
}

void test_tasks_init_with_null_list_returns_error(void) {
    enum TasksReturnCode rc;

    current_tick = 0U;
    rc = tasks_api_init(&tasks_handler, NULL, TASK_COUNT, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_init_with_invalid_list_item_state_returns_error(void) {
    enum TasksReturnCode rc;

    TaskList invalid_list = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_PAUSED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_ENABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    rc = tasks_api_init(&tasks_handler, invalid_list, TASK_COUNT, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_LIST, rc, "Expected INVALID_LIST return code");
}

void test_tasks_init_with_invalid_list_item_id_returns_error(void) {
    enum TasksReturnCode rc;

    TaskList invalid_list = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = 5U, .state = TASKS_STATE_ENABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    rc = tasks_api_init(&tasks_handler, invalid_list, TASK_COUNT, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_LIST, rc, "Expected INVALID_LIST return code");
}

void test_tasks_init_with_zero_tasks_returns_error(void) {
    enum TasksReturnCode rc;

    current_tick = 0U;
    rc = tasks_api_init(&tasks_handler, t_list, 0U, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_LIST, rc, "Expected INVALID_LIST return code");
}

void test_tasks_init_with_invalid_count_returns_error(void) {
    enum TasksReturnCode rc;

    current_tick = 0U;
    rc = tasks_api_init(&tasks_handler, t_list, MAX_TASKS + 1U, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_LIST, rc, "Expected INVALID_LIST return code");
}

void test_tasks_init_with_valid_list_initializes_tasks_handler(void) {
    enum TasksReturnCode rc;

    current_tick = 0U;
    rc = tasks_api_init(&tasks_handler, t_list, TASK_COUNT, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_COUNT, tasks_handler.task_num, "Number of tasks not set correctly");

    t_list[2].next_trigger = 10U; // The next trigger time is set to start for enabled tasks during initialization
    t_list[2].int_repeats = 1U;   // The int_repeats field is set to the value of repeats during initialization

    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&t_list[0], &tasks_handler.task_list[0], sizeof(struct Task), "Task 1 not copied correctly");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&t_list[1], &tasks_handler.task_list[1], sizeof(struct Task), "Task 2 not copied correctly");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&t_list[2], &tasks_handler.task_list[2], sizeof(struct Task), "Task 3 not copied correctly");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&t_list[3], &tasks_handler.task_list[3], sizeof(struct Task), "Task 4 not copied correctly");
}

void test_tasks_init_with_valid_list_initializes_heap(void) {
    enum TasksReturnCode rc;

    current_tick = 0U;
    rc = tasks_api_init(&tasks_handler, t_list, TASK_COUNT, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_FALSE_MESSAGE(min_heap_api_is_empty(&tasks_handler.scheduled_tasks), "Scheduled tasks heap should not be empty after initialization");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, tasks_handler.scheduled_tasks.size, "Scheduled tasks heap should contain 2 tasks after initialization");
    struct Task *next_task;
    if (min_heap_api_remove(&tasks_handler.scheduled_tasks, 0U, &next_task) == MIN_HEAP_RC_OK) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_1, next_task->task_id, "First task in heap should be Task 1");
    } else {
        TEST_FAIL_MESSAGE("Failed to remove task from heap");
    }
}

void test_tasks_init_without_function_original_states_defaults_disabled(void) {
    enum TasksReturnCode rc;

    TaskList invalid_list = {
        { .task_id = TASK_1, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    rc = tasks_api_init(&tasks_handler, invalid_list, TASK_COUNT, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    for (uint8_t i = 0; i < TASK_COUNT; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_DISABLED, tasks_handler.task_list[i].state, "Task state should be set to DISABLED when not initialized");
    }
}

void test_tasks_init_without_repeats_defaults_to_infinite(void) {
    enum TasksReturnCode rc;

    TaskList list = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_ENABLED, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    rc = tasks_api_init(&tasks_handler, list, TASK_COUNT, fake_get_tick);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    for (uint8_t i = 0; i < TASK_COUNT; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, tasks_handler.task_list[i].int_repeats, "int_repeats should default to 0 (infinite) when not initialized");
    }
}

void test_tasks_handle_task_transition_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = prv_handle_task_transition(NULL, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_handle_task_transition_with_invalid_id_returns_error(void) {
    enum TasksReturnCode rc;
    rc = prv_handle_task_transition(&tasks_handler, MAX_TASKS + 1U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_ID, rc, "Expected INVALID_ID return code");
}

void test_tasks_handle_task_transition_with_valid_id_returns_ok(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    tasks_handler.task_list[0].state = TASKS_STATE_ENABLED;

    rc = prv_handle_task_transition(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, tasks_handler.actual_state[0], "Task state should be updated to ENABLED");
}

void test_tasks_handle_task_transition_with_valid_id_adds_and_removes_from_heap(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    tasks_handler.task_list[0].state = TASKS_STATE_ENABLED;

    rc = prv_handle_task_transition(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_FALSE_MESSAGE(min_heap_api_is_empty(&tasks_handler.scheduled_tasks), "Scheduled tasks heap should not be empty after enabling task");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, tasks_handler.scheduled_tasks.size, "Scheduled tasks heap should contain 1 task after enabling task");

    tasks_handler.task_list[0].state = TASKS_STATE_DISABLED;

    rc = prv_handle_task_transition(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_TRUE_MESSAGE(min_heap_api_is_empty(&tasks_handler.scheduled_tasks), "Scheduled tasks heap should be empty after disabling task");
}

void test_tasks_handle_task_transition_with_valid_id_updates_trigger_time(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 10U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    tasks_handler.task_list[0].state = TASKS_STATE_ENABLED;

    rc = prv_handle_task_transition(&tasks_handler, 0U, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    struct Task *next_task;
    if (min_heap_api_remove(&tasks_handler.scheduled_tasks, 0U, &next_task) == MIN_HEAP_RC_OK) {
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(15U, next_task->next_trigger, "Next trigger time should be current tick + task start");
    } else {
        TEST_FAIL_MESSAGE("Failed to remove task from heap");
    }
}

void test_tasks_handle_task_transition_with_valid_id_updates_trigger_time_on_resume(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 10U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    tasks_handler.task_list[0].state = TASKS_STATE_ENABLED;
    tasks_handler.actual_state[0] = TASKS_STATE_PAUSED;
    tasks_handler.task_list[0].next_trigger = 20U;
    tasks_handler.task_list[0].last_update = 10U;

    rc = prv_handle_task_transition(&tasks_handler, 0U, 15U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    struct Task *next_task;
    if (min_heap_api_remove(&tasks_handler.scheduled_tasks, 0U, &next_task) == MIN_HEAP_RC_OK) {
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(25U, next_task->next_trigger, "Next trigger time should be updated according to the time the task was paused");
    } else {
        TEST_FAIL_MESSAGE("Failed to remove task from heap");
    }
}

void test_tasks_handle_task_transition_with_valid_id_and_no_state_change_returns_ok(void) {
    enum TasksReturnCode rc;
    tasks_handler.task_num = TASK_COUNT;
    tasks_handler.actual_state[0] = TASKS_STATE_ENABLED;
    tasks_handler.task_list[0].state = TASKS_STATE_ENABLED;

    rc = prv_handle_task_transition(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, tasks_handler.actual_state[0], "Task state should remain ENABLED");
}

void test_tasks_handle_task_transition_with_valid_id_and_no_state_change_does_not_modify_heap(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);
    tasks_handler.task_num = TASK_COUNT;
    tasks_handler.actual_state[0] = TASKS_STATE_ENABLED;
    tasks_handler.task_list[0].state = TASKS_STATE_ENABLED;

    rc = prv_handle_task_transition(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_FALSE_MESSAGE(min_heap_api_is_empty(&tasks_handler.scheduled_tasks), "Scheduled tasks heap should not be empty");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, tasks_handler.scheduled_tasks.size, "Scheduled tasks heap should still contain 1 task");
}

void test_tasks_update_heap_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = prv_tasks_update_heap(NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_update_heap_with_correct_parameters_returns_ok(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 10U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    tasks_handler.task_list[0].state = TASKS_STATE_ENABLED;

    rc = prv_tasks_update_heap(&tasks_handler, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    struct Task *next_task;
    if (min_heap_api_remove(&tasks_handler.scheduled_tasks, 0U, &next_task) == MIN_HEAP_RC_OK) {
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(15U, next_task->next_trigger, "Next trigger time should be current tick + task start");
    } else {
        TEST_FAIL_MESSAGE("Failed to remove task from heap");
    }
}

void test_tasks_routine_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_routine(NULL);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_routine_with_no_tasks_returns_error(void) {
    enum TasksReturnCode rc;

    tasks_handler.task_num = 0U;

    current_tick = 0U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_ERROR, rc, "Expected ERROR return code");
}

void test_tasks_routine_with_past_tick_returns_temporal_discontinuity(void) {
    enum TasksReturnCode rc;

    tasks_handler.prev_tick = 10U;

    current_tick = 5U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_tasks_routine_with_valid_parameters_executes_tasks_immediately(void) {
    enum TasksReturnCode rc;

    current_tick = 0U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task function should have been called once");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, function_2_fake.call_count, "Task function 2 should not have been called");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, function_3_fake.call_count, "Task function 3 should not have been called");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, function_4_fake.call_count, "Task function 4 should not have been called");
}

void test_tasks_routine_with_valid_parameters_executes_tasks_after_interval(void) {
    enum TasksReturnCode rc;

    current_tick = 4U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task function should not have been called yet");

    current_tick = 12U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, function_1_fake.call_count, "Task function should have been called once after interval");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, function_2_fake.call_count, "Task function 2 should not have been called");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_3_fake.call_count, "Task function 3 should not have been called");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, function_4_fake.call_count, "Task function 4 should not have been called");
}

void test_tasks_routine_with_valid_parameters_executes_repeats_task_exact_times(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_ENABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 5U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 4U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, function_3_fake.call_count, "Task function should not have been called yet");

    current_tick = 12U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_3_fake.call_count, "Task function should have been called once after interval");

    current_tick = 35U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_3_fake.call_count, "Repeats=1 task function should not have been called again");
}

void test_tasks_routine_with_valid_parameters_executes_repeats_task_after_reenabled(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_ENABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 5U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 12U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_3_fake.call_count, "Task function should have been called once after interval");

    current_tick = 20U;
    tasks_api_enable_task(&tasks_handler, TASK_3);

    current_tick = 24U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_3_fake.call_count, "Repeats=1 task function should not have been called again before timeout");

    current_tick = 30U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, function_3_fake.call_count, "Repeats=1 task function should have been called again after being reenabled and reaching timeout");
}

void test_tasks_routine_with_interval_set_to_zero_treats_as_one(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 0U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task function should have been called once immediately");

    current_tick = 1U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, function_1_fake.call_count, "Task function should have been called again after interval treated as 1");
}

void test_tasks_routine_with_valid_parameters_executes_repeats_N_times(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 3U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task should have fired once at tick 0");

    current_tick = 10U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, function_1_fake.call_count, "Task should have fired twice at tick 10");

    current_tick = 20U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3U, function_1_fake.call_count, "Task should have fired three times at tick 20");

    current_tick = 30U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3U, function_1_fake.call_count, "Task should not fire again after repeats exhausted");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_DISABLED, tasks_handler.actual_state[TASK_1], "Task should be disabled after repeats exhausted");
}

void test_tasks_enable_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_enable_task(NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_enable_with_invalid_id_returns_error(void) {
    enum TasksReturnCode rc;

    current_tick = 0U;
    rc = tasks_api_enable_task(&tasks_handler, MAX_TASKS + 1U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_ID, rc, "Expected INVALID_ID return code");
}

void test_tasks_enable_with_past_tick_returns_temporal_discontinuity(void) {
    enum TasksReturnCode rc;

    tasks_handler.prev_tick = 10U;

    current_tick = 5U;
    rc = tasks_api_enable_task(&tasks_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_tasks_enable_with_already_enabled_task_returns_ok(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_enable_task(&tasks_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, tasks_handler.actual_state[0], "Task state should remain ENABLED");
}

void test_tasks_enable_with_valid_id_enables_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_enable_task(&tasks_handler, 1U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, tasks_handler.actual_state[1], "Task state should be updated to ENABLED");
    struct Task *task_ptr = &tasks_handler.task_list[1];
    if (min_heap_api_find(&tasks_handler.scheduled_tasks, &task_ptr) < 0) {
        TEST_FAIL_MESSAGE("Enabled task should be in the scheduled tasks heap");
    }
}

void test_tasks_enable_after_pause_resumes_task_with_correct_trigger_time(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    tasks_handler.actual_state[1] = TASKS_STATE_PAUSED;
    tasks_handler.task_list[1].next_trigger = 20U;
    tasks_handler.task_list[1].last_update = 10U;

    current_tick = 15U;
    rc = tasks_api_enable_task(&tasks_handler, 1U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    struct Task *next_task;
    if (min_heap_api_remove(&tasks_handler.scheduled_tasks, 0U, &next_task) == MIN_HEAP_RC_OK) {
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(25U, next_task->next_trigger, "Next trigger time should be updated according to the time the task was paused");
    } else {
        TEST_FAIL_MESSAGE("Failed to remove task from heap");
    }
}

void test_tasks_enable_after_disable_restarts_task_with_correct_trigger_time(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    tasks_handler.actual_state[1] = TASKS_STATE_DISABLED;
    tasks_handler.task_list[1].next_trigger = 20U;
    tasks_handler.task_list[1].last_update = 10U;

    current_tick = 15U;
    rc = tasks_api_enable_task(&tasks_handler, 1U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    struct Task *next_task;
    if (min_heap_api_remove(&tasks_handler.scheduled_tasks, 0U, &next_task) == MIN_HEAP_RC_OK) {
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(20U, next_task->next_trigger, "Next trigger time should be reset to current tick + task start");
    } else {
        TEST_FAIL_MESSAGE("Failed to remove task from heap");
    }
}

void test_tasks_pause_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_pause_task(NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_pause_with_invalid_id_returns_error(void) {
    enum TasksReturnCode rc;

    current_tick = 0U;
    rc = tasks_api_pause_task(&tasks_handler, MAX_TASKS + 1U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_ID, rc, "Expected INVALID_ID return code");
}

void test_tasks_pause_with_past_tick_returns_temporal_discontinuity(void) {
    enum TasksReturnCode rc;

    tasks_handler.prev_tick = 10U;

    current_tick = 5U;
    rc = tasks_api_pause_task(&tasks_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_tasks_pause_with_already_paused_task_returns_ok(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    tasks_handler.actual_state[0] = TASKS_STATE_PAUSED;

    current_tick = 0U;
    rc = tasks_api_pause_task(&tasks_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_PAUSED, tasks_handler.actual_state[0], "Task state should remain PAUSED");
}

void test_tasks_pause_with_valid_id_pauses_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_pause_task(&tasks_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_PAUSED, tasks_handler.actual_state[0], "Task state should be updated to PAUSED");
    struct Task *task_ptr = &tasks_handler.task_list[0];
    if (min_heap_api_find(&tasks_handler.scheduled_tasks, &task_ptr) >= 0) {
        TEST_FAIL_MESSAGE("Paused task should NOT be in the scheduled tasks heap");
    }
}

void test_tasks_pause_repeats_task_before_start_resumes_correctly(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 1U, .function = function_1, .interval = 10U, .start = 10U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 5U;
    tasks_api_pause_task(&tasks_handler, 0U);

    current_tick = 10U;
    rc = tasks_api_enable_task(&tasks_handler, 0U);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, tasks_handler.actual_state[0], "Task state should be updated to ENABLED");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(15U, tasks_handler.task_list[0].next_trigger, "Next trigger time should be updated to current tick + remaining time until start");
    TEST_ASSERT_FALSE_MESSAGE(min_heap_api_is_empty(&tasks_handler.scheduled_tasks), "Scheduled tasks heap should not be empty after resuming task");
}

void test_tasks_pause_repeats_task_after_start_resumes_correctly(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 10U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 15U;
    tasks_api_pause_task(&tasks_handler, 0U);

    current_tick = 20U;
    rc = tasks_api_enable_task(&tasks_handler, 0U);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, tasks_handler.actual_state[0], "Task state should be updated to ENABLED");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(30U, tasks_handler.task_list[0].next_trigger, "Next trigger time should be updated to current tick + remaining time until next trigger");
    TEST_ASSERT_FALSE_MESSAGE(min_heap_api_is_empty(&tasks_handler.scheduled_tasks), "Scheduled tasks heap should not be empty after resuming task");
}

void test_tasks_pause_mid_count_preserves_remaining_repeats(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 3U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    // Fire once: repeats countdown goes from 3 to 2
    current_tick = 0U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task should have fired once");

    // Pause mid-interval: 5 ticks remaining until next trigger (next_trigger=10, last_update=0, pause at 5)
    current_tick = 5U;
    tasks_api_pause_task(&tasks_handler, TASK_1);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_PAUSED, tasks_handler.actual_state[TASK_1], "Task should be paused");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, tasks_handler.task_list[TASK_1].repeats, "Repeat count should be 2 after one execution");

    // Resume: remaining 5 ticks are preserved, next trigger = 8 + 5 = 13
    current_tick = 8U;
    rc = tasks_api_enable_task(&tasks_handler, TASK_1);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(13U, tasks_handler.task_list[TASK_1].next_trigger, "Next trigger should reflect preserved remaining time");

    // Fire second time
    current_tick = 13U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, function_1_fake.call_count, "Task should have fired a second time");

    // Fire third and final time
    current_tick = 23U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3U, function_1_fake.call_count, "Task should have fired a third time");

    // Should not fire again
    current_tick = 33U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3U, function_1_fake.call_count, "Task should not fire after repeats exhausted");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_DISABLED, tasks_handler.actual_state[TASK_1], "Task should be disabled after repeats exhausted");
}

void test_tasks_pause_disabled_task_returns_error(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_pause_task(&tasks_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_ERROR, rc, "Expected ERROR return code");
}

void test_tasks_disable_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_disable_task(NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_disable_with_invalid_id_returns_error(void) {
    enum TasksReturnCode rc;

    current_tick = 0U;
    rc = tasks_api_disable_task(&tasks_handler, MAX_TASKS + 1U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_ID, rc, "Expected INVALID_ID return code");
}

void test_tasks_disable_with_past_tick_returns_temporal_discontinuity(void) {
    enum TasksReturnCode rc;

    tasks_handler.prev_tick = 10U;

    current_tick = 5U;
    rc = tasks_api_disable_task(&tasks_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_tasks_disable_with_already_disabled_task_returns_ok(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_disable_task(&tasks_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_DISABLED, tasks_handler.actual_state[0], "Task state should remain DISABLED");
}

void test_tasks_disable_with_valid_id_disables_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_disable_task(&tasks_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_DISABLED, tasks_handler.actual_state[0], "Task state should be updated to DISABLED");
    struct Task *task_ptr = &tasks_handler.task_list[0];
    if (min_heap_api_find(&tasks_handler.scheduled_tasks, &task_ptr) >= 0) {
        TEST_FAIL_MESSAGE("Disabled task should NOT be in the scheduled tasks heap");
    }
}

void test_tasks_update_task_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_update_task(NULL, 0U, 10U, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_update_task_with_invalid_id_returns_error(void) {
    enum TasksReturnCode rc;

    current_tick = 0U;
    rc = tasks_api_update_task(&tasks_handler, MAX_TASKS + 1U, 10U, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_ID, rc, "Expected INVALID_ID return code");
}

void test_tasks_update_task_with_past_tick_returns_temporal_discontinuity(void) {
    enum TasksReturnCode rc;

    tasks_handler.prev_tick = 10U;

    current_tick = 5U;
    rc = tasks_api_update_task(&tasks_handler, 0U, 10U, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_tasks_update_task_with_valid_id_updates_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 5U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 10U;
    rc = tasks_api_update_task(&tasks_handler, 0U, 20U, 10U, 1U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(20U, tasks_handler.task_list[0].interval, "Task interval should be updated");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(10U, tasks_handler.task_list[0].start, "Task start should be updated");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, tasks_handler.task_list[0].int_repeats, "Task int_repeats should be updated to 1");

    // Check if the task is updated in the heap
    struct Task *next_task;
    if (min_heap_api_remove(&tasks_handler.scheduled_tasks, 0U, &next_task) == MIN_HEAP_RC_OK) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, next_task->task_id, "Updated task should be in the heap");
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(20U, next_task->interval, "Task interval in heap should be updated");
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(10U, next_task->start, "Task start in heap should be updated");
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, next_task->int_repeats, "Task int_repeats in heap should be updated to 1");
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(20U, next_task->next_trigger, "Next trigger time should be updated according to new start and interval");
    } else {
        TEST_FAIL_MESSAGE("Failed to remove task from heap");
    }
}

void test_tasks_update_task_with_valid_id_and_disabled_task_updates_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 10U;
    rc = tasks_api_update_task(&tasks_handler, 1U, 30U, 10U, 1U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(30U, tasks_handler.task_list[1].interval, "Task interval should be updated");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(10U, tasks_handler.task_list[1].start, "Task start should be updated");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, tasks_handler.task_list[1].int_repeats, "Task int_repeats should be updated to 1");
}

void test_tasks_update_task_with_valid_id_and_paused_task_updates_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    tasks_api_pause_task(&tasks_handler, 0U);

    current_tick = 10U;
    rc = tasks_api_update_task(&tasks_handler, 0U, 20U, 10U, 1U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(20U, tasks_handler.task_list[0].interval, "Task interval should be updated");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(10U, tasks_handler.task_list[0].start, "Task start should be updated");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, tasks_handler.task_list[0].int_repeats, "Task int_repeats should be updated to 1");

    // Check that the task is not in the heap since it is paused
    struct Task *task_ptr = &tasks_handler.task_list[0];
    if (min_heap_api_find(&tasks_handler.scheduled_tasks, &task_ptr) >= 0) {
        TEST_FAIL_MESSAGE("Paused task should NOT be in the scheduled tasks heap");
    }
}

void test_tasks_update_task_sets_repeats_to_arbitrary_value(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_update_task(&tasks_handler, TASK_1, 10U, 0U, 5U);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(5U, tasks_handler.task_list[TASK_1].int_repeats, "int_repeats should be set to 5 after update");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(5U, tasks_handler.task_list[TASK_1].repeats, "repeats countdown should be set to 5 after update");

    // Run 5 times and verify it fires exactly 5 times
    for (uint32_t i = 0; i < 5U; ++i) {
        current_tick = i * 10U;
        rc = tasks_api_routine(&tasks_handler);
        TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(i + 1U, function_1_fake.call_count, "Task should have fired once per iteration");
    }

    current_tick = 50U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(5U, function_1_fake.call_count, "Task should not fire after 5 repeats exhausted");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_DISABLED, tasks_handler.actual_state[TASK_1], "Task should be disabled after repeats exhausted");
}

void test_tasks_get_task_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;
    struct Task task;

    rc = tasks_api_get_task(NULL, 0U, &task);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_get_task_with_invalid_id_returns_error(void) {
    enum TasksReturnCode rc;
    struct Task task;

    rc = tasks_api_get_task(&tasks_handler, MAX_TASKS + 1U, &task);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_ID, rc, "Expected INVALID_ID return code");
}

void test_tasks_get_task_with_valid_id_returns_ok_and_copies_task(void) {
    enum TasksReturnCode rc;
    struct Task task;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 5U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    rc = tasks_api_get_task(&tasks_handler, 0U, &task);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, task.task_id, "Task ID should be copied correctly");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, task.state, "Task state should be copied correctly");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, task.int_repeats, "Task int_repeats should be copied correctly");
    TEST_ASSERT_EQUAL_PTR_MESSAGE(function_1, task.function, "Task function pointer should be copied correctly");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(10U, task.interval, "Task interval should be copied correctly");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(5U, task.start, "Task start should be copied correctly");
}

void test_tasks_routine_passes_task_id_to_callback(void) {
    enum TasksReturnCode rc;

    current_tick = 0U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task function 1 should have been called once");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_1, function_1_fake.arg0_val, "Callback should receive the ID of Task 1");
}

void test_tasks_routine_passes_correct_task_id_to_each_callback(void) {
    enum TasksReturnCode rc;

    current_tick = 4U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");

    current_tick = 12U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, function_1_fake.call_count, "Task function 1 should have been called twice");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_1, function_1_fake.arg0_history[0], "First call of Task 1 should receive TASK_1");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_1, function_1_fake.arg0_history[1], "Second call of Task 1 should receive TASK_1");

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_3_fake.call_count, "Task function 3 should have been called once");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_3, function_3_fake.arg0_val, "Callback of Task 3 should receive TASK_3");

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, function_2_fake.call_count, "Task function 2 should not have been called");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, function_4_fake.call_count, "Task function 4 should not have been called");
}

void test_tasks_routine_passes_non_zero_task_id_to_callback(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 0U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_2_fake.call_count, "Task function 2 should have been called once");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_2, function_2_fake.arg0_val, "Callback of Task 2 should receive TASK_2 (non-zero ID)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, function_1_fake.call_count, "Task function 1 should not have been called");
}

void test_tasks_routine_passes_own_task_id_to_all_callbacks_when_all_enabled(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 0U },
        { .task_id = TASK_3, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_3, .interval = 15U, .start = 0U },
        { .task_id = TASK_4, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task function 1 should have been called once");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_2_fake.call_count, "Task function 2 should have been called once");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_3_fake.call_count, "Task function 3 should have been called once");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_4_fake.call_count, "Task function 4 should have been called once");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_1, function_1_fake.arg0_val, "Callback of Task 1 should receive TASK_1");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_2, function_2_fake.arg0_val, "Callback of Task 2 should receive TASK_2");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_3, function_3_fake.arg0_val, "Callback of Task 3 should receive TASK_3");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_4, function_4_fake.arg0_val, "Callback of Task 4 should receive TASK_4");
}

void test_tasks_routine_passes_task_id_on_every_repeat(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 3U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    for (uint32_t i = 0; i < 3U; ++i) {
        current_tick = i * 10U;
        rc = tasks_api_routine(&tasks_handler);
        TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    }

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3U, function_1_fake.call_count, "Task should have fired three times");
    for (uint8_t i = 0; i < 3U; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_1, function_1_fake.arg0_history[i], "Every execution should receive TASK_1");
    }
}

void test_tasks_routine_passes_task_id_after_pause_and_resume(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 3U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task should have fired once");

    current_tick = 5U;
    tasks_api_pause_task(&tasks_handler, TASK_1);
    current_tick = 8U;
    rc = tasks_api_enable_task(&tasks_handler, TASK_1);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");

    current_tick = 13U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, function_1_fake.call_count, "Task should have fired again after resume");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_1, function_1_fake.arg0_history[1], "Callback after resume should receive TASK_1");
}

void test_tasks_routine_passes_task_id_after_reenable_from_disabled(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_enable_task(&tasks_handler, TASK_2);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");

    current_tick = 5U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_2_fake.call_count, "Enabled task should have been called once");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_2, function_2_fake.arg0_val, "Callback of Task 2 should receive TASK_2");
}

void test_tasks_routine_passes_task_id_after_update_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_update_task(&tasks_handler, TASK_1, 10U, 0U, 5U);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");

    for (uint32_t i = 0; i < 5U; ++i) {
        current_tick = i * 10U;
        rc = tasks_api_routine(&tasks_handler);
        TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    }

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(5U, function_1_fake.call_count, "Task should have fired five times");
    for (uint8_t i = 0; i < 5U; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_1, function_1_fake.arg0_history[i], "Every execution after update should receive TASK_1");
    }
}

void test_tasks_routine_with_interval_set_to_zero_passes_task_id(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 0U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    current_tick = 0U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");

    current_tick = 1U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, function_1_fake.call_count, "Task function should have been called twice");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_1, function_1_fake.arg0_history[0], "First call should receive TASK_1");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_1, function_1_fake.arg0_history[1], "Second call should receive TASK_1");
}

/*
 * CALLBACK REENTRANCY TESTS
 *
 * A task callback must be able to change the state of its own task. To make this possible the routine reschedules
 * (or retires) the task BEFORE calling its callback, so that the callback finds a consistent handler.
 */

static enum TasksReturnCode callback_rc;

static void callback_disable_self(uint8_t task_id) {
    callback_rc = tasks_api_disable_task(&tasks_handler, task_id);
}

static void callback_pause_self(uint8_t task_id) {
    callback_rc = tasks_api_pause_task(&tasks_handler, task_id);
}

static void callback_enable_self(uint8_t task_id) {
    callback_rc = tasks_api_enable_task(&tasks_handler, task_id);
}

static void callback_update_self(uint8_t task_id) {
    callback_rc = tasks_api_update_task(&tasks_handler, task_id, 20U, 3U, 0U);
}

static void callback_disable_task_2(uint8_t task_id) {
    (void)task_id;
    callback_rc = tasks_api_disable_task(&tasks_handler, TASK_2);
}

void test_tasks_routine_callback_can_disable_its_own_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    function_1_fake.custom_fake = callback_disable_self;

    current_tick = 0U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, callback_rc, "The disable called inside the callback should succeed");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task should have fired once");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_DISABLED, tasks_handler.task_list[TASK_1].state, "Task state should be DISABLED");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_DISABLED, tasks_handler.actual_state[TASK_1], "Actual state should be DISABLED");
    TEST_ASSERT_TRUE_MESSAGE(min_heap_api_is_empty(&tasks_handler.scheduled_tasks), "The disabled task should not be in the heap");

    current_tick = 10U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "A task that disabled itself should not fire again");
}

void test_tasks_routine_callback_can_pause_its_own_task_and_resume_with_remaining_time(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    function_1_fake.custom_fake = callback_pause_self;

    current_tick = 0U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, callback_rc, "The pause called inside the callback should succeed");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_PAUSED, tasks_handler.task_list[TASK_1].state, "Task state should be PAUSED and not overwritten by the routine");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_PAUSED, tasks_handler.actual_state[TASK_1], "Actual state should be PAUSED");
    TEST_ASSERT_TRUE_MESSAGE(min_heap_api_is_empty(&tasks_handler.scheduled_tasks), "The paused task should not be in the heap");

    function_1_fake.custom_fake = NULL;

    current_tick = 10U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "A paused task should not fire");

    // Paused with a full interval left (10 ticks): resuming at 20 must schedule the next call at 30
    current_tick = 20U;
    rc = tasks_api_enable_task(&tasks_handler, TASK_1);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(30U, tasks_handler.task_list[TASK_1].next_trigger, "The remaining time at the moment of the pause should be preserved");

    current_tick = 29U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task should not fire before the resumed trigger");

    current_tick = 30U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, function_1_fake.call_count, "Task should fire at the resumed trigger");
}

void test_tasks_routine_callback_can_update_its_own_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    function_1_fake.custom_fake = callback_update_self; // New schedule: start = 3, interval = 20

    current_tick = 0U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, callback_rc, "The update called inside the callback should succeed");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task should have fired once");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, tasks_handler.actual_state[TASK_1], "Task should still be ENABLED");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(3U, tasks_handler.task_list[TASK_1].next_trigger, "Task should be rescheduled according to the new start");

    function_1_fake.custom_fake = NULL;

    current_tick = 2U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task should not fire before the new start");

    // If the task was inserted twice in the heap it would fire twice in the same routine
    current_tick = 3U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, function_1_fake.call_count, "Task should fire exactly once at the new start");

    current_tick = 22U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, function_1_fake.call_count, "Task should not fire before the new interval has elapsed");

    current_tick = 23U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3U, function_1_fake.call_count, "Task should fire with the new interval");
}

void test_tasks_routine_callback_can_reenable_its_own_expired_one_shot_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 1U, .function = function_1, .interval = 10U, .start = 5U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    function_1_fake.custom_fake = callback_enable_self;

    current_tick = 5U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, callback_rc, "The enable called inside the callback should succeed");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task should have fired once");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, tasks_handler.task_list[TASK_1].state, "The routine should not disable a task that was re-enabled by its callback");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, tasks_handler.actual_state[TASK_1], "Actual state should be ENABLED");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(10U, tasks_handler.task_list[TASK_1].next_trigger, "Task should restart from its start time");

    function_1_fake.custom_fake = NULL;

    current_tick = 9U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task should not fire before the new start");

    current_tick = 10U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, function_1_fake.call_count, "Task should fire again after being re-enabled");

    current_tick = 30U;
    rc = tasks_api_routine(&tasks_handler);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, function_1_fake.call_count, "A one-shot task should fire only once per enable");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_DISABLED, tasks_handler.actual_state[TASK_1], "The expired task should end up DISABLED");
}

void test_tasks_routine_callback_can_disable_another_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_2, .interval = 10U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    function_1_fake.custom_fake = callback_disable_task_2;

    current_tick = 0U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, callback_rc, "Disabling another task from a callback should succeed");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_DISABLED, tasks_handler.actual_state[TASK_2], "The other task should be DISABLED");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, tasks_handler.actual_state[TASK_1], "The calling task should still be ENABLED");

    function_1_fake.custom_fake = NULL;

    current_tick = 10U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, function_1_fake.call_count, "The calling task should keep firing");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, function_2_fake.call_count, "The disabled task should never fire");
}

/*
 * TICK CALLBACK TESTS
 */

void test_tasks_api_with_uninitialized_handler_returns_null_pointer(void) {
    struct TasksHandler uninitialized_handler;
    memset(&uninitialized_handler, 0, sizeof(uninitialized_handler));

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, tasks_api_routine(&uninitialized_handler), "routine without a tick callback should not be executed");
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, tasks_api_enable_task(&uninitialized_handler, TASK_1), "enable without a tick callback should not be executed");
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, tasks_api_pause_task(&uninitialized_handler, TASK_1), "pause without a tick callback should not be executed");
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, tasks_api_disable_task(&uninitialized_handler, TASK_1), "disable without a tick callback should not be executed");
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, tasks_api_update_task(&uninitialized_handler, TASK_1, 10U, 0U, 0U), "update without a tick callback should not be executed");
}

void test_tasks_routine_reads_the_tick_only_once(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_2, .interval = 10U, .start = 0U },
        { .task_id = TASK_3, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_3, .interval = 10U, .start = 0U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    get_tick_calls = 0U;
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_1_fake.call_count, "Task 1 should have fired");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_2_fake.call_count, "Task 2 should have fired");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, function_3_fake.call_count, "Task 3 should have fired");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, get_tick_calls, "The routine should sample the tick once, no matter how many tasks are executed");
}

void test_tasks_control_functions_read_the_tick_once_at_call_time(void) {
    enum TasksReturnCode rc;

    current_tick = 7U;
    get_tick_calls = 0U;
    rc = tasks_api_enable_task(&tasks_handler, TASK_2);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, get_tick_calls, "enable should read the tick once");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(7U + t_list[TASK_2].start, tasks_handler.task_list[TASK_2].next_trigger, "The task should be scheduled from the tick returned by the callback");

    current_tick = 9U;
    get_tick_calls = 0U;
    rc = tasks_api_pause_task(&tasks_handler, TASK_2);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, get_tick_calls, "pause should read the tick once");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(9U, tasks_handler.task_list[TASK_2].last_update, "The pause should be recorded at the tick returned by the callback");

    current_tick = 20U;
    get_tick_calls = 0U;
    rc = tasks_api_disable_task(&tasks_handler, TASK_2);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, get_tick_calls, "disable should read the tick once");

    get_tick_calls = 0U;
    rc = tasks_api_update_task(&tasks_handler, TASK_2, 30U, 4U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, get_tick_calls, "update should read the tick once");
}

static void callback_advance_tick_and_pause_self(uint8_t task_id) {
    current_tick += 2U; // Time passes while the callback is running
    callback_rc = tasks_api_pause_task(&tasks_handler, task_id);
}

void test_tasks_routine_callback_api_calls_use_the_tick_at_call_time(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = function_1, .interval = 10U, .start = 0U },
        { .task_id = TASK_2, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_2, .interval = 20U, .start = 5U },
        { .task_id = TASK_3, .state = TASKS_STATE_DISABLED, .repeats = 1U, .function = function_3, .interval = 15U, .start = 10U },
        { .task_id = TASK_4, .state = TASKS_STATE_DISABLED, .repeats = 0U, .function = function_4, .interval = 25U, .start = 0U }
    };

    current_tick = 0U;
    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, fake_get_tick);

    function_1_fake.custom_fake = callback_advance_tick_and_pause_self;

    // The task fires at tick 0 and is paused by its own callback at tick 2, with 8 ticks left to the next trigger (10)
    rc = tasks_api_routine(&tasks_handler);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code from the routine");
    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, callback_rc, "The pause called inside the callback should succeed");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_PAUSED, tasks_handler.actual_state[TASK_1], "Task should be PAUSED");

    function_1_fake.custom_fake = NULL;

    current_tick = 20U;
    rc = tasks_api_enable_task(&tasks_handler, TASK_1);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(28U, tasks_handler.task_list[TASK_1].next_trigger, "The remaining time should be measured from the tick at which the callback paused the task");
}

void setUp(void) {
    memset(&tasks_handler, 0, sizeof(tasks_handler));
    current_tick = 0U;
    tasks_api_init(&tasks_handler, t_list, TASK_COUNT, fake_get_tick);
    get_tick_calls = 0U;

    RESET_FAKE(function_1);
    RESET_FAKE(function_2);
    RESET_FAKE(function_3);
    RESET_FAKE(function_4);

    callback_rc = TASKS_RC_ERROR;
}

void tearDown(void) {
}

int main(void) {
    UNITY_BEGIN();

    // INIT TESTS

    RUN_TEST(test_tasks_init_with_null_handler_returns_error);
    RUN_TEST(test_tasks_init_with_null_tick_callback_returns_error);
    RUN_TEST(test_tasks_init_stores_tick_callback);
    RUN_TEST(test_tasks_init_uses_the_tick_returned_by_the_callback);
    RUN_TEST(test_tasks_init_with_null_list_returns_error);
    RUN_TEST(test_tasks_init_with_invalid_list_item_state_returns_error);
    RUN_TEST(test_tasks_init_with_invalid_list_item_id_returns_error);
    RUN_TEST(test_tasks_init_with_zero_tasks_returns_error);
    RUN_TEST(test_tasks_init_with_invalid_count_returns_error);
    RUN_TEST(test_tasks_init_with_valid_list_initializes_tasks_handler);
    RUN_TEST(test_tasks_init_with_valid_list_initializes_heap);
    RUN_TEST(test_tasks_init_without_function_original_states_defaults_disabled);
    RUN_TEST(test_tasks_init_without_repeats_defaults_to_infinite);

    // TRANSITION TESTS

    RUN_TEST(test_tasks_handle_task_transition_with_null_handler_returns_error);
    RUN_TEST(test_tasks_handle_task_transition_with_invalid_id_returns_error);
    RUN_TEST(test_tasks_handle_task_transition_with_valid_id_returns_ok);
    RUN_TEST(test_tasks_handle_task_transition_with_valid_id_adds_and_removes_from_heap);
    RUN_TEST(test_tasks_handle_task_transition_with_valid_id_updates_trigger_time);
    RUN_TEST(test_tasks_handle_task_transition_with_valid_id_updates_trigger_time_on_resume);
    RUN_TEST(test_tasks_handle_task_transition_with_valid_id_and_no_state_change_returns_ok);
    RUN_TEST(test_tasks_handle_task_transition_with_valid_id_and_no_state_change_does_not_modify_heap);

    // HEAP UPDATE TESTS

    RUN_TEST(test_tasks_update_heap_with_null_handler_returns_error);
    RUN_TEST(test_tasks_update_heap_with_correct_parameters_returns_ok);

    // ROUTINE TESTS

    RUN_TEST(test_tasks_routine_with_null_handler_returns_error);
    RUN_TEST(test_tasks_routine_with_no_tasks_returns_error);
    RUN_TEST(test_tasks_routine_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_tasks_routine_with_valid_parameters_executes_tasks_immediately);
    RUN_TEST(test_tasks_routine_with_valid_parameters_executes_tasks_after_interval);
    RUN_TEST(test_tasks_routine_with_valid_parameters_executes_repeats_task_exact_times);
    RUN_TEST(test_tasks_routine_with_valid_parameters_executes_repeats_task_after_reenabled);
    RUN_TEST(test_tasks_routine_with_interval_set_to_zero_treats_as_one);
    RUN_TEST(test_tasks_routine_with_valid_parameters_executes_repeats_N_times);

    // CALLBACK REENTRANCY TESTS

    RUN_TEST(test_tasks_routine_callback_can_disable_its_own_task);
    RUN_TEST(test_tasks_routine_callback_can_pause_its_own_task_and_resume_with_remaining_time);
    RUN_TEST(test_tasks_routine_callback_can_update_its_own_task);
    RUN_TEST(test_tasks_routine_callback_can_reenable_its_own_expired_one_shot_task);
    RUN_TEST(test_tasks_routine_callback_can_disable_another_task);

    // TICK CALLBACK TESTS

    RUN_TEST(test_tasks_api_with_uninitialized_handler_returns_null_pointer);
    RUN_TEST(test_tasks_routine_reads_the_tick_only_once);
    RUN_TEST(test_tasks_control_functions_read_the_tick_once_at_call_time);
    RUN_TEST(test_tasks_routine_callback_api_calls_use_the_tick_at_call_time);

    // TASK CONTROL TESTS

    RUN_TEST(test_tasks_enable_with_null_handler_returns_error);
    RUN_TEST(test_tasks_enable_with_invalid_id_returns_error);
    RUN_TEST(test_tasks_enable_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_tasks_enable_with_already_enabled_task_returns_ok);
    RUN_TEST(test_tasks_enable_with_valid_id_enables_task);
    RUN_TEST(test_tasks_enable_after_pause_resumes_task_with_correct_trigger_time);
    RUN_TEST(test_tasks_enable_after_disable_restarts_task_with_correct_trigger_time);

    RUN_TEST(test_tasks_pause_with_null_handler_returns_error);
    RUN_TEST(test_tasks_pause_with_invalid_id_returns_error);
    RUN_TEST(test_tasks_pause_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_tasks_pause_with_already_paused_task_returns_ok);
    RUN_TEST(test_tasks_pause_with_valid_id_pauses_task);
    RUN_TEST(test_tasks_pause_repeats_task_before_start_resumes_correctly);
    RUN_TEST(test_tasks_pause_repeats_task_after_start_resumes_correctly);
    RUN_TEST(test_tasks_pause_mid_count_preserves_remaining_repeats);
    RUN_TEST(test_tasks_pause_disabled_task_returns_error);

    RUN_TEST(test_tasks_disable_with_null_handler_returns_error);
    RUN_TEST(test_tasks_disable_with_invalid_id_returns_error);
    RUN_TEST(test_tasks_disable_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_tasks_disable_with_already_disabled_task_returns_ok);
    RUN_TEST(test_tasks_disable_with_valid_id_disables_task);

    RUN_TEST(test_tasks_update_task_with_null_handler_returns_error);
    RUN_TEST(test_tasks_update_task_with_invalid_id_returns_error);
    RUN_TEST(test_tasks_update_task_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_tasks_update_task_with_valid_id_updates_task);
    RUN_TEST(test_tasks_update_task_with_valid_id_and_disabled_task_updates_task);
    RUN_TEST(test_tasks_update_task_with_valid_id_and_paused_task_updates_task);
    RUN_TEST(test_tasks_update_task_sets_repeats_to_arbitrary_value);

    RUN_TEST(test_tasks_get_task_with_null_handler_returns_error);
    RUN_TEST(test_tasks_get_task_with_invalid_id_returns_error);
    RUN_TEST(test_tasks_get_task_with_valid_id_returns_ok_and_copies_task);

    // CALLBACK TASK ID TESTS

    RUN_TEST(test_tasks_routine_passes_task_id_to_callback);
    RUN_TEST(test_tasks_routine_passes_correct_task_id_to_each_callback);
    RUN_TEST(test_tasks_routine_passes_non_zero_task_id_to_callback);
    RUN_TEST(test_tasks_routine_passes_own_task_id_to_all_callbacks_when_all_enabled);
    RUN_TEST(test_tasks_routine_passes_task_id_on_every_repeat);
    RUN_TEST(test_tasks_routine_passes_task_id_after_pause_and_resume);
    RUN_TEST(test_tasks_routine_passes_task_id_after_reenable_from_disabled);
    RUN_TEST(test_tasks_routine_passes_task_id_after_update_task);
    RUN_TEST(test_tasks_routine_with_interval_set_to_zero_passes_task_id);

    return UNITY_END();
}