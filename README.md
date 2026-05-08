# TIMEBASE

This library contains all the functionality needed to setup and run tasks and watchdog in an embedded system.

## General architecture

The library is divided into 3 main modules that can be used completely independently one from another.

### Timebase module
This module helps to convert from tick time into milliseconds. It is just an implementation of a counter with some converted getters.

### Tasks module
This module implements a task routine and all the functions needed for its functioning.
#### Usage
The user must provide to the initialization function 3 parameters:
 - `task_handler`: a pointer to a `TasksHandler` struct that will be used to handle the tasks module, this struct must be allocated by the user and it will be initialized by the library.
 - `t_list`: a pointer to an array of `Task` structs that will be used to store the tasks information, this array must be initialized by the user. See the example in `examples/tasks-example.c` for more info.
 - `num_tasks`: the number of tasks that the user wants to use, this number must be less than or equal to `MAX_TASKS` defined in `tasks-config.h` and has to be the same as the number of tasks in `t_list`.
 - `current_tick`: the current tick time, this value is needed to correctly schedule the tasks and it is used to avoid temporal discontinuities. The user must provide the current tick time at the moment of initialization, if the user wants to initialize the module at time 0, this value should be 0.

Once initialized, the user can enable, disable, pause and update the tasks as needed, the user just has to provide the current tick time at the moment of the operation to avoid temporal discontinuities and to ensure the consistency of the scheduling.

#### Behavior
The use of an enumerator is greatly recommended to define the task IDs, this way the code will be more readable and less error prone, but it is not mandatory as long as the user ensures that the task IDs are unique and sequential starting from 0. The required parameters for each task are the following:
 - `task_id`: the unique identifier of the task, this value must be unique and sequential starting from 0, it is used to identify the task in the API functions. (REQUIRED)
 - `task_state`: the initial state of the task, this value can be either enabled or disabled, if the task is enabled at initialization it will be scheduled to run for the first time at `current_tick + start`, where `start` is the start time of the task in ticks.
 - `one_shot`: a boolean value that indicates if the task is one-shot or not, if it is true the task will be automatically disabled after running for the first time, if it is false the task will be rescheduled to run again after `interval` ticks.
 - `task_function`: a pointer to a function that will be called when the task is triggered, this function must be defined by the user and it must have the following signature: `void callback(void)`. (REQUIRED)
 - `task_interval`: the interval time of the task in ticks, this value is used to calculate the next trigger time of the task after it has been triggered for the first time, if the task is one-shot this value is ignored (if not initialized or set to 0 then it will be treated as 1).
 - `task_start`: the start time of the task in ticks, this value is used to calculate the next trigger time of the task when it is enabled.

All the fields marked as required must be initialized by the user, otherwise the initialization will fail.

Tasks cannot be paused at initialization, they can only be enabled or disabled. If a task is enabled at initialization, it will be scheduled to run for the first time at `current_tick + start`, where `start` is the start time of the task in ticks.

When a task is enabled, it will be scheduled to run for the first time at `current_tick + start`, where `start` is the start time of the task in ticks. If the task is one-shot, it will be automatically disabled after running for the first time. If the task is not one-shot, it will be rescheduled to run again after `interval` ticks.

When a task is paused, it will not run until it is enabled again. When a paused task is enabled again, it will be scheduled to run for the first time at `current_tick + remaining_time`, where `remaining_time` is the remaining time until the next trigger of the task at the moment of pausing.

A disabled task can be paused and then enabled, in this case it will act as if it was paused at the moment of disabling, so when it is enabled again it will be scheduled to run for the first time at `current_tick + remaining_time`, where `remaining_time` is the remaining time until the next trigger of the task at the moment of disabling.
The same can be done from paused to disabled and then enabled again, in this case the behavior will be the same as a disabled task that is enabled again, so it will be scheduled to run for the first time at `current_tick + start`.

If the module is disabled the tasks will act as if they were frozen, so when the module is enabled again the tasks will be scheduled to run for the first time at `current_tick + remaining_time`, where `remaining_time` is the remaining time until the next trigger of the task at the moment of disabling.

If a task is updated while it is enabled, it will be rescheduled to run for the first time at `current_tick + new_start`, where `new_start` is the new start time of the task in ticks. If it isn't then the state will not change.

## Scripts
The library includes some scripts to compile and run the tests and examples, these scripts are located in the `scripts` folder and they are named `run-tests.sh` and `run-examples.sh`. These scripts will compile and run all the tests and examples respectively, they can be executed from the root of the project with the following commands:
 - `./scripts/run-tests.sh`
 - `./scripts/run-examples.sh <test_name_without_extension>`