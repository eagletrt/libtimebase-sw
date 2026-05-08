/*!
 * \file watchdogs.h
 * \date 2024-04-16
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 * \author Antonio Gelain [antonio.gelain2@gmail.com]
 *
 * \brief Implementation of generic watchdogs that time-out a certain interval of time 
 */

#ifndef WATCHDOGS_H
#define WATCHDOGS_H

#include <stdbool.h>

#include "mainboard-conf.h"
#include "mainboard-def.h"

#define MAX_WATCHDOGS (20U)

/*!
 * \brief Return code for the watchdog module functions
 *
 * \details
 *     - WATCHDOG_OK the function exeuted succesfully
 *     - WATCHDOG_NULL_POINTER a NULL pointer was given to a function
 *     - WATCHDOG_BUSY the watchdog is already running
 *     - WATCHDOG_TIMED_OUT the watchdog has timed out
 *     - WATCHDOG_NOT_RUNNING the watchdog is not running
 *     - WATCHDOG_UNAVAILABLE the watchdog is not registered inside the timebase
 */
enum WatchdogReturnCode {
    WATCHDOG_RC_OK,
    WATCHDOG_RC_NULL_POINTER,
    WATCHDOG_RC_BUSY,
    WATCHDOG_RC_TIMED_OUT,
    WATCHDOG_RC_NOT_RUNNING,
    WATCHDOG_RC_UNAVAILABLE
};

enum WatchdogState {
    WATCHDOG_NOT_RUNNING,
    WATCHDOG_RUNNING,
    WATCHDOG_PAUSED,
    WATCHDOG_TIMED_OUT
};

/*!
 * \brief Type definition for a function that is called when the watchdog times-out
 * 
 * \details When the watchdog times-out it unregister itself from the timebase automatically
 */
typedef void (*watchdog_timeout_callback_t)(void);

/*!
 * \brief Definiton of the watchdog structure handler
 *
 * \param running True if the watchdog is running, false otherwise
 * \param timed_out True if the watchdog is running, false otherwise
 * \param timeout The number of ticks that should elapse for the watchdog to time-out
 * \param expire The function that is called when the watchdog times-out
 */
struct Watchdog {

    enum WatchdogState watchdog_state;

    uint32_t timeout;
    uint32_t last_updated;

    watchdog_timeout_callback_t expire;
};

struct WatchdogHandler {
    struct MinHeapHandler scheduled_watchdogs;  /*!< The heap containing the scheduled tasks */
    struct ArenaAllocatorHandler arena_handler; /*!< The arena allocator handler used to manage the memory of the scheduled tasks */

    uint32_t prev_tick;
};

/*!
 * \brief Initialize the watchdog
 *
 * \param watchdog A pointer to the watchdog handler structure
 * \param timeout The number of ticks that should elapse for the watchdog to time-out
 * \param expire The function that is called when the watchdog times-out
 *
 * \return WatchdogReturnCode
 *     - WATCHDOG_NULL_POINTER if the watchdog or the expire pointers are NULL
 *     - WATCHDOG_BUSY if the watchdog is already running
 *     - WATCHDOG_OK otherwise
 */
enum WatchdogReturnCode watchdogs_api_module_init(
    WatchdogHandler *watchdogs_handler,
    uint32_t current_tick);

/*!
 * \brief De-initialize the watchdog
 *
 * \param watchdog A pointer to the watchdog handler structure 
 *
 * \return WatchdogReturnCode
 *     - WATCHDOG_NULL_POINTER if the watchdog or the internal expire pointers are NULL
 *     - WATCHDOG_OK otherwise
 */
enum WatchdogReturnCode watchdogs_api_module_routine(WatchdogHandler *watchdogs_handler, uint32_t current_tick);

/*!
 * \brief Start a watchdog
 *
 * \details A timed out watchdog cannot be started
 *
 * \param watchdog A pointer to the watchdog handler structure
 *
 * \return WatchdogReturnCode
 *     - WATCHDOG_NULL_POINTER if the watchdog is NULL
 *     - WATCHDOG_BUSY if the watchdog is already running
 *     - WATCHDOG_TIMED_OUT if the watchdog has already timed out
 *     - WATCHDOG_UNAVAILABLE if the watchdog can't be registered
 *     - WATCHDOG_OK otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_start(Watchdog *const watchdog);

/*!
 * \brief Stop a watchdog
 *
 * \details A timed out watchdog cannot be stopped
 *
 * \param watchdog A pointer to the watchdog handler structure
 *
 * \return WatchdogReturnCode
 *     - WATCHDOG_NULL_POINTER if the watchdog is NULL
 *     - WATCHDOG_NOT_RUNNING if the watchdog is not running
 *     - WATCHDOG_TIMED_OUT if the watchdog has already timed out
 *     - WATCHDOG_OK otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_stop(Watchdog *const watchdog);

/*!
 * \brief Stop a watchdog
 *
 * \details A timed out watchdog cannot be stopped
 *
 * \param watchdog A pointer to the watchdog handler structure
 *
 * \return WatchdogReturnCode
 *     - WATCHDOG_NULL_POINTER if the watchdog is NULL
 *     - WATCHDOG_NOT_RUNNING if the watchdog is not running
 *     - WATCHDOG_TIMED_OUT if the watchdog has already timed out
 *     - WATCHDOG_OK otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_pause(Watchdog *const watchdog);

/*!
 * \brief Start a watchdog even if it has timed out
 *
 * \details If the watchdog is not running it is started
 * as the watchdog start function
 *
 * \param watchdog A pointer to the watchdog handler structure
 *
 * \return WatchdogReturnCode
 *     - WATCHDOG_NULL_POINTER if the watchdog is NULL
 *     - WATCHDOG_UNAVAILABLE if the watchdog can't be registered
 *     - WATCHDOG_OK otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_restart(Watchdog *const watchdog);

/*!
 * \brief Reset the watchdog internal time to 0
 *
 * \details The watchdog is not stopped after the reset
 *
 * \param watchdog A pointer to the watchdog
 *
 * \return WatchogReturnCode
 *     - WATCHDOG_NULL_POINTER if the watchdog is NULL
 *     - WATCHDOG_NOT_RUNNING if the watchdog is not running
 *     - WATCHDOG_TIMED_OUT if the watchdog has already timed out
 *     - WATCHDOG_UNAVAILABLE if the watchdog can't be registered inside the timebase
 *     - WATCHDOG_OK otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_pet(Watchdog *const watchdog);

/*!
 * \brief Set the watchdog status as timed out
 *
 * \param watchdog A pointer to the watchdog
 *
 * \return WatchdogReturnCode
 *     - WATCHDOG_NULL_POINTER A NULL pointer was given as parameter
 *     - WATCHDOG_NOT_RUNNING if the watchdog is not running
 *     - WATCHDOG_TIMED_OUT if the watchdog has already timed out
 *     - WATCHDOG_OK otherwise
 */
enum WatchdogReturnCode watchdogs_api_watchdog_timeout(Watchdog *const watchdog);

/*!
 * \param Check if the watchdog has timed out
 *
 * \param watchdog A pointer to the watchdog
 *
 * \return bool True if the watchdog has timed out, false otherwise
 */
bool watchdogs_api_watchdog_is_timed_out(Watchdog *const watchdog);

#endif // WATCHDOGS_H
