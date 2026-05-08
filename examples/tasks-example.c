/*!
 * \file tasks-example.c
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
