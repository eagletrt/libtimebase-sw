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

    rc = tasks_init(NULL, t_list, TASK_COUNT, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_NULL_POINTER, rc, "Expected NULL_POINTER return code");
}

void test_tasks_init_with_null_list_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_init(&tasks_handler, NULL, TASK_COUNT, 0U);

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

    rc = tasks_init(&tasks_handler, invalid_list, TASK_COUNT, 0U);

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

    rc = tasks_init(&tasks_handler, invalid_list, TASK_COUNT, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_LIST, rc, "Expected INVALID_LIST return code");
}

void test_tasks_init_with_zero_tasks_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_init(&tasks_handler, t_list, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_LIST, rc, "Expected INVALID_LIST return code");
}

void test_tasks_init_with_invalid_count_returns_error(void) {
    enum TasksReturnCode rc;

    rc = tasks_init(&tasks_handler, t_list, MAX_TASKS + 1U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_INVALID_LIST, rc, "Expected INVALID_LIST return code");
}

void test_tasks_init_with_valid_list_initializes_tasks_handler(void) {
    enum TasksReturnCode rc;

    rc = tasks_init(&tasks_handler, t_list, TASK_COUNT, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASK_COUNT, tasks_handler.task_num, "Number of tasks not set correctly");

    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&t_list[0], &tasks_handler.task_list[0], sizeof(struct Task), "Task 1 not copied correctly");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&t_list[1], &tasks_handler.task_list[1], sizeof(struct Task), "Task 2 not copied correctly");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&t_list[2], &tasks_handler.task_list[2], sizeof(struct Task), "Task 3 not copied correctly");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&t_list[3], &tasks_handler.task_list[3], sizeof(struct Task), "Task 4 not copied correctly");
}

void test_tasks_init_with_valid_list_initializes_heap(void) {
    enum TasksReturnCode rc;

    rc = tasks_init(&tasks_handler, t_list, TASK_COUNT, 0U);

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
    tasks_handler.task_num = TASK_COUNT;
    tasks_handler.actual_state[0] = TASKS_STATE_DISABLED;
    tasks_handler.task_list[0].task_state = TASKS_STATE_ENABLED;

    rc = prv_handle_task_transition(&tasks_handler, 0U, 0U);

    TEST_ASSERT_EQUAL_MESSAGE(TASKS_RC_OK, rc, "Expected OK return code");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(TASKS_STATE_ENABLED, tasks_handler.actual_state[0], "Task state should be updated to ENABLED");
}

void test_tasks_handle_task_transition_with_valid_id_adds_and_removes_from_heap(void) {
    enum TasksReturnCode rc;
    tasks_handler.task_num = TASK_COUNT;
    tasks_handler.actual_state[0] = TASKS_STATE_DISABLED;
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

void setUp(void) {

    tasks_init(&tasks_handler, t_list, TASK_COUNT, 0U);
    memset(&tasks_handler, 0, sizeof(tasks_handler));
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

    // TRANSITION TESTS

    RUN_TEST(test_tasks_handle_task_transition_with_null_handler_returns_error);
    RUN_TEST(test_tasks_handle_task_transition_with_invalid_id_returns_error);
    RUN_TEST(test_tasks_handle_task_transition_with_valid_id_returns_ok);
    RUN_TEST(test_tasks_handle_task_transition_with_valid_id_adds_and_removes_from_heap);

    return UNITY_END();
}