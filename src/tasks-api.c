/*!
 * \file tasks-api.c
 * \date 2026-05-7
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 *
 * \brief Functions to manage periodic tasks at certain intervals
 */

#include "tasks.h"

int8_t prv_task_compare(void *a, void *b) {
    const struct Task *const f = (struct Task *)a;
    const struct Task *const s = (struct Task *)b;

    // Compare timestamps
    if (f->next_trigger < s->next_trigger)
        return -1;
    if (f->next_trigger > s->next_trigger)
        return 1;

    /**************************************************************************
     * For the equality check, in addition to the ticks, the pointers to the
     * task must also be equal, otherwise -1 or 1 may be returned
     * In this case 1 is preferred because it avoid useless swaps between
     * elements that have the same number of ticks
     ***************************************************************************/
    if (f->task_function == s->task_function)
        return 0;
    return 1;
}

enum TasksReturnCode tasks_init(struct TasksHandler *tasks_handler, TaskList t_list, uint8_t num_tasks, uint32_t current_tick) {

    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }

    if (num_tasks == 0 || num_tasks > MAX_TASKS) {
        return TASKS_RC_INVALID_LIST;
    }

    // This is intentional as the arena allocator already has safeguards
    // and this avoids any memory leak in case of double init
    arena_allocator_api_free(&tasks_handler->arena_handler);

    memset(tasks_handler, 0, sizeof(*tasks_handler));

    arena_allocator_api_init(&tasks_handler->arena_handler);

    if (min_heap_api_init(&tasks_handler->scheduled_tasks,
                          sizeof(struct Task *),
                          MAX_TASKS,
                          prv_task_compare,
                          &tasks_handler->arena_handler) != MIN_HEAP_RC_OK) {
        return TASKS_RC_ERROR;
    };

    uint8_t actual_num_tasks = 0;

    for (uint8_t i = 0; i < num_tasks && t_list[i].task_function != NULL; ++i) {
        tasks_handler->task_list[i] = t_list[i];
        ++actual_num_tasks;
    }

    if (actual_num_tasks != num_tasks) {
        return TASKS_RC_INVALID_LIST;
    }

    tasks_handler->task_num = num_tasks;

    prv_tasks_update_heap(tasks_handler, current_tick);

    return TASKS_RC_OK;
};

EAGLETRT_STATIC enum TasksReturnCode prv_tasks_update_heap(struct TasksHandler *tasks_handler, uint32_t current_tick) {

    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }

    for (int i = 0; i < tasks_handler->task_num; i++) {

        if (tasks_handler->general_state[i] == TASK_STATE_ENABLED && tasks_handler->task_list[i].task_state != TASK_STATE_ENABLED) {
            tasks_handler->general_state[i] = tasks_handler->task_list[i].task_state;
            tasks_handler->task_list[i].last_update = current_tick;

            long index = min_heap_api_find(&tasks_handler->scheduled_tasks, &tasks_handler->task_list[i]);
            if (index >= 0) {
                min_heap_api_remove(&tasks_handler->scheduled_tasks, index, NULL);
            } else {
                return TASKS_RC_ERROR;
            }

        } else if (tasks_handler->general_state[i] == TASK_STATE_PAUSED && tasks_handler->task_list[i].task_state == TASK_STATE_ENABLED) {
            tasks_handler->general_state[i] = TASK_STATE_ENABLED;

            tasks_handler->task_list[i].next_trigger = current_tick + (tasks_handler->task_list[i].next_trigger - tasks_handler->task_list[i].last_update);

            min_heap_api_insert(&tasks_handler->scheduled_tasks, &tasks_handler->task_list[i]);
        } else if (tasks_handler->general_state[i] == TASK_STATE_DISABLED && tasks_handler->task_list[i].task_state == TASK_STATE_ENABLED) {
            tasks_handler->general_state[i] = TASK_STATE_ENABLED;

            tasks_handler->task_list[i].next_trigger = current_tick;

            min_heap_api_insert(&tasks_handler->scheduled_tasks, &tasks_handler->task_list[i]);
        } else if (tasks_handler->general_state[i] == TASK_STATE_PAUSED && tasks_handler->task_list[i].task_state == TASK_STATE_DISABLED) {
            tasks_handler->general_state[i] = TASK_STATE_DISABLED;
            long index = min_heap_api_find(&tasks_handler->scheduled_tasks, &tasks_handler->task_list[i]);
            if (index >= 0) {
                min_heap_api_remove(&tasks_handler->scheduled_tasks, index, NULL);
            } else {
                return TASKS_RC_ERROR;
            }
        } else {
            return TASKS_RC_ERROR;
        }
    }
    return TASKS_RC_OK;
}

enum TasksReturnCode tasks_routine(struct TasksHandler *task_handler, uint32_t current_tick) {
    if (task_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }

    if (min_heap_api_is_empty(&task_handler->scheduled_tasks)) {
        return TASKS_RC_OK;
    }

    struct Task next_task;

    if (min_heap_api_remove(&task_handler->scheduled_tasks, 0U, &next_task) != MIN_HEAP_RC_OK) {
        return TASKS_RC_ERROR;
    }

    for (;;) {
        if (next_task.next_trigger > current_tick) {
            min_heap_api_insert(&task_handler->scheduled_tasks, &next_task);
            break;
        }

        // Execute the task
        next_task.task_function();

        next_task.last_update = current_tick;

        if (!next_task.one_shot) {

            // Update the next trigger time
            next_task.next_trigger += next_task.task_interval;

            // Reinsert the task with the updated trigger time
            if (min_heap_api_insert(&task_handler->scheduled_tasks, &next_task) != MIN_HEAP_RC_OK) {
                return TASKS_RC_ERROR;
            }
        }

        if (min_heap_api_is_empty(&task_handler->scheduled_tasks)) {
            break;
        }

        // Get the next task to check
        if (min_heap_api_remove(&task_handler->scheduled_tasks, 0U, &next_task) != MIN_HEAP_RC_OK) {
            return TASKS_RC_ERROR;
        }
    }

    return TASKS_RC_OK;
}

enum TasksReturnCode tasks_enable(struct TasksHandler *tasks_handler, uint8_t task_id, uint32_t current_tick) {
    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }

    if (task_id >= tasks_handler->task_num) {
        return TASKS_RC_INVALID_ID;
    }

    tasks_handler->task_list[task_id].task_state = TASK_STATE_ENABLED;

    return prv_tasks_update_heap(tasks_handler, current_tick);
}

enum TasksReturnCode tasks_pause(struct TasksHandler *tasks_handler, uint8_t task_id, uint32_t current_tick) {
    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }

    if (task_id >= tasks_handler->task_num) {
        return TASKS_RC_INVALID_ID;
    }

    tasks_handler->task_list[task_id].task_state = TASK_STATE_PAUSED;

    return prv_tasks_update_heap(tasks_handler, current_tick);
}

enum TasksReturnCode tasks_disable(struct TasksHandler *tasks_handler, uint8_t task_id, uint32_t current_tick) {
    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }

    if (task_id >= tasks_handler->task_num) {
        return TASKS_RC_INVALID_ID;
    }

    tasks_handler->task_list[task_id].task_state = TASK_STATE_DISABLED;

    return prv_tasks_update_heap(tasks_handler, current_tick);
}

enum TasksReturnCode tasks_update_task(struct TasksHandler *tasks_handler, const uint8_t task_id, uint16_t new_interval, uint16_t new_start, bool one_shot, uint32_t current_tick) {
    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }

    if (task_id >= tasks_handler->task_num) {
        return TASKS_RC_INVALID_ID;
    }

    struct Task *task = &tasks_handler->task_list[task_id];

    task->task_interval = new_interval;
    task->task_start = new_start;
    task->one_shot = one_shot;

    return prv_tasks_update_heap(tasks_handler, current_tick);
}

enum TasksReturnCode tasks_get_task(struct TasksHandler *tasks_handler, const uint8_t task_id, struct Task *out) {
    if (tasks_handler == NULL || out == NULL) {
        return TASKS_RC_NULL_POINTER;
    }

    if (task_id >= tasks_handler->task_num) {
        return TASKS_RC_INVALID_ID;
    }

    *out = tasks_handler->task_list[task_id];

    return TASKS_RC_OK;
}