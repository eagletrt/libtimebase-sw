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

#include <min-heap-api.h>

/*!
 * \brief The max number of tasks that can be initialized
 */
#define MAX_TASKS (20U)

typedef void (*task_definition)(void);

enum Task_state {
    TASK_STATE_DISABLED,
    TASK_STATE_ENABLED,
    TASK_STATE_PAUSED
};

/*!
 * \brief Structure containing all the information that a single task needs to be initialized
 * 
 * \attention At initialization every task must have a unique identifier, the use of an enumerator is greatly encouraged
 * the IDs must be sequential and start from 0.
 * 
 */
struct Task {
    uint8_t task_id;            /*!< The ID of the task*/
    enum Task_state task_state; /*!< The state of the task*/
    bool one_shot;
    task_definition task_function; /*!< The callback of the task*/
    uint16_t task_interval;        /*!< The interval in-between task calls. */
    uint16_t task_start;           /*!< The delay from when task enabled gets set to when the task activates for the first time*/

    uint32_t last_update;  /*!< The last time the task was updated, in ticks */
    uint32_t next_trigger; /*!< The next time when the task should be called, in ticks */
};

typedef struct Task TaskList[MAX_TASKS];

struct TasksHandler {
    TaskList task_list;
    uint8_t task_num;

    enum Task_state general_state[MAX_TASKS];

    struct MinHeapHandler scheduled_tasks;
    struct ArenaAllocatorHandler arena_handler;
};

/*!
 * \brief The possible task return codes
 */
enum TasksReturnCode {
    TASKS_RC_OK,
    TASKS_RC_INVALID_ID,
    TASKS_RC_NULL_POINTER,
    TASKS_RC_INVALID_LIST,
    TASKS_RC_ERROR

};

/*!
 * \brief Initialize the tasks module
 *
 * \param resolution The timebase resolution
 *
 * \return TasksReturnCode
 *     - TASKS_RC_OK
 */
TasksReturnCode tasks_init(milliseconds_t resolution);

/*!
 * \brief Get a pointer to the tasks
 *
 * \param id The identifier of the task
 *
 * \return Task* The pointer to the tasks or NULL if the id is not valid
 */
Task *tasks_get_task(const TasksId id);

/*!
 * \brief Get the start time of the task
 *
 * \param id The identifier of the task
 *
 * \return ticks_t The task start time or 0 if the id is not valid
 */
ticks_t tasks_get_start(const TasksId id);

/*!
 * \brief Get the interval time of the task
 *
 * \param id The identifier of the task
 *
 * \return ticks_t The task interval time or 0 if the id is not valid
 */
ticks_t tasks_get_interval(const TasksId id);

/*!
 * \brief Get a pointer to the task callback
 *
 * \param id The identifier of the task
 *
 * \return tasks_callback The task callback or NULL if the id is not valid
 */
tasks_callback tasks_get_callback(const TasksId id);

/*!
 * \brief Enable or disable a single task
 *
 * \param id The task identifier
 * \param enabled True to enable the tasks, false to disable
 *
 * \return TasksReturnCode
 *     - TASKS_RC_INVALID_ID the given identifier does not exists
 *     - TASKS_RC_OK otherwise
 */
TasksReturnCode tasks_set_enable(const TasksId id, const bool enabled);

/*!
 * \brief Check if a task is enabled or not
 *
 * \param id The task identifier
 *
 * \return True if the task is enabled, false otherwise
 */
bool tasks_is_enabled(const TasksId id);

#else // CONF_TASKS_MODULE_ENABLE

#define tasks_init(resolution) (TASKS_RC_OK)
#define tasks_get_task(id) (NULL)
#define tasks_get_start(id) (0U)
#define tasks_get_interval(id) (0U)
#define tasks_get_callback(id) (NULL)

#endif // CONF_TASKS_MODULE_ENABLE

#endif // TASKS_H
