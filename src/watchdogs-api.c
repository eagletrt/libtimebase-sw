/*!
 * \file watchdogs-api.c
 * \date 2026-05-7
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 *
 * \brief Functions to manage watchdogs
 */

#include "watchdogs-api.h"
#include "watchdogs.h"

/*!
 * \brief Compare two watchdogs based on their next trigger time.
 * \param a Pointer to the first watchdog.
 * \param b Pointer to the second watchdog.
 * \return -1 if the first watchdog should be scheduled before the second, 1 if after, 0 if equal.
 */
EAGLETRT_STATIC int8_t prv_watchdog_compare(void *a, void *b) {
    const struct Watchdog *const f = *(struct Watchdog **)a;
    const struct Watchdog *const s = *(struct Watchdog **)b;

    // Compare timestamps
    if (f->next_trigger < s->next_trigger) {
        return -1;
    }
    if (f->next_trigger > s->next_trigger) {
        return 1;
    }

    /**************************************************************************
     * For the equality check, in addition to the ticks, the pointers to the
     * watchdog must also be equal, otherwise -1 or 1 may be returned
     * In this case 1 is preferred because it avoid useless swaps between
     * elements that have the same number of ticks
     ***************************************************************************/
    if (f->watchdog_callback == s->watchdog_callback) {
        return 0;
    }
    return 1;
}

/*!
 * \brief Unregister a watchdog from the system.
 * \param watchdogs_handler Pointer to the watchdog handler.
 * \param watchdog Pointer to the watchdog to unregister.
 * \retval WATCHDOG_RC_OK if successful
 * \retval WATCHDOG_RC_NULL_POINTER if the watchdog handler or the watchdog is NULL
 * \retval WATCHDOG_RC_NOT_RUNNING if the watchdog is not running
 * \retval WATCHDOG_RC_TIMED_OUT if the watchdog has already timed out
 * \retval WATCHDOG_RC_ERROR if an error occurred during the execution of the function
 */
EAGLETRT_STATIC enum WatchdogReturnCode prv_watchdog_unregister(struct WatchdogHandler *watchdogs_handler, struct Watchdog *watchdog) {
    if (watchdogs_handler == NULL || watchdog == NULL) {
        return WATCHDOG_RC_NULL_POINTER;
    }
    if (watchdog->watchdog_state == WATCHDOG_STATE_TIMED_OUT) {
        return WATCHDOG_RC_TIMED_OUT;
    }
    if (watchdog->watchdog_state != WATCHDOG_STATE_RUNNING) {
        return WATCHDOG_RC_NOT_RUNNING;
    }

    long find_result = min_heap_api_find(&watchdogs_handler->scheduled_watchdogs, &watchdog);
    if (find_result < 0) {
        return WATCHDOG_RC_ERROR;
    }
    if (min_heap_api_remove(&watchdogs_handler->scheduled_watchdogs, find_result, NULL) != MIN_HEAP_RC_OK) {
        return WATCHDOG_RC_ERROR;
    }

    watchdog->watchdog_state = WATCHDOG_STATE_NOT_RUNNING;

    return WATCHDOG_RC_OK;
}

enum WatchdogReturnCode watchdogs_api_init_pool(struct WatchdogHandler *watchdogs_handler, uint32_t current_tick) {

    if (watchdogs_handler == NULL) {
        return WATCHDOG_RC_NULL_POINTER;
    }

    memset(watchdogs_handler, 0, sizeof(*watchdogs_handler));

    arena_allocator_api_init(&watchdogs_handler->arena_handler);

    if (min_heap_api_init(&watchdogs_handler->scheduled_watchdogs,
                          sizeof(struct Watchdog *),
                          MAX_WATCHDOGS,
                          prv_watchdog_compare,
                          &watchdogs_handler->arena_handler) != MIN_HEAP_RC_OK) {
        return WATCHDOG_RC_ERROR;
    };

    watchdogs_handler->prev_tick = current_tick;

    return WATCHDOG_RC_OK;
};

enum WatchdogReturnCode watchdogs_api_routine(struct WatchdogHandler *watchdogs_handler, uint32_t current_tick) {
    if (watchdogs_handler == NULL) {
        return WATCHDOG_RC_NULL_POINTER;
    }
    if (watchdogs_handler->prev_tick > current_tick) {
        return WATCHDOG_RC_TEMPORAL_DISCONTINUITY;
    }
    if (watchdogs_handler->watchdog_module_enabled == false) {
        return WATCHDOG_RC_DISABLED;
    }

    if (min_heap_api_is_empty(&watchdogs_handler->scheduled_watchdogs)) {
        watchdogs_handler->prev_tick = current_tick;
        return WATCHDOG_RC_OK;
    }

    struct Watchdog *next_watchdog;

    if (min_heap_api_remove(&watchdogs_handler->scheduled_watchdogs, 0U, &next_watchdog) != MIN_HEAP_RC_OK) {
        return WATCHDOG_RC_ERROR;
    }

    for (;;) {
        if (next_watchdog->next_trigger > current_tick) {
            min_heap_api_insert(&watchdogs_handler->scheduled_watchdogs, &next_watchdog);
            break;
        }

        next_watchdog->watchdog_state = WATCHDOG_STATE_TIMED_OUT;

        // Execute the task
        next_watchdog->watchdog_callback();

        if (min_heap_api_is_empty(&watchdogs_handler->scheduled_watchdogs)) {
            break;
        }

        // Get the next watchdog to check
        if (min_heap_api_remove(&watchdogs_handler->scheduled_watchdogs, 0U, &next_watchdog) != MIN_HEAP_RC_OK) {
            return WATCHDOG_RC_ERROR;
        }
    }

    watchdogs_handler->prev_tick = current_tick;

    return WATCHDOG_RC_OK;
}

enum WatchdogReturnCode watchdogs_api_init_watchdog(struct Watchdog *const watchdog, uint32_t timeout, watchdog_timeout_callback callback) {
    if (watchdog == NULL || callback == NULL) {
        return WATCHDOG_RC_NULL_POINTER;
    }
    if (watchdog->is_initialized == true) {
        return WATCHDOG_RC_ERROR;
    }
    if (timeout == 0) {
        return WATCHDOG_RC_ERROR;
    }

    watchdog->watchdog_state = WATCHDOG_STATE_NOT_RUNNING;
    watchdog->watchdog_callback = callback;
    watchdog->timeout = timeout;
    watchdog->next_trigger = 0;
    watchdog->last_update = 0;

    watchdog->is_initialized = true;

    return WATCHDOG_RC_OK;
}

enum WatchdogReturnCode watchdogs_api_watchdog_start(struct WatchdogHandler *const watchdogs_handler, struct Watchdog *const watchdog, uint32_t current_tick) {
    if (watchdogs_handler == NULL || watchdog == NULL) {
        return WATCHDOG_RC_NULL_POINTER;
    }
    if (watchdogs_handler->prev_tick > current_tick) {
        return WATCHDOG_RC_TEMPORAL_DISCONTINUITY;
    }
    if (watchdog->is_initialized == false) {
        return WATCHDOG_RC_UNINITIALIZED;
    }
    if (watchdogs_handler->watchdog_module_enabled == false) {
        return WATCHDOG_RC_DISABLED;
    }
    if (watchdog->watchdog_state == WATCHDOG_STATE_RUNNING) {
        return WATCHDOG_RC_BUSY;
    }
    if (watchdog->watchdog_state == WATCHDOG_STATE_TIMED_OUT) {
        return WATCHDOG_RC_TIMED_OUT;
    }

    enum WatchdogState original_state = watchdog->watchdog_state;

    if (watchdog->watchdog_state == WATCHDOG_STATE_PAUSED) {
        watchdog->next_trigger = current_tick + (watchdog->next_trigger - watchdog->last_update);
    } else {
        watchdog->next_trigger = current_tick + watchdog->timeout;
    }

    if (min_heap_api_insert(&watchdogs_handler->scheduled_watchdogs, &watchdog) != MIN_HEAP_RC_OK) {
        watchdog->watchdog_state = original_state;
        return WATCHDOG_RC_ERROR;
    }

    watchdog->watchdog_state = WATCHDOG_STATE_RUNNING;

    return WATCHDOG_RC_OK;
}

enum WatchdogReturnCode watchdogs_api_watchdog_stop(struct WatchdogHandler *const watchdogs_handler, struct Watchdog *const watchdog) {

    if (watchdogs_handler == NULL || watchdog == NULL) {
        return WATCHDOG_RC_NULL_POINTER;
    }
    if (watchdog->is_initialized == false) {
        return WATCHDOG_RC_UNINITIALIZED;
    }
    if (watchdog->watchdog_state == WATCHDOG_STATE_NOT_RUNNING) {
        return WATCHDOG_RC_NOT_RUNNING;
    }
    if (watchdog->watchdog_state == WATCHDOG_STATE_TIMED_OUT) {
        return WATCHDOG_RC_TIMED_OUT;
    }
    if (watchdogs_handler->watchdog_module_enabled == false) {
        return WATCHDOG_RC_DISABLED;
    }

    if (watchdog->watchdog_state == WATCHDOG_STATE_PAUSED) {
        watchdog->watchdog_state = WATCHDOG_STATE_NOT_RUNNING;
        return WATCHDOG_RC_OK;
    }

    enum WatchdogReturnCode unregister_result = prv_watchdog_unregister(watchdogs_handler, watchdog);
    if (unregister_result != WATCHDOG_RC_OK) {
        return unregister_result;
    }

    return WATCHDOG_RC_OK;
}

enum WatchdogReturnCode watchdogs_api_watchdog_pause(struct WatchdogHandler *const watchdogs_handler, struct Watchdog *const watchdog, uint32_t current_tick) {

    if (watchdogs_handler == NULL || watchdog == NULL) {
        return WATCHDOG_RC_NULL_POINTER;
    }
    if (watchdogs_handler->prev_tick > current_tick) {
        return WATCHDOG_RC_TEMPORAL_DISCONTINUITY;
    }
    if (watchdog->is_initialized == false) {
        return WATCHDOG_RC_UNINITIALIZED;
    }
    if (watchdog->watchdog_state == WATCHDOG_STATE_TIMED_OUT) {
        return WATCHDOG_RC_TIMED_OUT;
    }
    if (watchdog->watchdog_state != WATCHDOG_STATE_RUNNING) {
        return WATCHDOG_RC_NOT_RUNNING;
    }
    if (watchdogs_handler->watchdog_module_enabled == false) {
        return WATCHDOG_RC_DISABLED;
    }

    enum WatchdogReturnCode unregister_result = prv_watchdog_unregister(watchdogs_handler, watchdog);
    if (unregister_result != WATCHDOG_RC_OK) {
        return unregister_result;
    }

    watchdog->watchdog_state = WATCHDOG_STATE_PAUSED;

    watchdog->last_update = current_tick;

    return WATCHDOG_RC_OK;
}

enum WatchdogReturnCode watchdogs_api_watchdog_restart(struct WatchdogHandler *const watchdogs_handler, struct Watchdog *const watchdog, uint32_t current_tick) {

    if (watchdogs_handler == NULL || watchdog == NULL) {
        return WATCHDOG_RC_NULL_POINTER;
    }
    if (watchdog->is_initialized == false) {
        return WATCHDOG_RC_UNINITIALIZED;
    }
    if (watchdogs_handler->prev_tick > current_tick) {
        return WATCHDOG_RC_TEMPORAL_DISCONTINUITY;
    }
    if (watchdogs_handler->watchdog_module_enabled == false) {
        return WATCHDOG_RC_DISABLED;
    }

    if (watchdog->watchdog_state == WATCHDOG_STATE_RUNNING) {
        if (prv_watchdog_unregister(watchdogs_handler, watchdog) != WATCHDOG_RC_OK) {
            return WATCHDOG_RC_ERROR;
        }
    }

    watchdog->last_update = current_tick;
    watchdog->next_trigger = current_tick + watchdog->timeout;

    watchdog->watchdog_state = WATCHDOG_STATE_RUNNING;

    if (min_heap_api_insert(&watchdogs_handler->scheduled_watchdogs, &watchdog) != MIN_HEAP_RC_OK) {
        watchdog->watchdog_state = WATCHDOG_STATE_NOT_RUNNING;
        return WATCHDOG_RC_ERROR;
    }

    return WATCHDOG_RC_OK;
}

enum WatchdogReturnCode watchdogs_api_watchdog_pet(struct WatchdogHandler *const watchdogs_handler, struct Watchdog *const watchdog, uint32_t current_tick) {

    if (watchdogs_handler == NULL || watchdog == NULL) {
        return WATCHDOG_RC_NULL_POINTER;
    }
    if (watchdog->is_initialized == false) {
        return WATCHDOG_RC_UNINITIALIZED;
    }
    if (watchdogs_handler->prev_tick > current_tick) {
        return WATCHDOG_RC_TEMPORAL_DISCONTINUITY;
    }
    if (watchdog->watchdog_state == WATCHDOG_STATE_TIMED_OUT) {
        return WATCHDOG_RC_TIMED_OUT;
    }
    if (watchdog->watchdog_state != WATCHDOG_STATE_RUNNING) {
        return WATCHDOG_RC_NOT_RUNNING;
    }
    if (watchdogs_handler->watchdog_module_enabled == false) {
        return WATCHDOG_RC_DISABLED;
    }

    if (prv_watchdog_unregister(watchdogs_handler, watchdog) != WATCHDOG_RC_OK) {
        return WATCHDOG_RC_ERROR;
    }

    watchdog->next_trigger = current_tick + watchdog->timeout;

    watchdog->watchdog_state = WATCHDOG_STATE_RUNNING;

    if (min_heap_api_insert(&watchdogs_handler->scheduled_watchdogs, &watchdog) != MIN_HEAP_RC_OK) {
        watchdog->watchdog_state = WATCHDOG_STATE_NOT_RUNNING;
        return WATCHDOG_RC_ERROR;
    }

    return WATCHDOG_RC_OK;
}

enum WatchdogReturnCode watchdogs_api_watchdog_timeout(struct WatchdogHandler *watchdogs_handler, struct Watchdog *watchdog) {

    if (watchdogs_handler == NULL || watchdog == NULL) {
        return WATCHDOG_RC_NULL_POINTER;
    }
    if (watchdog->is_initialized == false) {
        return WATCHDOG_RC_UNINITIALIZED;
    }
    if (watchdog->watchdog_state != WATCHDOG_STATE_RUNNING) {
        return WATCHDOG_RC_NOT_RUNNING;
    }
    if (watchdogs_handler->watchdog_module_enabled == false) {
        return WATCHDOG_RC_DISABLED;
    }

    long find_result = min_heap_api_find(&watchdogs_handler->scheduled_watchdogs, &watchdog);
    if (find_result < 0) {
        return WATCHDOG_RC_ERROR;
    }
    if (min_heap_api_remove(&watchdogs_handler->scheduled_watchdogs, find_result, NULL) != MIN_HEAP_RC_OK) {
        return WATCHDOG_RC_ERROR;
    }

    watchdog->watchdog_state = WATCHDOG_STATE_TIMED_OUT;

    watchdog->watchdog_callback();

    return WATCHDOG_RC_OK;
}

bool watchdogs_api_watchdog_is_running(struct Watchdog *const watchdog) {
    if (watchdog == NULL) {
        return false;
    }

    return (watchdog->watchdog_state == WATCHDOG_STATE_RUNNING);
}

bool watchdogs_api_watchdog_is_timed_out(struct Watchdog *const watchdog) {
    if (watchdog == NULL) {
        return false;
    }

    return (watchdog->watchdog_state == WATCHDOG_STATE_TIMED_OUT);
}

enum WatchdogReturnCode watchdogs_api_enable_pool(struct WatchdogHandler *watchdogs_handler, uint32_t current_tick) {
    if (watchdogs_handler == NULL) {
        return WATCHDOG_RC_NULL_POINTER;
    }
    if (watchdogs_handler->prev_tick > current_tick) {
        return WATCHDOG_RC_TEMPORAL_DISCONTINUITY;
    }
    if (watchdogs_handler->watchdog_module_enabled == true) {
        return WATCHDOG_RC_OK;
    }

    watchdogs_handler->watchdog_module_enabled = true;

    // This looks like it breaks the heap but as it is a constant that is
    // added to all the elements it does not change the order of the elements
    for (int i = 0; i < watchdogs_handler->scheduled_watchdogs.size; ++i) {
        struct Watchdog *watchdog = *(struct Watchdog **)((uint8_t *)watchdogs_handler->scheduled_watchdogs.data + watchdogs_handler->scheduled_watchdogs.data_size * i);
        watchdog->next_trigger = current_tick + (watchdog->next_trigger - watchdogs_handler->prev_tick);
    }

    return WATCHDOG_RC_OK;
}

enum WatchdogReturnCode watchdogs_api_disable_pool(struct WatchdogHandler *watchdogs_handler, uint32_t current_tick) {
    if (watchdogs_handler == NULL) {
        return WATCHDOG_RC_NULL_POINTER;
    }
    if (watchdogs_handler->prev_tick > current_tick) {
        return WATCHDOG_RC_TEMPORAL_DISCONTINUITY;
    }
    if (watchdogs_handler->watchdog_module_enabled == false) {
        return WATCHDOG_RC_OK;
    }

    watchdogs_handler->watchdog_module_enabled = false;

    watchdogs_handler->prev_tick = current_tick;

    return WATCHDOG_RC_OK;
}