/*!
 * \file watchdogs-api.h
 * \date 2024-04-16
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 * \author Antonio Gelain [antonio.gelain2@gmail.com]
 *
 * \brief Implementation of generic watchdogs that time-out after a certain interval of time 
 */

#include "watchdogs.h"

#ifndef WATCHDOGS_API_H
#define WATCHDOGS_API_H

/*!
 * \brief Initialize the watchdog module containing all the scheduled watchdogs
 *
 * \details The module never receives the tick from its callers: every time it needs the current time it calls
 * get_tick, which is stored in the handler. The tick getter must be monotonic and non-blocking, and it is
 * called once for each API call (once per routine, however many watchdogs expire). If the tick can be updated by an
 * interrupt and cannot be read atomically on the target, the getter must take care of it.
 * The tick returned at initialization is used as the starting point for the temporal continuity check
 *
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param get_tick The function that returns the current tick, must not be NULL. Any tick source can be used, for
 * example a wrapper around timebase_get_tick, the HAL tick or the RTOS tick
 *
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog handler or the tick getter is NULL
 * \retval WATCHDOG_RC_ERROR An error occurred during the initialization of the watchdog handler
 * \retval WATCHDOG_RC_OK Otherwise
 */
enum WatchdogReturnCode watchdogs_api_init_pool(struct WatchdogHandler *watchdogs_handler, watchdog_tick_callback get_tick);

/*!
 * \brief The routine that should be called periodically to check if any watchdog has timed out and execute the corresponding callback
 *
 * \param watchdogs_handler A pointer to the watchdog handler structure
 *
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog handler is NULL
 * \retval WATCHDOG_RC_TEMPORAL_DISCONTINUITY The user tried to travel to the past
 * \retval WATCHDOG_RC_NOT_RUNNING The watchdog module is not enabled
 * \retval WATCHDOG_RC_ERROR An error occurred during the execution of the routine
 * \retval WATCHDOG_RC_OK Otherwise
 */
enum WatchdogReturnCode watchdogs_api_routine(struct WatchdogHandler *watchdogs_handler);

/*!
 * \brief Initialize a watchdog
 *
 * \param watchdog A pointer to the watchdog structure
 * \param timeout The timeout value
 * \param callback The timeout callback function
 *
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog is NULL
 * \retval WATCHDOG_RC_ERROR An error occurred during the initialization of the watchdog
 * \retval WATCHDOG_RC_OK Otherwise
 */
enum WatchdogReturnCode watchdogs_api_init_watchdog(struct Watchdog *watchdog, uint32_t timeout, watchdog_timeout_callback callback);

/*!
 * \brief Start a watchdog
 *
 * \details A timed out watchdog cannot be started
 * 
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param watchdog A pointer to the watchdog
 *
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog is NULL
 * \retval WATCHDOG_RC_BUSY The watchdog is already running
 * \retval WATCHDOG_RC_TIMED_OUT The watchdog has already timed out
 * \retval WATCHDOG_RC_UNINITIALIZED The watchdog is not initialized
 * \retval WATCHDOG_RC_TEMPORAL_DISCONTINUITY The user tried to travel to the past
 * \retval WATCHDOG_RC_OK Otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_start(struct WatchdogHandler *watchdogs_handler, struct Watchdog *watchdog);

/*!
 * \brief Stop a watchdog
 *
 * \details A timed out watchdog cannot be stopped
 *
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param watchdog A pointer to the watchdog
 *
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog is NULL
 * \retval WATCHDOG_RC_NOT_RUNNING The watchdog is not running
 * \retval WATCHDOG_RC_TIMED_OUT The watchdog has already timed out
 * \retval WATCHDOG_RC_OK Otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_stop(struct WatchdogHandler *watchdogs_handler, struct Watchdog *watchdog);

/*!
 * \brief Restarts a watchdog no matter its state
 *
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param watchdog A pointer to the watchdogd
 * 
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog is NULL
 * \retval WATCHDOG_RC_UNINITIALIZED The watchdog is not initialized
 * \retval WATCHDOG_RC_TEMPORAL_DISCONTINUITY The user tried to travel to the past
 * \retval WATCHDOG_RC_ERROR An error occurred during the execution of the function
 * \retval WATCHDOG_RC_OK Otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_restart(struct WatchdogHandler *watchdogs_handler, struct Watchdog *watchdog);

/*!
 * \brief Resets a watchdog to its initial state no matter its state
 *
 * \details A running watchdog is removed from the scheduled watchdogs without executing its callback.
 * A timed out or stopped watchdog is only moved back to the not running state. In every case the state
 * becomes WATCHDOG_STATE_NOT_RUNNING, so the watchdog can be started again with watchdogs_api_watchdog_start.
 * Resetting a watchdog that is already not running is not an error
 *
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param watchdog A pointer to the watchdog
 *
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog handler or the watchdog is NULL
 * \retval WATCHDOG_RC_UNINITIALIZED The watchdog is not initialized
 * \retval WATCHDOG_RC_TEMPORAL_DISCONTINUITY The user tried to travel to the past
 * \retval WATCHDOG_RC_ERROR An error occurred during the execution of the function
 * \retval WATCHDOG_RC_OK Otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_reset(struct WatchdogHandler *const watchdogs_handler, struct Watchdog *const watchdog);

/*!
 * \brief Replenishes the watchdog internal time
 *
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param watchdog A pointer to the watchdog
 *
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog is NULL
 * \retval WATCHDOG_RC_NOT_RUNNING The watchdog is not running
 * \retval WATCHDOG_RC_TIMED_OUT The watchdog has already timed out
 * \retval WATCHDOG_RC_UNINITIALIZED The watchdog is not initialized
 * \retval WATCHDOG_RC_TEMPORAL_DISCONTINUITY The user tried to travel to the past
 * \retval WATCHDOG_RC_ERROR An error occurred during the execution of the function
 * \retval WATCHDOG_RC_OK Otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_pet(struct WatchdogHandler *watchdogs_handler, struct Watchdog *watchdog);

/*!
 * \brief Check if the watchdog is running
 *
 * \param watchdog A pointer to the watchdog
 *
 * \returns bool True if the watchdog is running, false otherwise
 */
bool watchdogs_api_watchdog_is_running(struct Watchdog *watchdog);

/*!
 * \brief Check if the watchdog has timed out
 *
 * \param watchdog A pointer to the watchdog
 *
 * \returns bool True if the watchdog has timed out, false otherwise
 */
bool watchdogs_api_watchdog_is_timed_out(struct Watchdog *watchdog);

#endif /* WATCHDOGS_API_H */