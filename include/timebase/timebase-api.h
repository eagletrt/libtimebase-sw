/*!
 * \file timebase.h
 * \date 2024-04-16
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 * \author Antonio Gelain [antonio.gelain2@gmail.com]
 *
 * \brief Functions to manage periodic tasks at certain intervals
 */

#ifndef TIMEBASE_API_H
#define TIMEBASE_API_H

#include "timebase.h"

/*!
 * \brief Initialize the timebase handler
 *
 * \param resolution The amount of time that represent one tick (in ms)
 *
 * \retval TIMEBASE_RC_NULL_POINTER if a tasks is not implemented
 * \retval TIMEBASE_RC_OK otherwise
 */
enum TimebaseReturnCode timebase_api_init(struct TimebaseHandler *timebase_handler, const uint32_t resolution_ms);

/*!
 * \brief Enable or disable the timebase
 *
 * \param enabled True to enable the timebase false to disable it
 */
void timebase_set_enable(struct TimebaseHandler *timebase_handler, const bool enabled);

/*!
 * \brief Increment the internal timebase by one tick
 *
 * \retval TIMEBASE_RC_DISABLED if the timebase is disabled
 * \retval TIMEBASE_RC_OK otherwise
 */
enum TimebaseReturnCode timebase_inc_tick(struct TimebaseHandler *timebase_handler);

/*!
 * \brief Get the current number of ticks
 *
 * \returns uint32_t The number of ticks
 */
uint32_t timebase_get_tick(struct TimebaseHandler *timebase_handler);

/*!
 * \brief Get the current elapsed time in ms
 *
 * \returns uint32_t The current elapsed time
 */
uint32_t timebase_get_time(struct TimebaseHandler *timebase_handler);

/*!
 * \brief Get the number of ms that represents a single tick
 *
 * \returns uint32_t The timebase resolution
 */
uint32_t timebase_get_resolution(struct TimebaseHandler *timebase_handler);

#endif /* TIMEBASE_API_H */