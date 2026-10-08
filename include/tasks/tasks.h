/*!
 * \brief tasks.h
 * \date 2024-05-15
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 * \author Antonio Gelain [antonio.gelain2@gmail.com]
 *
 * \brief Implementations of a simple task scheduler to run periodic tasks at certain intervals
 *
 */
#ifndef TASKS_H
#define TASKS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <eagletrt-api.h>

#include <min-heap-api.h>

/*!
 * \brief The max number of tasks that can be initialized
 */
#define MAX_TASKS (20U)

/*! 
 * \brief The callback signature of a task
 * \details The callback is executed by the routine and can call the tasks API, also on its own task
 * \param task_id The ID of the task that is being executed
 */
typedef void (*task_definition)(uint8_t task_id);

/*!
 * \brief Type definition for the function that returns the current tick
 * \details The module calls it by itself every time it needs the time, so the API functions do not take a tick
 * parameter. It must be monotonic, fast and, if the tick is updated by an interrupt and cannot be read atomically
 * on the target, it must protect the read
 * \return The current tick
 */
typedef uint32_t (*task_tick_callback)(void);

/*!
 * \brief The possible states of a task
 */
enum TaskState {
    TASKS_STATE_DISABLED = 0U, /*!< The task is disabled, will not be executed and on restart will start as it was first initialized*/
    TASKS_STATE_ENABLED,       /*!< The task is enabled, will be executed according to its schedule*/
    TASKS_STATE_PAUSED         /*!< The task is paused, will not be executed but can be resumed from where it was frozen*/
};

/*!
 * \brief Structure containing all the information that a single task needs to be initialized
 * 
 * \attention At initialization every task must have a unique identifier, the use of an enumerator is greatly encouraged
 * the IDs must be sequential and start from 0. No task can be paused before initialization.
 * 
 */
struct Task {
    uint8_t task_id;          /*!< The ID of the task*/
    enum TaskState state;     /*!< The state of the task*/
    uint8_t repeats;          /*!< The amount of time the task will fire (0 = infinite, 1 = one shot, ecc...)*/
    task_definition function; /*!< The callback of the task*/
    uint16_t interval;        /*!< The interval in-between task calls. */
    uint16_t start;           /*!< The delay from when task enabled gets set to when the task activates for the first time*/
    uint8_t int_repeats;      /*!< The original amount of repeats*/
    uint32_t last_update;     /*!< The last time the task was updated, in ticks (used for pause)*/
    uint32_t next_trigger;    /*!< The next time when the task should be called, in ticks */
};

/*! \brief The list of tasks */
typedef struct Task TaskList[MAX_TASKS];

/*!
 * \brief The main structure of the tasks module, containing all the necessary information to manage the tasks
 */
struct TasksHandler {
    uint32_t prev_tick;                         /*!< The last tick with wich the task module was called*/
    TaskList task_list;                         /*!< The list of tasks */
    uint8_t task_num;                           /*!< The number of tasks initialized */
    task_tick_callback get_tick;                /*!< Function callback for getting the tick*/
    enum TaskState actual_state[MAX_TASKS];     /*!< The actual state of the tasks in the heap, this array reflects it directly*/
    struct MinHeapHandler scheduled_tasks;      /*!< The heap containing the scheduled tasks */
    struct ArenaAllocatorHandler arena_handler; /*!< The arena allocator handler used to manage the memory of the scheduled tasks */
};

/*!
 * \brief The possible task return codes
 */
enum TasksReturnCode {
    TASKS_RC_OK,                     /*!< The operation was successful */
    TASKS_RC_INVALID_ID,             /*!< The given identifier does not exists */
    TASKS_RC_TEMPORAL_DISCONTINUITY, /*!< The user tried to travel to the past but time must go on */
    TASKS_RC_NULL_POINTER,           /*!< A null pointer was passed as argument */
    TASKS_RC_INVALID_LIST,           /*!< The given list of tasks is not valid, either because it contains a null task or because the number of tasks is 0 or greater than MAX_TASKS*/
    TASKS_RC_ERROR                   /*!< An error occurred during the operation, for example when updating the heap */
};

#endif // TASKS_H