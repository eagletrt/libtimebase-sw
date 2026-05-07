/*!
 * \file timebase-basic.c
 * \date 2026-05-07
 * \authors Alessandro Giustina [giustinalessandro@gmail.com]
 *
 * \brief Simple example of the timebase module usage.
 * \details In this example, we initialize the timebase with a resolution
 *      of 10 ms and enable it.  
 *      We then simulate periodic timer ticks by incrementing the timebase
 *      counter in a loop.  
 *      During execution, the current tick count and elapsed time in
 *      milliseconds are printed to the console.  
 *      Finally, the timebase is disabled and the program terminates.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "timebase-api.h"

int main(void) {

    struct TimebaseHandler timebase_handler;

    /**
     * Initialize the timebase with
     * a resolution of 10 ms.
     */
    if (timebase_api_init(&timebase_handler, 10U) != TIMEBASE_RC_OK) {
        printf("[ERROR]: Cannot initialize timebase\n");
        return -1;
    }

    /**
     * Enable the timebase before
     * incrementing ticks.
     */
    timebase_set_enable(&timebase_handler, true);

    printf("Timebase resolution: %lu ms\n",
           (unsigned long)timebase_get_resolution(&timebase_handler));

    /**
     * Simulate periodic timer interrupts
     * by incrementing the tick counter.
     */
    for (uint32_t i = 0; i < 10U; ++i) {
        if (timebase_inc_tick(&timebase_handler) != TIMEBASE_RC_OK) {
            printf("[ERROR]: Cannot increment tick\n");
            return -1;
        }

        printf("Tick: %lu | Time: %lu ms\n",
               (unsigned long)timebase_get_tick(&timebase_handler),
               (unsigned long)timebase_get_time(&timebase_handler));
    }

    /**
     * Disable the timebase and check
     * that ticking is no longer allowed.
     */
    timebase_set_enable(&timebase_handler, false);

    if (timebase_inc_tick(&timebase_handler) == TIMEBASE_RC_DISABLED)
        printf("Timebase correctly disabled\n");

    return 0;
}