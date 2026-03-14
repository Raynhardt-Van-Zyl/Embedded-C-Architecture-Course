/**
 * @file superloop.c
 * @brief Superloop (Cooperative Scheduler) Implementation
 * 
 * IMPLEMENTATION NOTES:
 * ====================
 * This implementation provides a production-grade cooperative scheduler suitable
 * for safety-critical embedded systems. Key design decisions:
 * 
 * 1. STATIC ALLOCATION: All data structures are statically allocated at compile
 *    time. No dynamic memory allocation (malloc/free) is used, ensuring
 *    predictable memory usage and eliminating fragmentation risks.
 * 
 * 2. DETERMINISTIC SCHEDULING: Tasks are always executed in priority order.
 *    Within the same priority, tasks are executed in registration order.
 *    This makes timing analysis straightforward.
 * 
 * 3. NON-BLOCKING DESIGN: All operations are O(1) or O(n) where n is the
 *    number of tasks. No blocking operations are performed.
 * 
 * 4. WATCHDOG INTEGRATION: The scheduler only feeds the watchdog when all
 *    registered tasks report healthy status, providing comprehensive
 *    system health monitoring.
 * 
 * 5. THREAD SAFETY: This implementation is designed for single-threaded
 *    execution. If using with ISRs, appropriate critical sections must
 *    be added by the application.
 * 
 * @author Embedded C Architecture Course
 * @version 1.0
 */

/*============================================================================*/
/*                             INCLUDES                                        */
/*============================================================================*/

#include "superloop.h"
#include <string.h>

/*============================================================================*/
/*                             PRIVATE MACROS                                  */
/*============================================================================*/

/**
 * @brief Minimum macro (type-safe)
 */
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))

/**
 * @brief Maximum macro (type-safe)
 */
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))

/**
 * @brief Convert milliseconds to system ticks
 */
#define MS_TO_TICKS(ms) ((ms) / SUPERLOOP_TICK_PERIOD_MS)

/**
 * @brief Invalid task ID marker
 */
#define INVALID_TASK_ID (-1)

/*============================================================================*/
/*                             PRIVATE DATA                                    */
/*============================================================================*/

/**
 * @brief Task control block storage
 * 
 * DESIGN NOTE: Static allocation ensures this array is placed in a known
 * memory location. The size is configurable via SUPERLOOP_MAX_TASKS.
 */
static TaskControlBlock_t s_tasks[SUPERLOOP_MAX_TASKS];

/**
 * @brief Number of registered tasks
 */
static uint32_t s_taskCount = 0U;

/**
 * @brief Scheduler running flag
 */
static volatile bool s_schedulerRunning = false;

/**
 * @brief Scheduler configuration (stored copy)
 */
static SchedulerConfig_t s_config = {0};

/**
 * @brief Scheduler statistics
 */
static SchedulerStats_t s_stats = {0};

/**
 * @brief Flag to indicate scheduler stop request
 */
static volatile bool s_stopRequested = false;

/*============================================================================*/
/*                             PRIVATE FUNCTIONS                               */
/*============================================================================*/

/**
 * @brief Find the highest priority task that is ready to run
 * 
 * @return Pointer to task control block, or NULL if no task is ready
 * 
 * ALGORITHM:
 * 1. Iterate through all registered tasks
 * 2. Skip tasks that are not enabled or not in READY state
 * 3. For periodic tasks, check if it's time to run
 * 4. Select the task with the highest priority (lowest number)
 * 
 * DESIGN NOTE: This is O(n) where n is the number of tasks. For small
 * task counts (typical in embedded systems), this is efficient enough.
 * For larger systems, consider using priority queues.
 */
static TaskControlBlock_t *FindReadyTask(void)
{
    TaskControlBlock_t *readyTask = NULL;
    TaskPriority_t highestPriority = TASK_PRIORITY_COUNT;
    uint32_t currentTime;
    uint32_t i;
    
    /* Get current time once to ensure consistency */
    currentTime = Superloop_GetTick();
    
    /* Search for highest priority ready task */
    for (i = 0U; i < s_taskCount; i++)
    {
        TaskControlBlock_t *task = &s_tasks[i];
        
        /* Skip disabled or suspended tasks */
        if (!task->enabled || (task->state != TASK_STATE_READY))
        {
            continue;
        }
        
        /* For periodic tasks, check if it's time to run */
        if (task->periodMs > 0U)
        {
            /* Handle tick overflow using unsigned arithmetic */
            uint32_t timeSinceLastRun = currentTime - task->lastRunTime;
            
            if (timeSinceLastRun < task->periodMs)
            {
                /* Not time to run this task yet */
                continue;
            }
        }
        
        /* Check if this task has higher priority than current best */
        if (task->priority < highestPriority)
        {
            readyTask = task;
            highestPriority = task->priority;
            
            /* CRITICAL priority tasks don't need further searching */
            if (highestPriority == TASK_PRIORITY_CRITICAL)
            {
                break;
            }
        }
    }
    
    return readyTask;
}

/**
 * @brief Execute a single task
 * 
 * @param task Pointer to task control block
 * 
 * DESIGN NOTE: This function:
 * 1. Updates task state to RUNNING
 * 2. Records start time for timing statistics
 * 3. Calls the task function
 * 4. Records end time and updates statistics
 * 5. Updates task state based on result
 */
static void ExecuteTask(TaskControlBlock_t *task)
{
    TaskResult_t result;
    uint32_t startTime = 0U;
    uint32_t endTime;
    uint32_t executionTime;
    
    /* Validate task pointer */
    if (task == NULL)
    {
        return;
    }
    
    /* Update state to running */
    task->state = TASK_STATE_RUNNING;
    
    /* Record start time if timing is enabled */
#if (SUPERLOOP_ENABLE_TIMING == 1U)
    if (s_config.getTimeUsFn != NULL)
    {
        startTime = s_config.getTimeUsFn();
    }
#endif
    
    /* Execute the task function */
    result = task->function();
    
    /* Record end time and calculate execution time */
#if (SUPERLOOP_ENABLE_TIMING == 1U)
    if (s_config.getTimeUsFn != NULL)
    {
        endTime = s_config.getTimeUsFn();
        executionTime = endTime - startTime; /* Handles overflow correctly */
        
        /* Update statistics */
        task->totalExecutions++;
        task->totalExecTimeUs += executionTime;
        
        /* Track worst-case execution time */
        if (executionTime > task->actualWcetUs)
        {
            task->actualWcetUs = executionTime;
            
            /* Check for WCET violation */
            if (executionTime > task->worstCaseTimeUs)
            {
                /* Task exceeded its declared WCET - this is a safety concern */
                task->health = TASK_HEALTH_WARNING;
            }
        }
    }
#endif
    
    /* Update task state based on result */
    switch (result)
    {
        case TASK_RESULT_OK:
        case TASK_RESULT_NO_ACTION:
            task->state = TASK_STATE_READY;
            /* Only set health to OK if not already in warning/critical */
            if (task->health == TASK_HEALTH_UNKNOWN)
            {
                task->health = TASK_HEALTH_OK;
            }
            break;
            
        case TASK_RESULT_YIELD:
            task->state = TASK_STATE_READY;
            break;
            
        case TASK_RESULT_ERROR:
        case TASK_RESULT_TIMEOUT:
            task->state = TASK_STATE_ERROR;
            task->health = TASK_HEALTH_CRITICAL;
            break;
            
        default:
            /* Unknown result - treat as error */
            task->state = TASK_STATE_ERROR;
            task->health = TASK_HEALTH_CRITICAL;
            break;
    }
    
    /* Update last run time */
    task->lastRunTime = Superloop_GetTick();
    
    /* Handle run-once tasks */
    if (task->runOnce)
    {
        task->enabled = false;
        task->runOnce = false;
    }
    
    /* Update statistics */
    s_stats.totalTaskRuns++;
}

/**
 * @brief Check if all enabled tasks are healthy
 * 
 * @return true if all tasks are healthy, false otherwise
 * 
 * DESIGN NOTE: This is used for watchdog integration. The watchdog
 * is only fed if all tasks report healthy status, ensuring that
 * a hung task will eventually cause a watchdog reset.
 */
static bool AllTasksHealthy(void)
{
    uint32_t i;
    
    for (i = 0U; i < s_taskCount; i++)
    {
        TaskControlBlock_t *task = &s_tasks[i];
        
        /* Only check enabled tasks */
        if (task->enabled)
        {
            if ((task->health == TASK_HEALTH_CRITICAL) ||
                (task->health == TASK_HEALTH_UNKNOWN))
            {
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief Feed the watchdog if conditions are met
 * 
 * DESIGN NOTE: The watchdog is fed only when:
 * 1. Watchdog is enabled in configuration
 * 2. A watchdog feed function is provided
 * 3. All enabled tasks report healthy status
 * 
 * This ensures that a failure in any task will cause a system reset.
 */
static void FeedWatchdog(void)
{
#if (SUPERLOOP_ENABLE_WATCHDOG == 1U)
    if (s_config.enableWatchdog && (s_config.watchdogFeedFn != NULL))
    {
        if (AllTasksHealthy())
        {
            s_config.watchdogFeedFn();
            s_stats.watchdogFeeds++;
        }
        /* If tasks are not healthy, don't feed watchdog - let it expire */
    }
#else
    (void)AllTasksHealthy; /* Suppress unused warning */
#endif
}

/**
 * @brief Initialize all task control blocks
 */
static void InitTaskBlocks(void)
{
    uint32_t i;
    
    for (i = 0U; i < SUPERLOOP_MAX_TASKS; i++)
    {
        TaskControlBlock_t *task = &s_tasks[i];
        
        task->function = NULL;
        task->initFunction = NULL;
        memset(task->name, 0, sizeof(task->name));
        task->periodMs = 0U;
        task->lastRunTime = 0U;
        task->nextRunTime = 0U;
        task->worstCaseTimeUs = 0U;
        
#if (SUPERLOOP_ENABLE_TIMING == 1U)
        task->actualWcetUs = 0U;
        task->totalExecutions = 0U;
        task->totalExecTimeUs = 0U;
#endif
        
        task->state = TASK_STATE_CREATED;
        task->priority = TASK_PRIORITY_NORMAL;
        task->health = TASK_HEALTH_UNKNOWN;
        task->enabled = false;
        task->runOnce = false;
    }
    
    s_taskCount = 0U;
}

/**
 * @brief Run initialization functions for all registered tasks
 * 
 * @return true if all initializations succeeded, false otherwise
 */
static bool InitializeAllTasks(void)
{
    uint32_t i;
    
    for (i = 0U; i < s_taskCount; i++)
    {
        TaskControlBlock_t *task = &s_tasks[i];
        
        if (task->initFunction != NULL)
        {
            if (!task->initFunction())
            {
                /* Initialization failed - mark task as error */
                task->state = TASK_STATE_ERROR;
                task->health = TASK_HEALTH_CRITICAL;
                return false;
            }
        }
        
        /* Mark task as ready to run */
        task->state = TASK_STATE_READY;
        task->enabled = true;
    }
    
    return true;
}

/*============================================================================*/
/*                             PUBLIC FUNCTIONS                                */
/*============================================================================*/

/**
 * @brief Initialize the superloop scheduler
 */
bool Superloop_Init(const SchedulerConfig_t *config)
{
    /* Validate configuration */
    if (config == NULL)
    {
        return false;
    }
    
    /* Store configuration */
    s_config = *config;
    
    /* Initialize task control blocks */
    InitTaskBlocks();
    
    /* Clear statistics */
    memset(&s_stats, 0, sizeof(s_stats));
    
    /* Clear flags */
    s_schedulerRunning = false;
    s_stopRequested = false;
    
    return true;
}

/**
 * @brief Register a task with the scheduler
 */
int32_t Superloop_RegisterTask(
    const char              *name,
    TaskFunction_t          function,
    TaskInitFunction_t      initFunction,
    TaskPriority_t          priority,
    uint32_t                periodMs,
    uint32_t                worstCaseTimeUs
)
{
    TaskControlBlock_t *task;
    int32_t taskId;
    
    /* Validate parameters */
    if (function == NULL)
    {
        return INVALID_TASK_ID;
    }
    
    if (priority >= TASK_PRIORITY_COUNT)
    {
        return INVALID_TASK_ID;
    }
    
    /* Check if we have room for another task */
    if (s_taskCount >= SUPERLOOP_MAX_TASKS)
    {
        return INVALID_TASK_ID;
    }
    
    /* Get pointer to next available task slot */
    taskId = (int32_t)s_taskCount;
    task = &s_tasks[taskId];
    
    /* Initialize task control block */
    task->function = function;
    task->initFunction = initFunction;
    task->priority = priority;
    task->periodMs = periodMs;
    task->worstCaseTimeUs = worstCaseTimeUs;
    task->lastRunTime = 0U;
    task->nextRunTime = 0U;
    task->state = TASK_STATE_CREATED;
    task->health = TASK_HEALTH_UNKNOWN;
    task->enabled = false;
    task->runOnce = false;
    
    /* Copy task name */
    if (name != NULL)
    {
        strncpy(task->name, name, SUPERLOOP_TASK_NAME_LEN - 1U);
        task->name[SUPERLOOP_TASK_NAME_LEN - 1U] = '\0';
    }
    
#if (SUPERLOOP_ENABLE_TIMING == 1U)
    task->actualWcetUs = 0U;
    task->totalExecutions = 0U;
    task->totalExecTimeUs = 0U;
#endif
    
    /* Increment task count */
    s_taskCount++;
    
    return taskId;
}

/**
 * @brief Start the scheduler main loop
 */
void Superloop_Run(void)
{
    /* Run task initialization functions */
    if (!InitializeAllTasks())
    {
        /* Initialization failed - don't start scheduler */
        return;
    }
    
    /* Mark scheduler as running */
    s_schedulerRunning = true;
    s_stopRequested = false;
    
    /* Main scheduler loop */
    while (s_schedulerRunning && !s_stopRequested)
    {
        TaskControlBlock_t *readyTask;
        
        /* Find highest priority ready task */
        readyTask = FindReadyTask();
        
        if (readyTask != NULL)
        {
            /* Execute the ready task */
            ExecuteTask(readyTask);
        }
        else
        {
            /* No tasks ready - could enter low-power mode here */
            s_stats.idleTimeMs++;
        }
        
        /* Feed watchdog if conditions are met */
        FeedWatchdog();
        
        /* Increment iteration counter */
        s_stats.totalIterations++;
    }
    
    /* Scheduler has stopped */
    s_schedulerRunning = false;
}

/**
 * @brief Request the scheduler to stop
 */
void Superloop_Stop(void)
{
    s_stopRequested = true;
}

/**
 * @brief Enable a task by ID
 */
bool Superloop_EnableTask(int32_t taskId)
{
    if ((taskId < 0) || ((uint32_t)taskId >= s_taskCount))
    {
        return false;
    }
    
    s_tasks[taskId].enabled = true;
    return true;
}

/**
 * @brief Disable a task by ID
 */
bool Superloop_DisableTask(int32_t taskId)
{
    if ((taskId < 0) || ((uint32_t)taskId >= s_taskCount))
    {
        return false;
    }
    
    s_tasks[taskId].enabled = false;
    return true;
}

/**
 * @brief Suspend a task temporarily
 */
bool Superloop_SuspendTask(int32_t taskId)
{
    if ((taskId < 0) || ((uint32_t)taskId >= s_taskCount))
    {
        return false;
    }
    
    s_tasks[taskId].state = TASK_STATE_SUSPENDED;
    return true;
}

/**
 * @brief Resume a suspended task
 */
bool Superloop_ResumeTask(int32_t taskId)
{
    if ((taskId < 0) || ((uint32_t)taskId >= s_taskCount))
    {
        return false;
    }
    
    s_tasks[taskId].state = TASK_STATE_READY;
    return true;
}

/**
 * @brief Get the current system tick count
 */
uint32_t Superloop_GetTick(void)
{
    if (s_config.getTickFn != NULL)
    {
        return s_config.getTickFn();
    }
    
    return 0U;
}

/**
 * @brief Check if a given timeout has elapsed
 */
bool Superloop_IsTimeoutElapsed(uint32_t startTime, uint32_t timeoutMs)
{
    uint32_t currentTime = Superloop_GetTick();
    uint32_t elapsed = currentTime - startTime; /* Handles overflow */
    
    return (elapsed >= timeoutMs);
}

/**
 * @brief Get scheduler statistics
 */
bool Superloop_GetStats(SchedulerStats_t *stats)
{
    if (stats == NULL)
    {
        return false;
    }
    
#if (SUPERLOOP_ENABLE_TIMING == 1U)
    *stats = s_stats;
    return true;
#else
    (void)stats;
    return false;
#endif
}

/**
 * @brief Update task health status
 */
bool Superloop_SetTaskHealth(int32_t taskId, TaskHealth_t health)
{
    if ((taskId < 0) || ((uint32_t)taskId >= s_taskCount))
    {
        return false;
    }
    
    s_tasks[taskId].health = health;
    return true;
}

/**
 * @brief Process scheduler housekeeping tasks
 */
void Superloop_Process(void)
{
    TaskControlBlock_t *readyTask;
    
    /* Find and execute one ready task */
    readyTask = FindReadyTask();
    
    if (readyTask != NULL)
    {
        ExecuteTask(readyTask);
    }
    
    /* Feed watchdog */
    FeedWatchdog();
    
    /* Increment iteration counter */
    s_stats.totalIterations++;
}
