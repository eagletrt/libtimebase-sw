/*!
 * \file timebase-api.c
 * \date 2024-04-16
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 * \author Antonio Gelain [antonio.gelain2@gmail.com]
 *
 * \brief Functions to manage periodic tasks at certain intervals
 */

#include "timebase.h"

#include <string.h>

EAGLETRT_STATIC struct TimebaseHandler timebase_handler;

enum TimebaseReturnCode timebase_api_init(const uint32_t resolution_ms) {
    // Initialize timebase to 0
    memset(&timebase_handler, 0U, sizeof(timebase_handler));

    // Set default parameters
    timebase_handler.enabled = false;
    timebase_handler.resolution = EAGLETRT_API_MAX(1U, resolution_ms);

    return TIMEBASE_RC_OK;
}

void timebase_set_enable(const bool enabled) {
    timebase_handler.enabled = enabled;
}

enum TimebaseReturnCode timebase_inc_tick(void) {
    if (!timebase_handler.enabled)
        return TIMEBASE_RC_DISABLED;
    ++timebase_handler.ticks;
    return TIMEBASE_RC_OK;
}

uint32_t timebase_get_tick(void) {
    return timebase_handler.ticks;
}

uint32_t timebase_get_time(void) {
    return TIMEBASE_TICKS_TO_MS(timebase_handler.ticks, timebase_handler.resolution);
}

uint32_t timebase_get_resolution(void) {
    return timebase_handler.resolution;
}
