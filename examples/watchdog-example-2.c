/*!
 * \file watchdog-advanced-example.c
 * \date 2026-06-16
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 *
 * \brief Advanced example of the watchdog module usage.
 * \details Demonstrates petting, stopping, resetting, restarting a watchdog after a time-out, and state checking.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

#include "watchdogs-api.h"

/*
 * The watchdogs module reads the time through this function. Here the "clock" is a variable
 * that the example moves by hand, on a real target it would return the timebase, HAL or RTOS tick.
 */
static uint32_t current_tick;

static uint32_t get_tick(void) {
    return current_tick;
}

// --- Callbacks ---
void sensor_timeout_cb(void) {
    printf("[WARNING] Sensor watchdog timed out! ");
}

void comm_timeout_cb(void) {
    printf("[ERROR] Communication link lost! ");
}

void system_timeout_cb(void) {
    printf("[FATAL] System watchdog timed out! ");
}

int main(void) {
    struct WatchdogHandler watchdogs_handler;
    struct Watchdog sensor_wd = { 0 }; // watchdogs must be zero-initialized
    struct Watchdog comm_wd = { 0 };
    struct Watchdog system_wd = { 0 };

    /*
     * Initialize the watchdog pool at tick 0. From now on the module reads the time by itself
     * calling get_tick, so none of the following functions needs a tick parameter.
     */
    current_tick = 0U;
    watchdogs_api_init_pool(&watchdogs_handler, get_tick);

    /*
     * Initialize 3 different watchdogs with specific timeouts.
     */
    watchdogs_api_init_watchdog(&sensor_wd, 10U, sensor_timeout_cb); // Timeout: 10 ticks
    watchdogs_api_init_watchdog(&comm_wd, 15U, comm_timeout_cb);     // Timeout: 15 ticks
    watchdogs_api_init_watchdog(&system_wd, 25U, system_timeout_cb); // Timeout: 25 ticks

    /*
     * Start all watchdogs at tick 0.
     * Expected baseline triggers: Sensor at 10, Comm at 15, System at 25.
     */
    watchdogs_api_watchdog_start(&watchdogs_handler, &sensor_wd);
    watchdogs_api_watchdog_start(&watchdogs_handler, &comm_wd);
    watchdogs_api_watchdog_start(&watchdogs_handler, &system_wd);

    printf("--- Starting Watchdog Simulation ---\n");

    for (uint32_t i = 1; i <= 50; i++) {
        current_tick = i;
        printf("Tick: %2" PRIu32 " | ", i);

        /*
         * 1. Petting Example:
         * The sensor WD expects to fire at tick 10. By "petting" it at tick 5, 
         * we reset its timer, extending the trigger to tick 15 (5 + 10). 
         * Petting it again at tick 12 extends it to tick 22.
         */
        if (i == 5 || i == 12) {
            printf("[Petting Sensor WD] ");
            watchdogs_api_watchdog_pet(&watchdogs_handler, &sensor_wd);
        }

        /*
         * 2. Stopping Example:
         * The Comm WD is scheduled to fire at tick 15. We stop it gracefully 
         * at tick 10, meaning its callback should never execute.
         */
        if (i == 10) {
            printf("[Stopping Comm WD] ");
            watchdogs_api_watchdog_stop(&watchdogs_handler, &comm_wd);
        }

        /*
         * 3. Reset Example:
         * The System WD is scheduled to fire at tick 25. However, at tick 18, 
         * an external fault is detected and the supervised part is shut down, so the
         * watchdog is reset: it is removed from the scheduler without firing its callback.
         * It is started again at tick 20 and will fire at tick 45 (20 + 25).
         * Reset works in any state, so it is also the way to bring a timed-out watchdog
         * back to the not running state before starting it again.
         */
        if (i == 18) {
            printf("[Resetting System WD] ");
            watchdogs_api_watchdog_reset(&watchdogs_handler, &system_wd);
        }
        if (i == 20) {
            printf("[Starting System WD again] ");
            watchdogs_api_watchdog_start(&watchdogs_handler, &system_wd);
        }

        /*
         * Execute the routine to process scheduled watchdogs.
         */
        watchdogs_api_routine(&watchdogs_handler);

        /*
         * 4. State Checking Example:
         * At tick 16, verify that the Comm WD did not trigger and is safely inactive.
         */
        if (i == 16) {
            if (!watchdogs_api_watchdog_is_running(&comm_wd) && !watchdogs_api_watchdog_is_timed_out(&comm_wd)) {
                printf("(Verified: Comm WD is safely stopped and hasn't timed out) ");
            }
        }

        printf("\n");
    }

    printf("--- End of Simulation ---\n");
    return 0;
}