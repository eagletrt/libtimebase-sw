/*!
 * \brief tasks-api.h
 * \date 2024-05-15
 * \author Alessandro Giustina [giustinalessandro@gmail.com]
 * \author Antonio Gelain [antonio.gelain2@gmail.com]
 *
 * \brief Implementations of a simple task scheduler to run periodic tasks at certain intervals
 *
 */
#ifndef TASKS_API_H
#define TASKS_API_H

#include <tasks.h>

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
enum TasksReturnCode tasks_get_task(struct TasksHandler *tasks_handler, uint8_t task_id, struct Task *task);

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

#endif // TASKS_API_H
