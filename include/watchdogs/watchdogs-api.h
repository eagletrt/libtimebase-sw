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
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param current_tick The number of ticks that should elapse for the watchdog to time-out
 *
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog handler is NULL
 * \retval WATCHDOG_RC_ERROR An error occurred during the initialization of the watchdog handler
 * \retval WATCHDOG_RC_OK Otherwise
 */
enum WatchdogReturnCode watchdogs_api_init_pool(struct WatchdogHandler *watchdogs_handler, uint32_t current_tick);

/*!
 * \brief The routine that should be called periodically to check if any watchdog has timed out and execute the corresponding callback
 *
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param current_tick The current tick 
 *
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog handler is NULL
 * \retval WATCHDOG_RC_TEMPORAL_DISCONTINUITY The user tried to travel to the past
 * \retval WATCHDOG_RC_NOT_RUNNING The watchdog module is not enabled
 * \retval WATCHDOG_RC_ERROR An error occurred during the execution of the routine
 * \retval WATCHDOG_RC_OK Otherwise
 */
enum WatchdogReturnCode watchdogs_api_routine(struct WatchdogHandler *watchdogs_handler, uint32_t current_tick);

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
enum WatchdogReturnCode watchdogs_api_init_watchdog(struct Watchdog *const watchdog, uint32_t timeout, watchdog_timeout_callback callback);

/*!
 * \brief Start a watchdog
 *
 * \details A timed out watchdog cannot be started
 * 
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param watchdog A pointer to the watchdog
 * \param current_tick The current tick
 *
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog is NULL
 * \retval WATCHDOG_RC_BUSY The watchdog is already running
 * \retval WATCHDOG_RC_TIMED_OUT The watchdog has already timed out
 * \retval WATCHDOG_RC_UNINITIALIZED The watchdog is not initialized
 * \retval WATCHDOG_RC_TEMPORAL_DISCONTINUITY The user tried to travel to the past
 * \retval WATCHDOG_RC_OK Otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_start(struct WatchdogHandler *const watchdogs_handler, struct Watchdog *const watchdog, uint32_t current_tick);

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
enum WatchdogReturnCode watchdogs_api_watchdog_stop(struct WatchdogHandler *const watchdogs_handler, struct Watchdog *const watchdog);

/*!
 * \brief Pauses a watchdog
 *
 * \details A timed out watchdog cannot be paused
 * 
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param watchdog A pointer to the watchdog
 * \param current_tick The current tick
 *
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog is NULL
 * \retval WATCHDOG_RC_NOT_RUNNING The watchdog is not running
 * \retval WATCHDOG_RC_TIMED_OUT The watchdog has already timed out
 * \retval WATCHDOG_RC_UNINITIALIZED The watchdog is not initialized
 * \retval WATCHDOG_RC_TEMPORAL_DISCONTINUITY The user tried to travel to the past
 * \retval WATCHDOG_RC_ERROR An error occurred during the execution of the function
 * \retval WATCHDOG_RC_OK Otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_pause(struct WatchdogHandler *const watchdogs_handler, struct Watchdog *const watchdog, uint32_t current_tick);

/*!
 * \brief Restarts a watchdog no matter its state
 *
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param watchdog A pointer to the watchdogd
 * \param current_tick The current tick
 * 
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog is NULL
 * \retval WATCHDOG_RC_UNINITIALIZED The watchdog is not initialized
 * \retval WATCHDOG_RC_TEMPORAL_DISCONTINUITY The user tried to travel to the past
 * \retval WATCHDOG_RC_ERROR An error occurred during the execution of the function
 * \retval WATCHDOG_RC_OK Otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_restart(struct WatchdogHandler *const watchdogs_handler, struct Watchdog *const watchdog, uint32_t current_tick);

/*!
 * \brief Replenishes the watchdog internal time
 *
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param watchdog A pointer to the watchdog
 * \param current_tick The current tick
 *
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog is NULL
 * \retval WATCHDOG_RC_NOT_RUNNING The watchdog is not running
 * \retval WATCHDOG_RC_TIMED_OUT The watchdog has already timed out
 * \retval WATCHDOG_RC_UNINITIALIZED The watchdog is not initialized
 * \retval WATCHDOG_RC_TEMPORAL_DISCONTINUITY The user tried to travel to the past
 * \retval WATCHDOG_RC_ERROR An error occurred during the execution of the function
 * \retval WATCHDOG_RC_OK Otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_pet(struct WatchdogHandler *const watchdogs_handler, struct Watchdog *const watchdog, uint32_t current_tick);

/*!
 * \brief Causes the watchdog to time-out immediately and execute the corresponding callback
 * if the watchdog is running
 *
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param watchdog A pointer to the watchdog
 *
 * \retval WATCHDOG_RC_OK Otherwise
 * \retval WATCHDOG_RC_NOT_RUNNING The watchdog is not running
 * \retval WATCHDOG_RC_UNINITIALIZED The watchdog is not initialized
 * \retval WATCHDOG_RC_ERROR An error occurred during the execution of the function
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog is NULL
 */
enum WatchdogReturnCode watchdogs_api_watchdog_timeout(struct WatchdogHandler *watchdogs_handler, struct Watchdog *watchdog);

/*!
 * \brief Check if the watchdog is running
 *
 * \param watchdog A pointer to the watchdog
 *
 * \returns bool True if the watchdog is running, false otherwise
 */
bool watchdogs_api_watchdog_is_running(struct Watchdog *const watchdog);

/*!
 * \brief Check if the watchdog has timed out
 *
 * \param watchdog A pointer to the watchdog
 *
 * \returns bool True if the watchdog has timed out, false otherwise
 */
bool watchdogs_api_watchdog_is_timed_out(struct Watchdog *const watchdog);

/*!
 * \brief Enable a watchdog module
 *
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param current_tick The current tick
 *
 * \retval WATCHDOG_RC_OK Otherwise
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog handler is NULL
 * \retval WATCHDOG_RC_ERROR An error occurred during the execution of the function
 */
enum WatchdogReturnCode watchdogs_api_enable_pool(struct WatchdogHandler *watchdogs_handler, uint32_t current_tick);

/*!
 * \brief Disable a watchdog module, the watchdogs are frozen until the module is enabled again
 *
 * \param watchdogs_handler A pointer to the watchdog handler structure
 * \param current_tick The current tick
 *
 * \retval WATCHDOG_RC_OK Otherwise
 * \retval WATCHDOG_RC_NULL_POINTER The watchdog handler is NULL
 * \retval WATCHDOG_RC_ERROR An error occurred during the execution of the function
 */
enum WatchdogReturnCode watchdogs_api_disable_pool(struct WatchdogHandler *watchdogs_handler, uint32_t current_tick);

#endif /* WATCHDOGS_API_H */