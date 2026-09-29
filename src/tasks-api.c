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
 * \param task_a pointer to the first task
 * \param task_b pointer to the second task
 * \return -1 if the first task should be scheduled before the second, 1 if it should be scheduled after, 0 if they are equal
 */
EAGLETRT_STATIC int8_t prv_task_compare(void *task_a, void *task_b) {
    const struct Task *const task_f = *(struct Task **)task_a;
    const struct Task *const task_s = *(struct Task **)task_b;

    // Compare timestamps
    if (task_f->next_trigger < task_s->next_trigger) {
        return -1;
    }
    if (task_f->next_trigger > task_s->next_trigger) {
        return 1;
    }

    /**************************************************************************
     * For the equality check, in addition to the ticks, the pointers to the
     * task must also be equal, otherwise -1 or 1 may be returned
     ***************************************************************************/
    if (task_f->task_id < task_s->task_id) {
        return -1;
    }
    if (task_f->task_id > task_s->task_id) {
        return 1;
    }
    return 0;
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

    enum TaskState from_state = tasks_handler->actual_state[task_id];
    enum TaskState to_state = tasks_handler->task_list[task_id].state;

    if (from_state == to_state) {
        return TASKS_RC_OK;
    }

    struct Task *task = &tasks_handler->task_list[task_id];

    // Leaving ENABLED
    if (from_state == TASKS_STATE_ENABLED) {
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
    if (to_state == TASKS_STATE_ENABLED) {
        // (task->next_trigger - task->last_update) is the remaining time to the next trigger when the task was paused, if the task was paused and there is still time to wait before the next trigger,
        // we can just add that remaining time to the current tick to get the new trigger time,
        // otherwise we can just calculate the trigger time from the start time of the task
        if (from_state == TASKS_STATE_PAUSED && (int32_t)(task->next_trigger - task->last_update) >= 0) {
            task->next_trigger = tick + (task->next_trigger - task->last_update);
        } else {
            tasks_handler->task_list[task_id].repeats = tasks_handler->task_list[task_id].int_repeats;
            task->next_trigger = tick + task->start;
        }
        if (min_heap_api_insert(&tasks_handler->scheduled_tasks, &task) != MIN_HEAP_RC_OK) {
            return TASKS_RC_ERROR;
        }
    }

    // Transitions between PAUSED and DISABLED are just state updates, as the next trigger time is calculated only when entering ENABLED
    tasks_handler->actual_state[task_id] = to_state;
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
        enum TasksReturnCode return_code = prv_handle_task_transition(tasks_handler, i, current_tick);
        if (return_code != TASKS_RC_OK) {
            return return_code;
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
            t_list[i].state == TASKS_STATE_PAUSED ||
            t_list[i].function == NULL) {
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
        next_task->function();

        next_task->last_update = current_tick;

        if (next_task->int_repeats > 0) {
            --(next_task->repeats);
        }

        if ((next_task->repeats > 0 || next_task->int_repeats == 0) && next_task->state == TASKS_STATE_ENABLED) {

            // Update the next trigger time
            next_task->next_trigger += EAGLETRT_API_MAX(next_task->interval, 1U);

            // Reinsert the task with the updated trigger time
            if (min_heap_api_insert(&tasks_handler->scheduled_tasks, &next_task) != MIN_HEAP_RC_OK) {
                return TASKS_RC_ERROR;
            }
        } else {
            // For expired tasks, just update the state to disabled and don't reinsert it into the heap
            next_task->state = TASKS_STATE_DISABLED;
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

    tasks_handler->task_list[task_id].state = TASKS_STATE_ENABLED;

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
    if (tasks_handler->actual_state[task_id] == TASKS_STATE_DISABLED) {
        return TASKS_RC_ERROR;
    }

    tasks_handler->task_list[task_id].state = TASKS_STATE_PAUSED;

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

    tasks_handler->task_list[task_id].state = TASKS_STATE_DISABLED;

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

    struct Task *task = &tasks_handler->task_list[task_id];

    task->interval = new_interval;
    task->start = new_start;
    task->int_repeats = repeats;
    task->repeats = repeats;

    // Disable and reenable the task to update the heap with the new information
    enum TaskState original_state = task->state;

    if (original_state != TASKS_STATE_ENABLED) {
        tasks_handler->prev_tick = current_tick;
        return TASKS_RC_OK;
    }

    task->state = TASKS_STATE_DISABLED;
    enum TasksReturnCode return_code = prv_handle_task_transition(tasks_handler, task_id, current_tick);
    if (return_code != TASKS_RC_OK) {
        task->state = original_state; // Restore the original state in case of error
        return return_code;
    }
    task->state = original_state;

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