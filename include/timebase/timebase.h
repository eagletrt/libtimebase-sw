/*!
 * \file timebase.h
 * \date 2024-04-16
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 * \author Antonio Gelain [antonio.gelain2@gmail.com]
 *
 * \brief Functions to manage periodic tasks at certain intervals
 */

#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <stdbool.h>
#include <stdint.h>

#include "eagletrt-api.h"

/*!
 * \brief Convert the time in ms to ticks
 *
 * \param T The time to convert
 * \param RES The resolution of a tick
 *
 * \return The corresponing amount of ticks
 */
#define TIMEBASE_MS_TO_TICKS(T, RES) ((T) / (RES))

/*!
 * \brief Convert the ticks in ms
 *
 * \param T The ticks to convert
 * \param RES The resolution of a tick
 *
 * \return The corresponing amount of ms
 */
#define TIMEBASE_TICKS_TO_MS(T, RES) ((T) * (RES))

/*!
 * \brief Return code for the timebase module functions
 */
enum TimebaseReturnCode {
    TIMEBASE_RC_OK,           /*!< Function executed successfully */
    TIMEBASE_RC_NULL_POINTER, /*!< A NULL pointer was given to a function */
    TIMEBASE_RC_DISABLED,     /*!< The timebase is not running */
};

/*!
 * \brief Type definition for the timebase handler structure
 *
 * \attention This structure should not be used outside of this module
 *
 * \param enabled True if the timebase is running, false otherwise
 * \param resolution Number of ms that represent one tick
 * \param ticks The current number of ticks
 */
struct TimebaseHandler {
    bool enabled;
    uint32_t resolution;
    EAGLETRT_VOLATILE uint32_t ticks;
};

#endif // TIMEBASE_H
