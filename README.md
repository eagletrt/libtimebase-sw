# TIMEBASE

This library contains all the functionality needed to set up and run periodic tasks and watchdogs in an embedded system, all driven by a single tick counter.

## General architecture

The library is divided into 3 main modules that can be used completely independently one from another.

| Module | Purpose | Headers |
|--------|---------|---------|
| Timebase | Tick counter with tick <-> milliseconds conversion | `timebase.h`, `timebase-api.h` |
| Tasks | Scheduler for periodic / one-shot / N-shot functions | `tasks.h`, `tasks-api.h` |
| Watchdogs | Software watchdogs that fire a callback if not petted in time | `watchdogs.h`, `watchdogs-api.h` |

Tasks and watchdogs do **not** depend on the timebase module: they only need a monotonically increasing `uint32_t` tick, which can come from the timebase module or from any other source (HAL tick, RTOS tick, ...).

### Dependencies

 - `eagletrt-api.h`: common macros (`EAGLETRT_STATIC`, `EAGLETRT_VOLATILE`, `EAGLETRT_API_MAX`).
 - `min-heap-api` and `arena-allocator-api`: used by tasks and watchdogs to keep the scheduled items ordered. All memory is static, each handler owns its own arena, nothing is allocated on the system heap.
 - Unit tests use [Unity](https://github.com/ThrowTheSwitch/Unity) and [fff](https://github.com/meekrosoft/fff).

### Common conventions

 - **Where the tick comes from**: the library never reads a clock by itself. The tasks and watchdogs modules take a tick getter function (`uint32_t get_tick(void)`) at initialization and call it whenever they need the time, so no function of these modules takes a tick parameter (see [Tick getter](#tick-getter)).
 - **Temporal discontinuity**: if the tick returned by the getter is lower than the last tick the module has seen, the function returns `*_RC_TEMPORAL_DISCONTINUITY` and does nothing. Time can only move forward.
 - Every function checks its pointers and returns `*_RC_NULL_POINTER` if one of them is `NULL`.
 - Callbacks (tasks functions and watchdog timeouts) are executed **inside the routine of their module**, never from an interrupt.
 - The modules are not thread-safe. The only function meant to be called from an interrupt is `timebase_inc_tick`. Everything else (routines, enable/pause/start/pet, ...) must be called from the same context.

### Tick getter
Tasks and watchdogs receive a function with the signature `uint32_t get_tick(void)` when they are initialized (`tasks_api_init`, `watchdogs_api_init_pool`), it must not be `NULL` (the init function returns `*_RC_NULL_POINTER` otherwise). The function is stored in the handler and called by the module itself, which makes it possible to use the modules from code that has no access to the current tick (and no reason to). Any monotonic tick source works: the timebase, the HAL tick, the RTOS tick... A few rules:

 - The getter is called **once per API call**. In particular the routines read the tick a single time, so everything that is due in the same call is compared against the same instant, no matter how long the callbacks take. A callback that calls the API (for example to restart its own watchdog or to pause its own task) causes a new read, so the operation uses the time of that call.
 - The getter must be monotonic and fast, it is called from the same context as the module functions.
 - If the tick is updated by an interrupt and a `uint32_t` cannot be read atomically on the target (8 and 16 bit MCUs), the getter must protect the read, for example by disabling interrupts. The modules do not do it.
 - `timebase_get_tick` takes a pointer to the timebase, so it cannot be used directly as a getter. Write a small wrapper, as in the setup example below.
 - A handler that was not successfully initialized has no getter: the tasks functions return `TASKS_RC_NULL_POINTER` when they are called on it.

### Typical setup

```c
#include "timebase-api.h"
#include "tasks-api.h"
#include "watchdogs-api.h"

enum TasksNames { TASK_LED = 0, TASK_LOG, TASK_COUNT };

static struct TimebaseHandler timebase;
static struct TasksHandler tasks;
static struct WatchdogHandler watchdogs;
static struct Watchdog can_watchdog = { 0 }; // must be zero-initialized

static void led_toggle(void);
static void log_status(void);
static void can_timeout(void);

// Tick getter of the watchdogs module, any monotonic tick source works
static uint32_t get_tick(void) {
    return timebase_get_tick(&timebase);
}

static TaskList task_list = {
    { .task_id = TASK_LED, .state = TASKS_STATE_ENABLED, .repeats = 0U, .function = led_toggle, .interval = 500U, .start = 0U },
    { .task_id = TASK_LOG, .state = TASKS_STATE_ENABLED, .repeats = 3U, .function = log_status, .interval = 1000U, .start = 100U },
};

// Called every 1 ms by the hardware timer
void SysTick_Handler(void) {
    timebase_inc_tick(&timebase);
}

int main(void) {
    timebase_api_init(&timebase, 1U);
    timebase_set_enable(&timebase, true);

    tasks_api_init(&tasks, task_list, TASK_COUNT, get_tick);

    watchdogs_api_init_pool(&watchdogs, get_tick);
    watchdogs_api_init_watchdog(&can_watchdog, 200U, can_timeout);
    watchdogs_api_watchdog_start(&watchdogs, &can_watchdog);

    for (;;) {
        tasks_api_routine(&tasks);
        watchdogs_api_routine(&watchdogs);
    }
}
```

---

## Timebase module
This module helps to convert from tick time into milliseconds. It is just an implementation of a counter with some converted getters.

#### Usage
 - `timebase_api_init(handler, resolution_ms)`: initializes the handler. `resolution_ms` is the number of milliseconds that a single tick represents, if 0 is given it defaults to 1. The counter is set to 0 and the timebase starts **disabled**. The handler must be allocated by the user.
 - `timebase_set_enable(handler, enabled)`: starts or stops the counter.
 - `timebase_inc_tick(handler)`: increments the counter by one tick, to be called from the timer interrupt. Returns `TIMEBASE_RC_DISABLED` (and does not increment) if the timebase is disabled.
 - `timebase_get_tick(handler)`: the current number of ticks.
 - `timebase_get_time(handler)`: the elapsed time in ms (`ticks * resolution`).
 - `timebase_get_resolution(handler)`: the number of ms per tick.

The getters return 0 if the handler is `NULL`. The macros `TIMEBASE_MS_TO_TICKS(T, RES)` and `TIMEBASE_TICKS_TO_MS(T, RES)` can be used to convert values at compile time.

#### Behavior
 - Disabling the timebase freezes the counter, it does not reset it. Re-initializing the handler resets the counter and disables it again.
 - `TIMEBASE_MS_TO_TICKS` truncates. A duration smaller than the resolution converts to 0 ticks, keep this in mind when computing task intervals or watchdog timeouts (a watchdog with a timeout of 0 is rejected, a task interval of 0 is treated as 1).
 - The counter is a `uint32_t`: at 1 ms per tick it wraps after about 49.7 days.

---

## Tasks module
This module implements a task scheduler and all the functions needed for its functioning. Tasks are kept in a min-heap ordered by next trigger time (ties are broken by task ID), so the routine only has to look at the tasks that are actually due.

#### Usage
The user must provide to the initialization function `tasks_api_init` 4 parameters:
 - `tasks_handler`: a pointer to a `TasksHandler` struct that will be used to handle the tasks module, this struct must be allocated by the user and it will be initialized by the library.
 - `t_list`: a pointer to an array of `Task` structs that will be used to store the tasks information, this array must be initialized by the user. See the example in `examples/tasks-example.c` for more info. The list is **copied** into the handler, so it can be discarded after initialization.
 - `num_tasks`: the number of tasks that the user wants to use, this number must be greater than 0 and less than or equal to `MAX_TASKS` (defined in `tasks.h`, 20 by default) and has to be the same as the number of tasks in `t_list`. The library cannot check the real length of the array, so a wrong value results in reading out of bounds.
 - `get_tick`: the tick getter, a function with the signature `uint32_t get_tick(void)` that the module calls whenever it needs the current tick (see [Tick getter](#tick-getter)). It must not be `NULL`, otherwise the initialization returns `TASKS_RC_NULL_POINTER`. The tick returned during the initialization is used to schedule the enabled tasks and as the starting point to avoid temporal discontinuities.

Once initialized, the user calls `tasks_api_routine(handler)` periodically (e.g. in the main loop) and can enable, disable, pause and update the tasks as needed with `tasks_api_enable_task(handler, id)`, `tasks_api_pause_task(handler, id)`, `tasks_api_disable_task(handler, id)` and `tasks_api_update_task(handler, id, ...)`. These functions use the tick returned by the getter at the moment of the call. `tasks_api_get_task` copies the current information of a task into a user provided `Task`.

#### Task parameters
The use of an enumerator is greatly recommended to define the task IDs, this way the code will be more readable and less error prone, but it is not mandatory as long as the user ensures that the task IDs are unique and sequential starting from 0. The parameters of each task are the following:
 - `task_id`: the unique identifier of the task, this value must be unique and sequential starting from 0 (it must be equal to the index of the task in the list), it is used to identify the task in the API functions. (REQUIRED)
 - `function`: a pointer to a function that will be called when the task is triggered, this function must be defined by the user and it must have the following signature: `void callback(uint8_t task_id)`, where `task_id` is the ID of the task that is being executed. (REQUIRED)
 - `state`: the initial state of the task, this value can be either `TASKS_STATE_ENABLED` or `TASKS_STATE_DISABLED`. If it is not set the task starts disabled. A task cannot be initialized as paused.
 - `repeats`: the number of times the task will fire, `0` means indefinitely, `1` means one-shot, `N` means exactly N times. If not set it defaults to 0. Maximum value is 255.
 - `interval`: the time between two consecutive triggers, in ticks. If not set or set to 0 it is treated as 1. Maximum value is 65535.
 - `start`: the delay, in ticks, between the moment the task is enabled and its first trigger. If not set it is 0, meaning the task fires the first time the routine is called. Maximum value is 65535.

All the fields marked as required must be initialized by the user, otherwise the initialization fails with `TASKS_RC_INVALID_LIST`.

#### Behavior
When a task is enabled (at initialization or with `tasks_api_enable_task`) it is scheduled to run for the first time at `tick + start`, where `tick` is the value returned by the tick getter when the task is enabled. After each execution it is rescheduled at `previous_trigger + interval`, so the period does not drift even if the routine is called late. When a task with `repeats` has run for the requested number of times it is automatically **disabled**.

The behavior of every state change is summarized in this table:

| Transition | Behavior |
|------------|----------|
| same state -> same state | Nothing happens |
| ENABLED -> PAUSED | The task is removed from the scheduler and the moment of the pause is remembered |
| ENABLED -> DISABLED | The task is removed from the scheduler |
| PAUSED -> ENABLED | If the task was not overdue when paused, it is scheduled at `tick + remaining_time`, where `remaining_time` is the time that was left until its next trigger at the moment of pausing. The number of repeats that were left is preserved. If the task was overdue when paused (its trigger time had already passed but the routine had not run yet), it behaves as a DISABLED -> ENABLED transition |
| DISABLED -> ENABLED | The task is scheduled at `tick + start` and the repeats counter is reset to the initial value |
| PAUSED -> DISABLED | Only the state changes. When enabled again the task restarts from the beginning |
| DISABLED -> PAUSED | Not allowed, `tasks_api_pause_task` returns `TASKS_RC_ERROR` for a disabled task |

`tasks_api_update_task(handler, id, new_interval, new_start, repeats)` changes the parameters of a task, including the number of repeats (which also becomes the new initial value). If the task is enabled it is rescheduled immediately at `tick + new_start` with the repeats counter reset. If the task is paused or disabled the new values are just stored and the state does not change: a paused task keeps its remaining time, a disabled task uses the new `start` the next time it is enabled.

Calling a function on a task that is already in the requested state (enable an enabled task, disable a disabled one, ...) is not an error and returns `TASKS_RC_OK`.

#### Routine
`tasks_api_routine` executes, in order, every task whose trigger time is less than or equal to the tick returned by the getter (read once at the beginning of the routine). If the routine is called late, a task that missed several triggers is executed once for each missed trigger (all in the same call) until it has caught up, and each execution consumes one repeat. Choose a routine period that is small compared to the shortest task interval, the counter for dropped tasks is updated accordingly and saturates at UINT16_MAX.

#### Managing tasks from a callback
A callback can call any function of the tasks API, **including on its own task** (`tasks_api_disable_task`, `tasks_api_pause_task`, `tasks_api_enable_task`, `tasks_api_update_task`). To make this possible the routine updates the task *before* calling its callback: the task is already rescheduled at its next trigger (`previous_trigger + interval`) and its repeat has already been consumed. If it was the last repeat the task is already `DISABLED`. This means that, from inside the callback:

 - **Disable**: the task does not run again. The call returns `TASKS_RC_OK`.
 - **Pause**: the time left until the next trigger is preserved, so a later enable resumes the task with that remaining time. If the routine is running late and the next trigger is already overdue, the pause behaves as described in the `PAUSED -> ENABLED` row of the table above.
 - **Update**: the task is rescheduled with the new parameters, as if `tasks_api_update_task` had been called from outside the routine.
 - **Enable on the last repeat**: a task that has just consumed its last repeat is already `DISABLED`, so enabling it from its own callback restarts it from the beginning (`tick + start`, repeats reset).
 - **Enable/disable of other tasks**: works as usual.

Every call made from a callback reads the tick again, so it may be later than the tick the routine is working with if time passed while the callbacks were running. The tick source must never go backwards, otherwise the calls return `TASKS_RC_TEMPORAL_DISCONTINUITY`.

**Warning**: if a callback reschedules its own task at the current tick (for example re-enabling a one-shot task with `start` set to 0, or updating itself with `new_start` set to 0) and the tick has not advanced yet, the task is due again in the same call of the routine, runs again, and so on: the routine never returns. Use a `start` greater than 0 for tasks that restart themselves.

#### Return codes
`TASKS_RC_OK`, `TASKS_RC_INVALID_ID` (unknown task ID), `TASKS_RC_TEMPORAL_DISCONTINUITY`, `TASKS_RC_NULL_POINTER`, `TASKS_RC_INVALID_LIST` (bad list given to init) and `TASKS_RC_ERROR` (internal error, for example a heap operation failed, or trying to pause a disabled task).

---

## Watchdogs module
This module implements generic software watchdogs. A watchdog has a timeout in ticks and a callback: if the watchdog is not "petted" before the timeout expires, the callback is executed by the routine. Running watchdogs are kept in a min-heap ordered by expiration time.

#### Usage
 - `watchdogs_api_init_pool(handler, get_tick)`: initializes the watchdog handler that contains all the scheduled watchdogs. `get_tick` is the function the module calls whenever it needs the current tick, it has the signature `uint32_t get_tick(void)` and must not be `NULL` (`WATCHDOG_RC_NULL_POINTER` otherwise). The handler must be allocated by the user. At most `MAX_WATCHDOGS` (defined in `watchdogs.h`, 20 by default) can run at the same time.
 - `watchdogs_api_init_watchdog(watchdog, timeout, callback)`: initializes a single watchdog. `timeout` is in ticks and must be greater than 0, `callback` has the signature `void callback(void)` and must not be `NULL`. The `struct Watchdog` must be **zero-initialized** before the call (`struct Watchdog wd = { 0 };` or a static variable), and a watchdog can be initialized only once.
 - `watchdogs_api_watchdog_start(handler, watchdog)`: starts the watchdog, it will expire at `tick + timeout`, where `tick` is the value returned by the tick getter.
 - `watchdogs_api_watchdog_stop(handler, watchdog)`: stops a running watchdog without firing the callback.
 - `watchdogs_api_watchdog_pet(handler, watchdog)`: replenishes a running watchdog, the new expiration is `tick + timeout`. This is the function to call periodically from the code that is being supervised.
 - `watchdogs_api_watchdog_restart(handler, watchdog)`: starts the watchdog no matter its state, a timed-out watchdog included. To bring a timed-out watchdog back to the not running state without starting it use `reset`.
 - `watchdogs_api_watchdog_reset(handler, watchdog)`: resets the watchdog to its initial state no matter its state. A running watchdog is removed from the heap without firing the callback, a timed-out one is cleared, and in every case the watchdog ends up `NOT_RUNNING` and can be started again with `start`. Resetting a watchdog that is already not running is not an error.
 - `watchdogs_api_watchdog_is_running(watchdog)` / `watchdogs_api_watchdog_is_timed_out(watchdog)`: state getters, they return `false` for a `NULL` watchdog.
 - `watchdogs_api_routine(handler)`: to be called periodically, it checks which watchdogs have expired and executes their callbacks.

The `struct Watchdog` is owned by the user and must stay valid (and at the same address) for as long as it is running, since the handler stores a pointer to it.

#### Behavior
A watchdog can be in one of three states:

| State | Meaning | Allowed operations |
|-------|---------|--------------------|
| `WATCHDOG_STATE_NOT_RUNNING` | Initialized (stopped or reset), not scheduled | `start`, `reset`, `restart` |
| `WATCHDOG_STATE_RUNNING` | Scheduled in the heap | `stop`, `pet`, `reset`, `restart` |
| `WATCHDOG_STATE_TIMED_OUT` | Expired, callback has been called, no longer scheduled | `reset`, `restart` |

`start` on a running watchdog returns `WATCHDOG_RC_BUSY`. `start`, `stop` and `pet` on a timed-out watchdog return `WATCHDOG_RC_TIMED_OUT`. `stop` and `pet` on a watchdog that is not running return `WATCHDOG_RC_NOT_RUNNING`. `reset` never fails because of the state of the watchdog. Any operation on a watchdog that was never initialized returns `WATCHDOG_RC_UNINITIALIZED`.

When the routine finds an expired watchdog (`next_trigger <= tick`) it sets its state to `TIMED_OUT`, removes it from the heap and then calls its callback, so a callback can safely call `watchdogs_api_watchdog_restart` (or `reset` followed by `start`) on its own watchdog to re-arm it. Calling only `start` from the callback fails with `WATCHDOG_RC_TIMED_OUT`, because the watchdog is already timed out at that point.

#### Temporal continuity
The watchdogs module records the last tick only inside `watchdogs_api_routine` (the tasks module records it in every call that changes something). `start`, `pet`, `reset` and `restart` compare the tick returned by the getter with the tick of the last routine call, and return `WATCHDOG_RC_TEMPORAL_DISCONTINUITY` if it is lower (for example if the timebase was re-initialized), but they do not update it. See [Tick getter](#tick-getter) for the rules of the tick getter.

#### Return codes
`WATCHDOG_RC_OK`, `WATCHDOG_RC_NULL_POINTER`, `WATCHDOG_RC_TIMED_OUT`, `WATCHDOG_RC_TEMPORAL_DISCONTINUITY`, `WATCHDOG_RC_ERROR`, `WATCHDOG_RC_BUSY`, `WATCHDOG_RC_NOT_RUNNING`, `WATCHDOG_RC_UNINITIALIZED`.

---

## Known limitations

 - **Tick wrap-around**: ticks are `uint32_t` and the modules compare them directly, so after the counter wraps (about 49.7 days at 1 ms per tick) the schedule breaks and calls are rejected as temporal discontinuity. Re-initialize the modules before that happens if the system can stay up that long.
 - **Late routine calls**: see the routine section of the tasks module, missed triggers are all executed on the next call.
 - **Tasks restarting themselves**: a callback can manage its own task (see the tasks module), but a task that reschedules itself at the current tick (`start` equal to 0) makes the routine loop forever.
 - **Callback duration**: callbacks run inside the routines, a long callback delays every other task and watchdog.
 - **Limits**: at most `MAX_TASKS` tasks and `MAX_WATCHDOGS` watchdogs; `interval` and `start` are 16 bit, `repeats` is 8 bit.

## Tests
Unit tests for each module are in `test-timebase.c`, `test-tasks.c` and `test-watchdogs.c`.