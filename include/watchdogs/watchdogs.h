/*!
 * \file watchdogs.h
 * \date 2024-04-16
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 * \author Antonio Gelain [antonio.gelain2@gmail.com]
 *
 * \brief Implementation of generic watchdogs that time-out after a certain interval of time 
 */

#ifndef WATCHDOGS_H
#define WATCHDOGS_H

#include <stdbool.h>

#include "arena-allocator-api.h"
#include "min-heap-api.h"

#define MAX_WATCHDOGS (20U)

/*!
 * \brief Return code for the watchdog module functions
 */
enum WatchdogReturnCode {
    WATCHDOG_RC_OK,                     /*!< The function executed successfully */
    WATCHDOG_RC_NULL_POINTER,           /*!< A NULL pointer was given to a function */
    WATCHDOG_RC_TIMED_OUT,              /*!< The watchdog has timed out */
    WATCHDOG_RC_TEMPORAL_DISCONTINUITY, /*!< The user tried to travel to the past but time must go on */
    WATCHDOG_RC_ERROR,                  /*!< An error occurred during the execution of the function */
    WATCHDOG_RC_BUSY,                   /*!< The watchdog is already running */
    WATCHDOG_RC_NOT_RUNNING,            /*!< The watchdog is not running */
    WATCHDOG_RC_UNINITIALIZED,          /*!< The watchdog is not initialized */
};

/*!
 * \brief State of the watchdog
 */
enum WatchdogState {
    WATCHDOG_STATE_NOT_RUNNING, /*!< The watchdog is not running */
    WATCHDOG_STATE_RUNNING,     /*!< The watchdog is running */
    WATCHDOG_STATE_PAUSED,      /*!< The watchdog is paused */
    WATCHDOG_STATE_TIMED_OUT    /*!< The watchdog has timed out */
};

/*!
 * \brief Type definition for a function that is called when the watchdog times-out
 * 
 * \details When the watchdog times-out it unregister itself from the timebase automatically
 */
typedef void (*watchdog_timeout_callback_t)(void);

/*!
 * \brief Definiton of a single watchdog
 *
 * \param running True if the watchdog is running, false otherwise
 * \param timed_out True if the watchdog is running, false otherwise
 * \param timeout The number of ticks that should elapse for the watchdog to time-out
 * \param watchdog_callback The function that is called when the watchdog times-out
 */
struct Watchdog {

    enum WatchdogState watchdog_state; /*!< The state of the watchdog */

    watchdog_timeout_callback_t watchdog_callback; /*!< The function that is called when the watchdog times-out */

    uint32_t timeout; /*!< The number of ticks that should elapse for the watchdog to time-out */

    uint32_t next_trigger; /*!< The tick at which the watchdog will time-out */
    uint32_t last_update;  /*!< The last tick at which the watchdog was updated */

    bool is_initialized; /*!< True if the watchdog is initialized, false otherwise */
};

/*!
 * \brief Definition of the watchdog handler structure
 */
struct WatchdogHandler {
    struct MinHeapHandler scheduled_watchdogs;  /*!< The heap containing the scheduled tasks */
    struct ArenaAllocatorHandler arena_handler; /*!< The arena allocator handler used to manage the memory of the scheduled tasks */

    bool watchdog_module_enabled; /*!< True if the watchdog module is enabled, false otherwise */
    uint32_t prev_tick;           /*!< The last tick at which the watchdogs were updated used for temporal continuity disabling the module*/
};

#endif // WATCHDOGS_H
