/*!
 * \file tasks-example.c
 * \date 2026-05-07
 * \authors Alessandro Giustina [giustinalessandro@gmail.com]
 *
 * \brief Simple example of the tasks module usage.
 * \details In this example, we initialize the tasks system and create three simple tasks.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "tasks-api.h"
#include <inttypes.h>

void print_task_1(void) {
    printf("Task_1 executed");
    return;
}

void print_task_2(void) {
    printf("Task_2 executed");
    return;
}

void print_task_3(void) {
    printf("Task_3 executed");
    return;
}

enum TaskNames {
    PRINT_TASK_1 = 0,
    PRINT_TASK_2,
    PRINT_TASK_3,
    TASK_COUNT

};

// The original task list can go out of scope
void init_tasks_module(struct TasksHandler *task_handler) {

    // The only parameters that can be omitted are one_shot (default false)
    // and task_state (default DISABLED), but the task_function and ID must be provided for every task
    static TaskList t_l = {
        // Enabled default task, not one-shot, immediate start, interval of 5 ticks
        {
            .task_id = PRINT_TASK_1,
            .repeats = 0,
            .task_function = print_task_1,
            .task_interval = 5U,
            .task_state = TASKS_STATE_ENABLED,
            .task_start = 0U,
        },

        // Enabled default task, not one-shot, start delayed by 5 ticks, interval of 7 ticks
        {
            .task_id = PRINT_TASK_2,
            .task_function = print_task_2,
            .task_interval = 7U,
            .task_start = 5U,
        },

        // Enabled one-shot task, start delayed by 10 ticks, interval of 5 ticks (ignored since one-shot)
        {
            .task_id = PRINT_TASK_3,
            .repeats = 1,
            .task_function = print_task_3,
            .task_interval = 5U,
            .task_state = TASKS_STATE_ENABLED,
            .task_start = 10U,
        }
    };

    enum TasksReturnCode rc = tasks_api_init(task_handler, t_l, TASK_COUNT, 0U);

    if (rc == TASKS_RC_OK) {
        printf("The module initialized correctly!\n");
    }
}

int main(void) {

    /*
     * Initialize the task module with some tasks
     */

    struct TasksHandler tasks_handler;

    init_tasks_module(&tasks_handler);

    /*
     * Go trough some ticks to see initialized behaviour.
     * We enable the module at tick 5
     * At tick 5 we should see task 1 fire
     * At tick 10 again task 1 should fire as the
     * At tick 15 task 3 should fire for the first time and also task 1.
     * 
     * We also enable task 2 at tick 7, so it should fire at ticks 12 and 19.
     */

    for (uint32_t i = 0; i <= 25; i++) {

        if (i == 5) {
            tasks_api_enable_module(&tasks_handler, i);
        }

        if (i == 7) {
            tasks_api_enable_task(&tasks_handler, PRINT_TASK_2, i);
        }

        printf("Tick: %" PRId32 "  -> ", i);
        tasks_api_routine(&tasks_handler, i);

        printf("\n");
    }

    tasks_api_pause_task(&tasks_handler, PRINT_TASK_2, 25U);

    // Update and start task 3 to run 5 times with an interval of 3 ticks and an immediate start, so it should fire at ticks 26, 29, 32, 35 and 38
    tasks_api_update_task(&tasks_handler, PRINT_TASK_3, 3U, 0U, 5U, 25U);
    tasks_api_enable_task(&tasks_handler, PRINT_TASK_3, 25U);
    /*
     * Pause task 2 at tick 25 and see that it doesn't fire at tick 26
     * and then reenable it at tick 35, at which point it should fire after 1 tick and then every 7 ticks as before.
     * once resumed at tick 35 it should fire after 5 ticks.
     */
    for (uint32_t i = 26; i <= 60; i++) {

        if (i == 35) {
            tasks_api_enable_task(&tasks_handler, PRINT_TASK_2, i);
        }

        printf("Tick: %" PRId32 "  -> ", i);
        tasks_api_routine(&tasks_handler, i);

        printf("\n");
    }

    /*
     * If the module is disabled at tick 65, no task should fire at tick 70, 
     * but if we reenable it at tick 76 all the tasks should resume as if they were frozen.
     */
    for (uint32_t i = 61; i <= 100; i++) {

        if (i == 65) {
            tasks_api_disable_module(&tasks_handler, i);
        }

        if (i == 76) {
            tasks_api_enable_module(&tasks_handler, i);
        }

        printf("Tick: %" PRId32 "  -> ", i);
        tasks_api_routine(&tasks_handler, i);

        printf("\n");
    }

    tasks_api_disable_task(&tasks_handler, PRINT_TASK_1, 100U);
    tasks_api_disable_task(&tasks_handler, PRINT_TASK_2, 100U);
    tasks_api_disable_task(&tasks_handler, PRINT_TASK_3, 100U);

    /*
     * After disabling and reenabling the tasks, they should all resume from the beginning as if they were never executed before
     */

    tasks_api_enable_task(&tasks_handler, PRINT_TASK_1, 100U);
    tasks_api_enable_task(&tasks_handler, PRINT_TASK_2, 100U);
    tasks_api_enable_task(&tasks_handler, PRINT_TASK_3, 100U);

    /*
     * We can see the same behaviour if the tick doesn't have a regular increment
     */

    /*
     * Expected behaviour:
     * At tick 100 task 1 should fire
     * At tick 105 task 1 should fire again and also task 2 should fire for the first time
     * At tick 110 task 1 should fire again and task 3 should fire for the first (and last) time
     * At tick 112 task 2 should fire again
     * At tick 115 task 1 should fire again
     * Then task 1 should fire every 5 ticks and task 2 every 7 ticks.
     */

    for (uint32_t i = 100; i <= 150;) {

        printf("Tick: %" PRIu32 "  -> ", i);
        tasks_api_routine(&tasks_handler, i);

        printf("\n");

        // Increment tick by a random amount between 1 and 5
        i += rand() % 5 + 1;
    }

    return 0;
}