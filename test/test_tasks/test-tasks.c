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

FAKE_VOID_FUNC(task_function_1);
FAKE_VOID_FUNC(task_function_2);
FAKE_VOID_FUNC(task_function_3);
FAKE_VOID_FUNC(task_function_4);

enum TasksNames {
    TASK_1 = 0,
    TASK_2,
    TASK_3,
    TASK_4,
    TASK_COUNT
};

TaskList t_list = {
    { .task_id = TASK_1, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
    { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
    { .task_id = TASK_3, .task_state = TASKS_STATE_ENABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
    { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
};

// Function definition for private functions to be tested
enum TasksReturnCode prv_handle_task_transition(struct TasksHandler *tasks_handler, int8_t task_id, uint32_t tick);
enum TasksReturnCode prv_tasks_update_heap(struct TasksHandler *tasks_handler, uint32_t current_tick);

void test_tasks_init_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_init(NULL, t_list, TASK_COUNT, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_init_with_null_list_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_init(&tasks_handler, NULL, TASK_COUNT, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_init_with_invalid_list_item_state_returns_error(void) {
    enum TasksReturnCode rc;

    TaskList invalid_list = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_PAUSED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_ENABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    rc = tasks_api_init(&tasks_handler, invalid_list, TASK_COUNT, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_LIST, rc, "Expected INVALID_LIST return code");
}

void test_tasks_init_with_invalid_list_item_id_returns_error(void) {
    enum TasksReturnCode rc;

    TaskList invalid_list = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = 5U, .task_state = TASKS_STATE_ENABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    rc = tasks_api_init(&tasks_handler, invalid_list, TASK_COUNT, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_LIST, rc, "Expected INVALID_LIST return code");
}

void test_tasks_init_with_zero_tasks_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_init(&tasks_handler, t_list, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_LIST, rc, "Expected INVALID_LIST return code");
}

void test_tasks_init_with_invalid_count_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_init(&tasks_handler, t_list, MAX_TASKS + 1U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_LIST, rc, "Expected INVALID_LIST return code");
}

void test_tasks_init_with_valid_list_initializes_tasks_handler(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_init(&tasks_handler, t_list, TASK_COUNT, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_COUNT, tasks_handler.task_num, "Number of tasks not set correctly");

    t_list[2].next_trigger = 10U; // The next trigger time is set to task_start for enabled tasks during initialization

    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&t_list[0], &tasks_handler.task_list[0], sizeof(struct Task), "Task 1 not copied correctly");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&t_list[1], &tasks_handler.task_list[1], sizeof(struct Task), "Task 2 not copied correctly");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&t_list[2], &tasks_handler.task_list[2], sizeof(struct Task), "Task 3 not copied correctly");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&t_list[3], &tasks_handler.task_list[3], sizeof(struct Task), "Task 4 not copied correctly");
}

void test_tasks_init_with_valid_list_initializes_heap(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_init(&tasks_handler, t_list, TASK_COUNT, 0U);

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
        { .task_id = TASK_1, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    rc = tasks_api_init(&tasks_handler, invalid_list, TASK_COUNT, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    for (uint8_t i = 0; i < TASK_COUNT; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_DISABLED, tasks_handler.task_list[i].task_state, "Task state should be set to DISABLED when not initialized");
    }
}

void test_tasks_init_without_one_shot_defaults_to_non_one_shot(void) {
    enum TasksReturnCode rc;

    TaskList invalid_list = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_ENABLED, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_ENABLED, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    rc = tasks_api_init(&tasks_handler, invalid_list, TASK_COUNT, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    for (uint8_t i = 0; i < TASK_COUNT; ++i) {
        TEST_ASSERT_FALSE_MESSAGE(tasks_handler.task_list[i].one_shot, "one_shot should be set to false when not initialized");
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
        { .task_id = TASK_1, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    tasks_handler.task_list[0].task_state = TASKS_STATE_ENABLED;

    rc = prv_handle_task_transition(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, tasks_handler.actual_state[0], "Task state should be updated to ENABLED");
}

void test_tasks_handle_task_transition_with_valid_id_adds_and_removes_from_heap(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    tasks_handler.task_list[0].task_state = TASKS_STATE_ENABLED;

    rc = prv_handle_task_transition(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_FALSE_MESSAGE(min_heap_api_is_empty(&tasks_handler.scheduled_tasks), "Scheduled tasks heap should not be empty after enabling task");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, tasks_handler.scheduled_tasks.size, "Scheduled tasks heap should contain 1 task after enabling task");

    tasks_handler.task_list[0].task_state = TASKS_STATE_DISABLED;

    rc = prv_handle_task_transition(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_TRUE_MESSAGE(min_heap_api_is_empty(&tasks_handler.scheduled_tasks), "Scheduled tasks heap should be empty after disabling task");
}

void test_tasks_handle_task_transition_with_valid_id_updates_trigger_time(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 10U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    tasks_handler.task_list[0].task_state = TASKS_STATE_ENABLED;

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
        { .task_id = TASK_1, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 10U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    tasks_handler.task_list[0].task_state = TASKS_STATE_ENABLED;
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
    tasks_handler.task_list[0].task_state = TASKS_STATE_ENABLED;

    rc = prv_handle_task_transition(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, tasks_handler.actual_state[0], "Task state should remain ENABLED");
}

void test_tasks_handle_task_transition_with_valid_id_and_no_state_change_does_not_modify_heap(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);
    tasks_handler.task_num = TASK_COUNT;
    tasks_handler.actual_state[0] = TASKS_STATE_ENABLED;
    tasks_handler.task_list[0].task_state = TASKS_STATE_ENABLED;

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
        { .task_id = TASK_1, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 10U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    tasks_handler.task_list[0].task_state = TASKS_STATE_ENABLED;

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

    rc = tasks_api_routine(NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_routine_with_no_tasks_returns_error(void) {
    enum TasksReturnCode rc;

    tasks_handler.task_num = 0U;

    rc = tasks_api_routine(&tasks_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_ERROR, rc, "Expected ERROR return code");
}

void test_tasks_routine_with_module_disabled_returns_disabled(void) {
    enum TasksReturnCode rc;

    tasks_handler.task_num = TASK_COUNT;
    tasks_handler.task_module_enabled = false;

    rc = tasks_api_routine(&tasks_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_DISABLED, rc, "Expected DISABLED return code");
}

void test_task_routine_with_past_tick_returns_temporal_discontinuity(void) {
    enum TasksReturnCode rc;

    tasks_handler.prev_tick = 10U;

    rc = tasks_api_routine(&tasks_handler, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_tasks_routine_with_valid_parameters_executes_tasks_immediately(void) {
    enum TasksReturnCode rc;

    tasks_handler.task_module_enabled = true;

    rc = tasks_api_routine(&tasks_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, task_function_1_fake.call_count, "Task function should have been called once");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, task_function_2_fake.call_count, "Task function 2 should not have been called");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, task_function_3_fake.call_count, "Task function 3 should not have been called");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, task_function_4_fake.call_count, "Task function 4 should not have been called");
}

void test_tasks_routine_with_valid_parameters_executes_tasks_after_interval(void) {
    enum TasksReturnCode rc;

    tasks_handler.task_module_enabled = true;

    rc = tasks_api_routine(&tasks_handler, 4U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, task_function_1_fake.call_count, "Task function should not have been called yet");

    rc = tasks_api_routine(&tasks_handler, 12U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, task_function_1_fake.call_count, "Task function should have been called once after interval");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, task_function_2_fake.call_count, "Task function 2 should not have been called");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, task_function_3_fake.call_count, "Task function 3 should not have been called");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, task_function_4_fake.call_count, "Task function 4 should not have been called");
}

void test_tasks_routine_with_valid_parameters_executes_one_shot_tasks_only_once(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_ENABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 5U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    tasks_handler.task_module_enabled = true;

    rc = tasks_api_routine(&tasks_handler, 4U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, task_function_3_fake.call_count, "Task function should not have been called yet");

    rc = tasks_api_routine(&tasks_handler, 12U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, task_function_3_fake.call_count, "Task function should have been called once after interval");

    rc = tasks_api_routine(&tasks_handler, 35U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, task_function_3_fake.call_count, "One-shot task function should not have been called again");
}

void test_tasks_routine_with_valid_parameters_executes_one_shot_after_reenabled(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_ENABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 5U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    tasks_handler.task_module_enabled = true;

    rc = tasks_api_routine(&tasks_handler, 12U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, task_function_3_fake.call_count, "Task function should have been called once after interval");

    tasks_api_enable_task(&tasks_handler, TASK_3, 20U);

    rc = tasks_api_routine(&tasks_handler, 24U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1U, task_function_3_fake.call_count, "One-shot task function should not have been called again after being reenabled before timeout");

    rc = tasks_api_routine(&tasks_handler, 30U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2U, task_function_3_fake.call_count, "One-shot task function should have been called again after being reenabled and reaching timeout");
}

void test_tasks_enable_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_enable_task(NULL, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_enable_with_invalid_id_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_enable_task(&tasks_handler, MAX_TASKS + 1U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_ID, rc, "Expected INVALID_ID return code");
}

void test_tasks_enable_with_past_tick_returns_temporal_discontinuity(void) {
    enum TasksReturnCode rc;

    tasks_handler.prev_tick = 10U;

    rc = tasks_api_enable_task(&tasks_handler, 0U, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_tasks_enable_with_already_enabled_task_returns_ok(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    rc = tasks_api_enable_task(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, tasks_handler.actual_state[0], "Task state should remain ENABLED");
}

void test_tasks_enable_with_valid_id_enables_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    rc = tasks_api_enable_task(&tasks_handler, 1U, 0U);

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
        { .task_id = TASK_1, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    tasks_handler.actual_state[1] = TASKS_STATE_PAUSED;
    tasks_handler.task_list[1].next_trigger = 20U;
    tasks_handler.task_list[1].last_update = 10U;

    rc = tasks_api_enable_task(&tasks_handler, 1U, 15U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    struct Task *next_task;
    if (min_heap_api_remove(&tasks_handler.scheduled_tasks, 0U, &next_task) == MIN_HEAP_RC_OK) {
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(25U, next_task->next_trigger, "Next trigger time should be updated according to the time the task was paused");
    } else {
        TEST_FAIL_MESSAGE("Failed to remove task from heap");
    }
}

void test_tasks_pause_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_pause_task(NULL, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_pause_with_invalid_id_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_pause_task(&tasks_handler, MAX_TASKS + 1U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_ID, rc, "Expected INVALID_ID return code");
}

void test_tasks_pause_with_past_tick_returns_temporal_discontinuity(void) {
    enum TasksReturnCode rc;

    tasks_handler.prev_tick = 10U;

    rc = tasks_api_pause_task(&tasks_handler, 0U, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_tasks_pause_with_already_paused_task_returns_ok(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    tasks_handler.actual_state[0] = TASKS_STATE_PAUSED;

    rc = tasks_api_pause_task(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_PAUSED, tasks_handler.actual_state[0], "Task state should remain PAUSED");
}

void test_tasks_pause_with_valid_id_pauses_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    rc = tasks_api_pause_task(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_PAUSED, tasks_handler.actual_state[0], "Task state should be updated to PAUSED");
    struct Task *task_ptr = &tasks_handler.task_list[0];
    if (min_heap_api_find(&tasks_handler.scheduled_tasks, &task_ptr) >= 0) {
        TEST_FAIL_MESSAGE("Paused task should NOT be in the scheduled tasks heap");
    }
}

void test_tasks_disable_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_disable_task(NULL, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_disable_with_invalid_id_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_disable_task(&tasks_handler, MAX_TASKS + 1U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_ID, rc, "Expected INVALID_ID return code");
}

void test_tasks_disable_with_past_tick_returns_temporal_discontinuity(void) {
    enum TasksReturnCode rc;

    tasks_handler.prev_tick = 10U;

    rc = tasks_api_disable_task(&tasks_handler, 0U, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_tasks_disable_with_already_disabled_task_returns_ok(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    rc = tasks_api_disable_task(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_DISABLED, tasks_handler.actual_state[0], "Task state should remain DISABLED");
}

void test_tasks_disable_with_valid_id_disables_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    rc = tasks_api_disable_task(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_DISABLED, tasks_handler.actual_state[0], "Task state should be updated to DISABLED");
    struct Task *task_ptr = &tasks_handler.task_list[0];
    if (min_heap_api_find(&tasks_handler.scheduled_tasks, &task_ptr) >= 0) {
        TEST_FAIL_MESSAGE("Disabled task should NOT be in the scheduled tasks heap");
    }
}

void test_tasks_update_task_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_update_task(NULL, 0U, 10U, 0U, false, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_update_task_with_invalid_id_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_update_task(&tasks_handler, MAX_TASKS + 1U, 10U, 0U, false, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_ID, rc, "Expected INVALID_ID return code");
}

void test_tasks_update_task_with_past_tick_returns_temporal_discontinuity(void) {
    enum TasksReturnCode rc;

    tasks_handler.prev_tick = 10U;

    rc = tasks_api_update_task(&tasks_handler, 0U, 10U, 0U, false, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_tasks_update_task_with_valid_id_updates_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 5U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    rc = tasks_api_update_task(&tasks_handler, 0U, 20U, 10U, true, 10U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(20U, tasks_handler.task_list[0].task_interval, "Task interval should be updated");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(10U, tasks_handler.task_list[0].task_start, "Task start should be updated");
    TEST_ASSERT_TRUE_MESSAGE(tasks_handler.task_list[0].one_shot, "Task one_shot flag should be updated to true");

    // Check if the task is updated in the heap
    struct Task *next_task;
    if (min_heap_api_remove(&tasks_handler.scheduled_tasks, 0U, &next_task) == MIN_HEAP_RC_OK) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, next_task->task_id, "Updated task should be in the heap");
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(20U, next_task->task_interval, "Task interval in heap should be updated");
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(10U, next_task->task_start, "Task start in heap should be updated");
        TEST_ASSERT_TRUE_MESSAGE(next_task->one_shot, "Task one_shot flag in heap should be updated to true");
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(20U, next_task->next_trigger, "Next trigger time should be updated according to new start and interval");
    } else {
        TEST_FAIL_MESSAGE("Failed to remove task from heap");
    }
}

void test_tasks_update_task_with_valid_id_and_disabled_task_updates_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    rc = tasks_api_update_task(&tasks_handler, 1U, 30U, 10U, true, 10U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(30U, tasks_handler.task_list[1].task_interval, "Task interval should be updated");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(10U, tasks_handler.task_list[1].task_start, "Task start should be updated");
    TEST_ASSERT_TRUE_MESSAGE(tasks_handler.task_list[1].one_shot, "Task one_shot flag should be updated to true");

    // Check that the task is not in the heap since it is disabled
    struct Task *task_ptr = &tasks_handler.task_list[1];
    if (min_heap_api_find(&tasks_handler.scheduled_tasks, &task_ptr) >= 0) {
        TEST_FAIL_MESSAGE("Disabled task should NOT be in the scheduled tasks heap");
    }
}

void test_tasks_update_task_with_valid_id_and_paused_task_updates_task(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    tasks_api_pause_task(&tasks_handler, 0U, 0U);

    rc = tasks_api_update_task(&tasks_handler, 0U, 20U, 10U, true, 10U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(20U, tasks_handler.task_list[0].task_interval, "Task interval should be updated");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(10U, tasks_handler.task_list[0].task_start, "Task start should be updated");
    TEST_ASSERT_TRUE_MESSAGE(tasks_handler.task_list[0].one_shot, "Task one_shot flag should be updated to true");

    // Check that the task is not in the heap since it is paused
    struct Task *task_ptr = &tasks_handler.task_list[0];
    if (min_heap_api_find(&tasks_handler.scheduled_tasks, &task_ptr) >= 0) {
        TEST_FAIL_MESSAGE("Paused task should NOT be in the scheduled tasks heap");
    }
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
        { .task_id = TASK_1, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 5U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_DISABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_DISABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    rc = tasks_api_get_task(&tasks_handler, 0U, &task);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0U, task.task_id, "Task ID should be copied correctly");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, task.task_state, "Task state should be copied correctly");
    TEST_ASSERT_FALSE_MESSAGE(task.one_shot, "Task one_shot flag should be copied correctly");
    TEST_ASSERT_EQUAL_PTR_MESSAGE(task_function_1, task.task_function, "Task function pointer should be copied correctly");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(10U, task.task_interval, "Task interval should be copied correctly");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(5U, task.task_start, "Task start should be copied correctly");
}

void test_tasks_module_enable_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_enable_module(NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_module_enable_with_past_tick_returns_temporal_discontinuity(void) {
    enum TasksReturnCode rc;

    tasks_handler.prev_tick = 10U;

    rc = tasks_api_enable_module(&tasks_handler, 5U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_tasks_module_enable_with_valid_handler_enables_module(void) {
    enum TasksReturnCode rc;

    tasks_handler.task_module_enabled = false;

    rc = tasks_api_enable_module(&tasks_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_TRUE_MESSAGE(tasks_handler.task_module_enabled, "Task module should be enabled");
}

void test_tasks_module_enable_updates_next_trigger_times_on_enable(void) {
    enum TasksReturnCode rc;

    TaskList local_tasks = {
        { .task_id = TASK_1, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_1, .task_interval = 10U, .task_start = 0U },
        { .task_id = TASK_2, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_2, .task_interval = 20U, .task_start = 5U },
        { .task_id = TASK_3, .task_state = TASKS_STATE_ENABLED, .one_shot = true, .task_function = task_function_3, .task_interval = 15U, .task_start = 10U },
        { .task_id = TASK_4, .task_state = TASKS_STATE_ENABLED, .one_shot = false, .task_function = task_function_4, .task_interval = 25U, .task_start = 0U }
    };

    tasks_api_init(&tasks_handler, local_tasks, TASK_COUNT, 0U);

    rc = tasks_api_enable_module(&tasks_handler, 10U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(10U, tasks_handler.prev_tick, "prev_tick should be updated to the current tick when module is enabled");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(10U, tasks_handler.task_list[0].next_trigger, "Next trigger time for task 0 should be updated according to the current tick");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(15U, tasks_handler.task_list[1].next_trigger, "Next trigger time for task 1 should be updated according to the current tick");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(20U, tasks_handler.task_list[2].next_trigger, "Next trigger time for task 2 should be updated according to the current tick");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(10U, tasks_handler.task_list[3].next_trigger, "Next trigger time for task 3 should be updated according to the current tick");
}

void test_tasks_module_disable_with_null_handler_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_api_disable_module(NULL, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_module_disable_sets_prev_tick_to_current_tick(void) {
    enum TasksReturnCode rc;

    tasks_handler.prev_tick = 100U;

    rc = tasks_api_disable_module(&tasks_handler, 150U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(150U, tasks_handler.prev_tick, "prev_tick should be reset to the current tick when module is disabled");
}

void test_tasks_module_disable_with_past_tick_returns_temporal_discontinuity(void) {
    enum TasksReturnCode rc;

    tasks_handler.prev_tick = 100U;

    rc = tasks_api_disable_module(&tasks_handler, 90U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_TEMPORAL_DISCONTINUITY, rc, "Expected TEMPORAL_DISCONTINUITY return code");
}

void test_tasks_module_disable_with_valid_handler_disables_module(void) {
    enum TasksReturnCode rc;

    tasks_handler.task_module_enabled = true;

    rc = tasks_api_disable_module(&tasks_handler, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_FALSE_MESSAGE(tasks_handler.task_module_enabled, "Task module should be disabled");
}

void setUp(void) {
    memset(&tasks_handler, 0, sizeof(tasks_handler));
    tasks_api_init(&tasks_handler, t_list, TASK_COUNT, 0U);

    RESET_FAKE(task_function_1);
    RESET_FAKE(task_function_2);
    RESET_FAKE(task_function_3);
    RESET_FAKE(task_function_4);
}

void tearDown(void) {
}

int main(void) {
    UNITY_BEGIN();

    // INIT TESTS

    RUN_TEST(test_tasks_init_with_null_handler_returns_error);
    RUN_TEST(test_tasks_init_with_null_list_returns_error);
    RUN_TEST(test_tasks_init_with_invalid_list_item_state_returns_error);
    RUN_TEST(test_tasks_init_with_invalid_list_item_id_returns_error);
    RUN_TEST(test_tasks_init_with_zero_tasks_returns_error);
    RUN_TEST(test_tasks_init_with_invalid_count_returns_error);
    RUN_TEST(test_tasks_init_with_valid_list_initializes_tasks_handler);
    RUN_TEST(test_tasks_init_with_valid_list_initializes_heap);
    RUN_TEST(test_tasks_init_without_function_original_states_defaults_disabled);
    RUN_TEST(test_tasks_init_without_one_shot_defaults_to_non_one_shot);

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
    RUN_TEST(test_tasks_routine_with_module_disabled_returns_disabled);
    RUN_TEST(test_task_routine_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_tasks_routine_with_valid_parameters_executes_tasks_immediately);
    RUN_TEST(test_tasks_routine_with_valid_parameters_executes_tasks_after_interval);
    RUN_TEST(test_tasks_routine_with_valid_parameters_executes_one_shot_tasks_only_once);
    RUN_TEST(test_tasks_routine_with_valid_parameters_executes_one_shot_after_reenabled);

    // TASK CONTROL TESTS

    RUN_TEST(test_tasks_enable_with_null_handler_returns_error);
    RUN_TEST(test_tasks_enable_with_invalid_id_returns_error);
    RUN_TEST(test_tasks_enable_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_tasks_enable_with_already_enabled_task_returns_ok);
    RUN_TEST(test_tasks_enable_with_valid_id_enables_task);
    RUN_TEST(test_tasks_enable_after_pause_resumes_task_with_correct_trigger_time);

    RUN_TEST(test_tasks_pause_with_null_handler_returns_error);
    RUN_TEST(test_tasks_pause_with_invalid_id_returns_error);
    RUN_TEST(test_tasks_pause_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_tasks_pause_with_already_paused_task_returns_ok);
    RUN_TEST(test_tasks_pause_with_valid_id_pauses_task);

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

    RUN_TEST(test_tasks_get_task_with_null_handler_returns_error);
    RUN_TEST(test_tasks_get_task_with_invalid_id_returns_error);
    RUN_TEST(test_tasks_get_task_with_valid_id_returns_ok_and_copies_task);

    RUN_TEST(test_tasks_module_enable_with_null_handler_returns_error);
    RUN_TEST(test_tasks_module_enable_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_tasks_module_enable_with_valid_handler_enables_module);
    RUN_TEST(test_tasks_module_enable_updates_next_trigger_times_on_enable);

    RUN_TEST(test_tasks_module_disable_with_null_handler_returns_error);
    RUN_TEST(test_tasks_module_disable_sets_prev_tick_to_current_tick);
    RUN_TEST(test_tasks_module_disable_with_past_tick_returns_temporal_discontinuity);
    RUN_TEST(test_tasks_module_disable_with_valid_handler_disables_module);

    return UNITY_END();
}