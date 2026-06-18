/*!
 * \file watchdog-example.c
 * \date 2026-05-07
 * \authors Alessandro Giustina [giustinalessandro@gmail.com]
 *
 * \brief Simple example of the watchdog module usage.
 * \details In this example, we initialize the watchdog system and create three simple watchdog instances.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "watchdogs-api.h"
#include <inttypes.h>

void print_watchdog_1(void) {
    printf("Watchdog_1 executed");
    return;
}

void print_watchdog_2(void) {
    printf("Watchdog_2 executed");
    return;
}

void print_watchdog_3(void) {
    printf("Watchdog_3 executed");
    return;
}

int main(void) {

    /*
     * Initialize the watchdog pool with some watchdogs
     */

    struct WatchdogHandler watchdogs_handler;

    watchdogs_api_init_pool(&watchdogs_handler, 0U);
    struct Watchdog watchdog_1;
    struct Watchdog watchdog_2;
    struct Watchdog watchdog_3;

    watchdogs_api_init_watchdog(&watchdog_1, 5U, print_watchdog_1);
    watchdogs_api_init_watchdog(&watchdog_2, 7U, print_watchdog_2);
    watchdogs_api_init_watchdog(&watchdog_3, 10U, print_watchdog_3);

    /*
     * Go trough some ticks to see initialized behaviour.
     * We enable the watchdogs at tick 5, then we let them run until tick 25.
     * At tick 10 watchdog 1 should fire
     * At tick 12 watchdog 2 should fire
     * At tick 15 watchdog 3 should fire
     * 
     */

    for (uint32_t i = 0; i <= 25; i++) {

        if (i == 5) {
            watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_1, i);
            watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_2, i);
            watchdogs_api_watchdog_start(&watchdogs_handler, &watchdog_3, i);
        }

        printf("Tick: %" PRId32 "  -> ", i);
        watchdogs_api_routine(&watchdogs_handler, i);

        printf("\n");
    }

    /*
     * Restart watchdog 2 at tick 30 and pet it at tick 36, it should fire at tick 43.
     * 
     * At tick 30 restart also watchdog 3 that should fire after 10 ticks
     */

    for (uint32_t i = 26; i <= 60; i++) {

        if (i == 30) {
            watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_2, i);
            watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_3, i);
        }
        if (i == 36) {
            watchdogs_api_watchdog_pet(&watchdogs_handler, &watchdog_2, i);
        }

        printf("Tick: %" PRId32 "  -> ", i);
        watchdogs_api_routine(&watchdogs_handler, i);

        printf("\n");
    }

    /*
     * Restart all watchdogs at tick 60 they should fire at tick 65, 67 and 70 respectively.
     */

    watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_1, 60U);
    watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_2, 60U);
    watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_3, 60U);

    for (uint32_t i = 61; i <= 100; i++) {

        printf("Tick: %" PRId32 "  -> ", i);
        watchdogs_api_routine(&watchdogs_handler, i);

        printf("\n");
    }

    /*
     * We can see the same behaviour if the tick doesn't have a regular increment
     */

    /*
     * Expected behaviour:
     * At tick 100 all watchdogs are restarted, watchdog 1 should fire at tick 105, watchdog 2 at tick 107 and watchdog 3 at tick 110
     */

    watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_1, 100U);
    watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_2, 100U);
    watchdogs_api_watchdog_restart(&watchdogs_handler, &watchdog_3, 100U);

    for (uint32_t i = 100; i <= 130;) {

        printf("Tick: %" PRIu32 "  -> ", i);
        watchdogs_api_routine(&watchdogs_handler, i);

        printf("\n");

        // Increment tick by a random amount between 1 and 5
        i += rand() % 3 + 1;
    }

    return 0;
}