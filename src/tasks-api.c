/*!
 * \file tasks-api.c
 * \date 2026-05-7
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 *
 * \brief Functions to manage periodic tasks at certain intervals
 */

#include "tasks.h"

/*!
 * \brief Compares two tasks based on their next trigger times
 * \param a pointer to the first task
 * \param b pointer to the second task
 * \return -1 if the first task should be scheduled before the second, 1 if it should be scheduled after, 0 if they are equal
 */
int8_t prv_task_compare(void *a, void *b) {
    const struct Task *const f = *(struct Task **)a;
    const struct Task *const s = *(struct Task **)b;

    // Compare timestamps
    if (f->next_trigger < s->next_trigger) {
        return -1;
    }
    if (f->next_trigger > s->next_trigger) {
        return 1;
    }

    /**************************************************************************
     * For the equality check, in addition to the ticks, the pointers to the
     * task must also be equal, otherwise -1 or 1 may be returned
     * In this case 1 is preferred because it avoid useless swaps between
     * elements that have the same number of ticks
     ***************************************************************************/
    if (f->task_function == s->task_function) {
        return 0;
    }
    return 1;
}

/*!
 * \brief This function handles the phase transition of a single function.
 * POSSIBLE TRANSITIONS
 * same-state transitions -> no-op
 * 
 * ENABLED -> PAUSED removes task from heap and sets paused state
 * ENABLED -> DISABLED removes task from heap and sets disabled state
 * 
 * PAUSED -> ENABLED if the task isn't one shot the next trigger time is calculated using the remaining time from when it was paused and added to heap
 * DISABLED -> ENABLED the next trigger time is set from the start time of the task and then it is added to heap
 * 
 * DISABLED -> PAUSED this just updates the status of the task. As the trigger time is calculated only when entering enabled this is defined behaviour
 * and a disabled task can be "paused" as if it was paused from the start.
 * 
 * PAUSED -> DISABLED same as above
 * 
 * \param tasks_handler the handler structure 
 * \param task_id the ID of the task to modify
 * \param tick the current tick
 * 
 * \retval TASKS_RC_NULL_POINTER the task handler pointer is null
 * \retval TASKS_RC_INVALID_ID the task id is invalid
 * \retval TASKS_RC_ERROR problems were encountered when accessing the heap
 * \retval TASKS_RC_OK the function executed correctly
 */
EAGLETRT_STATIC enum TasksReturnCode prv_handle_task_transition(struct TasksHandler *tasks_handler, uint8_t task_id, uint32_t tick) {

    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }
    if (task_id >= tasks_handler->task_num) {
        return TASKS_RC_INVALID_ID;
    }

    enum TaskState from = tasks_handler->actual_state[task_id];
    enum TaskState to = tasks_handler->task_list[task_id].task_state;

    if (from == to) {
        return TASKS_RC_OK;
    }

    struct Task *task = &tasks_handler->task_list[task_id];

    // Leaving ENABLED
    if (from == TASKS_STATE_ENABLED) {
        long idx = min_heap_api_find(&tasks_handler->scheduled_tasks, &task);
        if (idx < 0) {
            return TASKS_RC_ERROR;
        }
        if (min_heap_api_remove(&tasks_handler->scheduled_tasks, (size_t)idx, NULL) != MIN_HEAP_RC_OK) {
            return TASKS_RC_ERROR;
        }
        task->last_update = tick;
    }

    // Entering ENABLED
    if (to == TASKS_STATE_ENABLED) {
        // (task->next_trigger - task->last_update) is the remaining time to the next trigger when the task was paused, if the task was paused and there is still time to wait before the next trigger,
        // we can just add that remaining time to the current tick to get the new trigger time,
        // otherwise we can just calculate the trigger time from the start time of the task
        if (from == TASKS_STATE_PAUSED && (int)(task->next_trigger - task->last_update) >= 0) {
            task->next_trigger = tick + (task->next_trigger - task->last_update);
        } else {
            tasks_handler->task_list[task_id].repeats = tasks_handler->task_list[task_id].int_repeats;
            task->next_trigger = tick + task->task_start;
        }
        if (min_heap_api_insert(&tasks_handler->scheduled_tasks, &task) != MIN_HEAP_RC_OK) {
            return TASKS_RC_ERROR;
        }
    }

    // Transitions between PAUSED and DISABLED are just state updates, as the next trigger time is calculated only when entering ENABLED
    tasks_handler->actual_state[task_id] = to;
    return TASKS_RC_OK;
}

/*!
 * \brief updates all tasks in the heap
 *
 * \param tasks_handler the handler structure
 * \param current_tick the current tick
 * 
 * \retval TASKS_RC_NULL_POINTER the task handler pointer is null
 * \retval TASKS_RC_ERROR problems were encountered when accessing the heap
 * \retval TASKS_RC_OK the function executed correctly
 */
EAGLETRT_STATIC enum TasksReturnCode prv_tasks_update_heap(struct TasksHandler *tasks_handler, uint32_t current_tick) {
    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }

    for (int i = 0; i < tasks_handler->task_num; i++) {
        enum TasksReturnCode rc = prv_handle_task_transition(tasks_handler, i, current_tick);
        if (rc != TASKS_RC_OK) {
            return rc;
        }
    }
    return TASKS_RC_OK;
}

enum TasksReturnCode tasks_api_init(struct TasksHandler *tasks_handler, TaskList t_list, uint8_t num_tasks, uint32_t current_tick) {

    if (tasks_handler == NULL || t_list == NULL) {
        return TASKS_RC_NULL_POINTER;
    }
    if (num_tasks == 0 || num_tasks > MAX_TASKS) {
        return TASKS_RC_INVALID_LIST;
    }

    memset(tasks_handler, 0, sizeof(*tasks_handler));

    arena_allocator_api_init(&tasks_handler->arena_handler);

    if (min_heap_api_init(&tasks_handler->scheduled_tasks,
                          sizeof(struct Task *),
                          MAX_TASKS,
                          prv_task_compare,
                          &tasks_handler->arena_handler) != MIN_HEAP_RC_OK) {
        return TASKS_RC_ERROR;
    };

    for (uint8_t i = 0; i < num_tasks; ++i) {
        if (t_list[i].task_id != i ||
            t_list[i].task_state == TASKS_STATE_PAUSED ||
            t_list[i].task_function == NULL) {
            return TASKS_RC_INVALID_LIST;
        }
        tasks_handler->task_list[i] = t_list[i];
        tasks_handler->task_list[i].int_repeats = t_list[i].repeats;
    }

    tasks_handler->task_num = num_tasks;

    tasks_handler->prev_tick = current_tick;

    prv_tasks_update_heap(tasks_handler, current_tick);

    return TASKS_RC_OK;
};

enum TasksReturnCode tasks_api_routine(struct TasksHandler *tasks_handler, uint32_t current_tick) {
    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }
    if (tasks_handler->task_num == 0) {
        return TASKS_RC_ERROR;
    }
    if (tasks_handler->prev_tick > current_tick) {
        return TASKS_RC_TEMPORAL_DISCONTINUITY;
    }
    if (tasks_handler->task_module_enabled == false) {
        return TASKS_RC_DISABLED;
    }

    if (min_heap_api_is_empty(&tasks_handler->scheduled_tasks)) {
        return TASKS_RC_OK;
    }

    struct Task *next_task;

    if (min_heap_api_remove(&tasks_handler->scheduled_tasks, 0U, &next_task) != MIN_HEAP_RC_OK) {
        return TASKS_RC_ERROR;
    }

    for (;;) {
        if (next_task->next_trigger > current_tick) {
            min_heap_api_insert(&tasks_handler->scheduled_tasks, &next_task);
            break;
        }

        // Execute the task
        next_task->task_function();

        next_task->last_update = current_tick;

        if (next_task->int_repeats > 0) {
            --(next_task->repeats);
        }

        if (next_task->repeats > 0 || next_task->int_repeats == 0) {

            // Update the next trigger time
            next_task->next_trigger += EAGLETRT_API_MAX(next_task->task_interval, 1U);

            // Reinsert the task with the updated trigger time
            if (min_heap_api_insert(&tasks_handler->scheduled_tasks, &next_task) != MIN_HEAP_RC_OK) {
                return TASKS_RC_ERROR;
            }
        } else {
            // For expired tasks, just update the state to disabled and don't reinsert it into the heap
            next_task->task_state = TASKS_STATE_DISABLED;
            tasks_handler->actual_state[next_task->task_id] = TASKS_STATE_DISABLED;
        }

        if (min_heap_api_is_empty(&tasks_handler->scheduled_tasks)) {
            break;
        }

        // Get the next task to check
        if (min_heap_api_remove(&tasks_handler->scheduled_tasks, 0U, &next_task) != MIN_HEAP_RC_OK) {
            return TASKS_RC_ERROR;
        }
    }

    tasks_handler->prev_tick = current_tick;

    return TASKS_RC_OK;
}

enum TasksReturnCode tasks_api_enable_task(struct TasksHandler *tasks_handler, uint8_t task_id, uint32_t current_tick) {
    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }
    if (tasks_handler->prev_tick > current_tick) {
        return TASKS_RC_TEMPORAL_DISCONTINUITY;
    }
    if (task_id >= tasks_handler->task_num) {
        return TASKS_RC_INVALID_ID;
    }
    if (tasks_handler->task_module_enabled == false) {
        return TASKS_RC_DISABLED;
    }

    tasks_handler->task_list[task_id].task_state = TASKS_STATE_ENABLED;

    tasks_handler->prev_tick = current_tick;

    return prv_handle_task_transition(tasks_handler, task_id, current_tick);
}

enum TasksReturnCode tasks_api_pause_task(struct TasksHandler *tasks_handler, uint8_t task_id, uint32_t current_tick) {
    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }
    if (tasks_handler->prev_tick > current_tick) {
        return TASKS_RC_TEMPORAL_DISCONTINUITY;
    }
    if (task_id >= tasks_handler->task_num) {
        return TASKS_RC_INVALID_ID;
    }
    if (tasks_handler->task_module_enabled == false) {
        return TASKS_RC_DISABLED;
    }

    tasks_handler->task_list[task_id].task_state = TASKS_STATE_PAUSED;

    tasks_handler->prev_tick = current_tick;

    return prv_handle_task_transition(tasks_handler, task_id, current_tick);
}

enum TasksReturnCode tasks_api_disable_task(struct TasksHandler *tasks_handler, uint8_t task_id, uint32_t current_tick) {
    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }
    if (tasks_handler->prev_tick > current_tick) {
        return TASKS_RC_TEMPORAL_DISCONTINUITY;
    }
    if (task_id >= tasks_handler->task_num) {
        return TASKS_RC_INVALID_ID;
    }
    if (tasks_handler->task_module_enabled == false) {
        return TASKS_RC_DISABLED;
    }

    tasks_handler->task_list[task_id].task_state = TASKS_STATE_DISABLED;

    tasks_handler->prev_tick = current_tick;

    return prv_handle_task_transition(tasks_handler, task_id, current_tick);
}

enum TasksReturnCode tasks_api_update_task(struct TasksHandler *tasks_handler, const uint8_t task_id, uint16_t new_interval, uint16_t new_start, uint8_t repeats, uint32_t current_tick) {
    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }
    if (tasks_handler->prev_tick > current_tick) {
        return TASKS_RC_TEMPORAL_DISCONTINUITY;
    }
    if (task_id >= tasks_handler->task_num) {
        return TASKS_RC_INVALID_ID;
    }
    if (tasks_handler->task_module_enabled == false) {
        return TASKS_RC_DISABLED;
    }

    struct Task *task = &tasks_handler->task_list[task_id];

    task->task_interval = new_interval;
    task->task_start = new_start;
    task->int_repeats = repeats;
    task->repeats = repeats;

    // Disable and reenable the task to update the heap with the new information
    enum TaskState original_state = task->task_state;

    if (original_state != TASKS_STATE_ENABLED) {
        tasks_handler->prev_tick = current_tick;
        return TASKS_RC_OK;
    }

    task->task_state = TASKS_STATE_DISABLED;
    enum TasksReturnCode rc = prv_handle_task_transition(tasks_handler, task_id, current_tick);
    if (rc != TASKS_RC_OK) {
        return rc;
    }
    task->task_state = original_state;

    tasks_handler->prev_tick = current_tick;

    return prv_handle_task_transition(tasks_handler, task_id, current_tick);
}

enum TasksReturnCode tasks_api_get_task(struct TasksHandler *tasks_handler, uint8_t task_id, struct Task *task) {
    if (tasks_handler == NULL || task == NULL) {
        return TASKS_RC_NULL_POINTER;
    }

    if (task_id >= tasks_handler->task_num) {
        return TASKS_RC_INVALID_ID;
    }

    *task = tasks_handler->task_list[task_id];

    return TASKS_RC_OK;
}

enum TasksReturnCode tasks_api_enable_module(struct TasksHandler *tasks_handler, uint32_t current_tick) {
    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }
    if (current_tick < tasks_handler->prev_tick) {
        return TASKS_RC_TEMPORAL_DISCONTINUITY;
    }

    for (int i = 0; i < tasks_handler->task_num; i++) {
        tasks_handler->task_list[i].next_trigger += (current_tick - tasks_handler->prev_tick);
    }

    tasks_handler->task_module_enabled = true;

    tasks_handler->prev_tick = current_tick;

    return TASKS_RC_OK;
}

enum TasksReturnCode tasks_api_disable_module(struct TasksHandler *tasks_handler, uint32_t current_tick) {
    if (tasks_handler == NULL) {
        return TASKS_RC_NULL_POINTER;
    }
    if (current_tick < tasks_handler->prev_tick) {
        return TASKS_RC_TEMPORAL_DISCONTINUITY;
    }
    if (tasks_handler->task_module_enabled == false) {
        return TASKS_RC_OK;
    }

    tasks_handler->prev_tick = current_tick;

    tasks_handler->task_module_enabled = false;

    return TASKS_RC_OK;
}