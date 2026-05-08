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

#include "tasks-api.h"

uint32_t tick = 0;

void print_task_1(void) {
    printf("Task_1 executed correctly at tick %d", tick);
    return;
}

void print_task_2(void) {
    printf("Task_2 executed correctly at tick %d", tick);
    return;
}

void print_task_3(void) {
    printf("Task_3 executed correctly at tick %d", tick);
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
    TaskList t_l = {
        // Enabled default task, not one-shot, immediate start, interval of 5 ticks
        {
            .task_id = PRINT_TASK_1,
            .one_shot = false,
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

        // Enabled one-shot task, start delayed by 10 ticks, interval of 15 ticks (ignored since one-shot)
        {
            .task_id = PRINT_TASK_3,
            .one_shot = true,
            .task_function = print_task_3,
            .task_interval = 15U,
            .task_state = TASKS_STATE_ENABLED,
            .task_start = 10U,
        }
    };

    enum TasksReturnCode rc = (task_handler, t_l, TASK_COUNT, 0U);

    if (rc == TASKS_RC_OK) {
        printf("The module initialized correctly!");
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
     * 
     * At tick 0 we should see task 1 fire
     * At tick 5 again task 1 should fire as the 
     * 
     */

    return 0;
}