/*!
 * \file watchdogs-api.h
 * \date 2024-04-16
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 * \author Antonio Gelain [antonio.gelain2@gmail.com]
 *
 * \brief Implementation of generic watchdogs that time-out after a certain interval of time 
 */

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
enum WatchdogReturnCode watchdogs_api_init_module(struct WatchdogHandler *watchdogs_handler, uint32_t current_tick);

/*!
 * \brief De-initialize the watchdog
 *
 * \param watchdog A pointer to the watchdog handler structure 
 *
 * \return WatchdogReturnCode
 *     - WATCHDOG_NULL_POINTER if the watchdog or the internal expire pointers are NULL
 *     - WATCHDOG_OK otherwise
 */
enum WatchdogReturnCode watchdogs_api_routine(WatchdogHandler *watchdogs_handler, uint32_t current_tick);

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

enum WatchdogReturnCode watchdogs_api_enable_module();

enum WatchdogReturnCode watchdogs_api_disable_module();