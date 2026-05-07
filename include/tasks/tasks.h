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

/*!
 * \brief The possible states of a task
 */
enum TaskState {
    TASKS_STATE_DISABLED, /*!< The task is disabled, will not be executed and on restart will start immediately*/
    TASKS_STATE_ENABLED,  /*!< The task is enabled, will be executed according to its schedule*/
    TASKS_STATE_PAUSED    /*!< The task is paused, will not be executed but can be resumed from where it was frozen*/
};

/*!
 * \brief Structure containing all the information that a single task needs to be initialized
 * 
 * \attention At initialization every task must have a unique identifier, the use of an enumerator is greatly encouraged
 * the IDs must be sequential and start from 0.
 * 
 */
struct Task {
    uint8_t task_id;               /*!< The ID of the task*/
    enum TaskState task_state;     /*!< The state of the task*/
    bool one_shot;                 /*!< Whether the task is a one-shot task*/
    task_definition task_function; /*!< The callback of the task*/
    uint16_t task_interval;        /*!< The interval in-between task calls. */
    uint16_t task_start;           /*!< The delay from when task enabled gets set to when the task activates for the first time*/

    uint32_t last_update;  /*!< The last time the task was updated, in ticks (used for pause)*/
    uint32_t next_trigger; /*!< The next time when the task should be called, in ticks */
};

/*! \brief The list of tasks */
typedef struct Task TaskList[MAX_TASKS];

/*!
 * \brief The main structure of the tasks module, containing all the necessary information to manage the tasks
 */
struct TasksHandler {
    TaskList task_list; /*!< The list of tasks */
    uint8_t task_num;   /*!< The number of tasks initialized */

    enum TaskState actual_state[MAX_TASKS]; /*!< The general state of the task, used to manage the state of the task when it is paused or disabled*/

    struct MinHeapHandler scheduled_tasks;      /*!< The heap containing the scheduled tasks */
    struct ArenaAllocatorHandler arena_handler; /*!< The arena allocator handler used to manage the memory of the scheduled tasks */

    bool task_module_enabled; /*!< Whether the task module is enabled or not, used to disable the module when it is not needed */
};

/*!
 * \brief The possible task return codes
 */
enum TasksReturnCode {
    TASKS_RC_OK,           /*!< The operation was successful */
    TASKS_RC_DISABLED,     /*!< The tasks module is disabled, no operation can be performed */
    TASKS_RC_INVALID_ID,   /*!< The given identifier does not exists */
    TASKS_RC_NULL_POINTER, /*!< A null pointer was passed as argument */
    TASKS_RC_INVALID_LIST, /*!< The given list of tasks is not valid, either because it contains a null task or because the number of tasks is 0 or greater than MAX_TASKS*/
    TASKS_RC_ERROR         /*!< An error occurred during the operation, for example when updating the heap */

};

/*!
 * \brief Initialize the tasks module
 *
 * \param tasks_handler The pointer to the tasks handler structure, must not be NULL
 * \param t_list The list of tasks to initialize, must not be NULL and must contain at least one task, all the tasks must have a unique identifier starting from 0 and sequential
 * \param num_tasks The number of tasks in the list, must be greater than 0 and less than or equal to MAX_TASKS
 * \param current_tick The current tick count, used to calculate the next trigger time of the tasks
 * 
 * \retval TASKS_RC_OK The operation was successful
 * \retval TASKS_RC_NULL_POINTER A null pointer was passed as argument
 * \retval TASKS_RC_INVALID_LIST The given list of tasks is not valid, either because it contains a null task, because the number of tasks is 0 or greater than MAX_TASKS or because the task IDs are not unique and sequential starting from 0
 * \retval TASKS_RC_ERROR An error occurred during the operation
 */
enum TasksReturnCode tasks_init(struct TasksHandler *tasks_handler, TaskList t_list, uint8_t num_tasks, uint32_t current_tick);

/*!
 * \brief Routine to be called in the main loop to execute the scheduled tasks
 *
 * \param tasks_handler The pointer to the tasks handler structure, must not be NULL
 * \param current_tick The current tick count, used to check if any task needs to be executed
 * 
 * \retval TASKS_RC_OK The operation was successful
 * \retval TASKS_RC_NULL_POINTER A null pointer was passed as argument
 * \retval TASKS_RC_DISABLED The tasks module is disabled, no operation can be performed
 * \retval TASKS_RC_ERROR An error occurred during the operation
 */
enum TasksReturnCode tasks_routine(struct TasksHandler *tasks_handler, uint32_t current_tick);

/*!
 * \brief Enables a single task
 *
 * \param tasks_handler The pointer to the tasks handler structure, must not be NULL
 * \param task_id The identifier of the task
 * \param current_tick The current tick count
 * 
 * \retval TASKS_RC_OK The operation was successful
 * \retval TASKS_RC_NULL_POINTER A null pointer was passed as argument
 * \retval TASKS_RC_INVALID_ID The given identifier does not exist
 * \retval TASKS_RC_ERROR An error occurred during the operation
 */
enum TasksReturnCode tasks_enable(struct TasksHandler *tasks_handler, uint8_t task_id, uint32_t current_tick);

/*!
 * \brief Pauses a single task, the task can be resumed from where it was paused
 *
 * \param tasks_handler The pointer to the tasks handler structure, must not be NULL
 * \param task_id The identifier of the task
 * \param current_tick The current tick count
 * 
 * \retval TASKS_RC_OK The operation was successful
 * \retval TASKS_RC_NULL_POINTER A null pointer was passed as argument
 * \retval TASKS_RC_INVALID_ID The given identifier does not exist
 * \retval TASKS_RC_ERROR An error occurred during the operation
 */
enum TasksReturnCode tasks_pause(struct TasksHandler *tasks_handler, uint8_t task_id, uint32_t current_tick);

/*!
 * \brief Disables a single task, the task will be restarted from the beginning when enabled again
 *
 * \param tasks_handler The pointer to the tasks handler structure, must not be NULL
 * \param task_id The identifier of the task
 * \param current_tick The current tick count
 * 
 * \retval TASKS_RC_OK The operation was successful
 * \retval TASKS_RC_NULL_POINTER A null pointer was passed as argument
 * \retval TASKS_RC_INVALID_ID The given identifier does not exist
 * \retval TASKS_RC_ERROR An error occurred during the operation
 */
enum TasksReturnCode tasks_disable(struct TasksHandler *tasks_handler, uint8_t task_id, uint32_t current_tick);

/*!
 * \brief Get a single task information
 *
 * \param tasks_handler The pointer to the tasks handler structure, must not be NULL
 * \param task_id The identifier of the task
 * \param task The pointer to the task structure where the information will be stored, must not be NULL
 * 
 * \retval TASKS_RC_OK The operation was successful, the task information is stored in the provided pointer
 * \retval TASKS_RC_NULL_POINTER A null pointer was passed as argument
 * \retval TASKS_RC_INVALID_ID The given identifier does not exist
 */
enum TasksReturnCode tasks_get_task(const struct TasksHandler *tasks_handler, uint8_t task_id, const struct Task *task);

/*!
 * \brief Update a single task information, the task will be rescheduled according to the new information
 * if any changes are made to the interval or start time of the task they will take effect
 * only after the next time the task is called by the routine.
 *
 * \param tasks_handler The pointer to the tasks handler structure, must not be NULL
 * \param task_id The identifier of the task
 * \param new_interval The new interval of the task
 * \param new_start The new start time of the task
 * \param one_shot Whether the task is a one-shot task or not
 * \param current_tick The current tick count
 * 
 * \retval TASKS_RC_OK The operation was successful
 * \retval TASKS_RC_NULL_POINTER A null pointer was passed as argument
 * \retval TASKS_RC_INVALID_ID The given identifier does not exist
 * \retval TASKS_RC_ERROR An error occurred during the operation
 */
enum TasksReturnCode tasks_update_task(struct TasksHandler *tasks_handler, const uint8_t task_id, uint16_t new_interval, uint16_t new_start, bool one_shot, uint32_t current_tick);

/*!
 * \brief Enable the tasks module
 *
 * \param tasks_handler The pointer to the tasks handler structure, must not be NULL
 *
 * \retval TASKS_RC_OK The operation was successful
 * \retval TASKS_RC_NULL_POINTER A null pointer was passed as argument
 */
enum TasksReturnCode tasks_module_enable(struct TasksHandler *tasks_handler);

/*!
 * \brief Disable the tasks module
 *
 * \param tasks_handler The pointer to the tasks handler structure, must not be NULL
 *
 * \retval TASKS_RC_OK The operation was successful
 * \retval TASKS_RC_NULL_POINTER A null pointer was passed as argument
 */
enum TasksReturnCode tasks_module_disable(struct TasksHandler *tasks_handler);

#endif // TASKS_H
