/**
 * @file superloop.h
 * @brief Superloop (Cooperative Scheduler) Architecture Header
 * 
 * ARCHITECTURE OVERVIEW:
 * =====================
 * The superloop is the simplest form of embedded task scheduling. It runs tasks
 * in a continuous while(1) loop, executing each task to completion before moving
 * to the next. This is a cooperative multitasking model - tasks must voluntarily
 * yield control by returning.
 * 
 * KEY CHARACTERISTICS:
 * - Deterministic execution order
 * - No context switch overhead
 * - Simple to debug and verify
 * - Tasks must be non-blocking
 * - Lower priority tasks can be starved by higher priority long-running tasks
 * 
 * SAFETY CONSIDERATIONS:
 * - All tasks must complete within their worst-case execution time (WCET)
 * - Watchdog must be serviced regularly
 * - Critical sections must be minimized
 * - Stack usage is predictable (single stack for all tasks)
 * 
 * WHEN TO USE:
 * - Simple embedded systems with well-defined task timing
 * - Safety-critical systems requiring deterministic behavior
 * - Resource-constrained MCUs without RTOS support
 * - Systems where task priorities are static and well-understood
 * 
 * @author Embedded C Architecture Course
 * @version 1.0
 * @date 2024
 */

#ifndef SUPERLOOP_H
#define SUPERLOOP_H

/*============================================================================*/
/*                             INCLUDES                                        */
/*============================================================================*/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*============================================================================*/
/*                             CONFIGURATION                                   */
/*============================================================================*/

/**
 * @brief Maximum number of tasks that can be registered
 * 
 * DESIGN NOTE: This is compile-time configurable to allow static allocation.
 * In embedded systems, we avoid dynamic memory allocation for safety and
 * predictability. Adjust based on your application requirements.
 */
#ifndef SUPERLOOP_MAX_TASKS
#define SUPERLOOP_MAX_TASKS         (16U)
#endif

/**
 * @brief System tick period in milliseconds
 * 
 * DESIGN NOTE: This defines the time base for task scheduling. A 1ms tick
 * provides good resolution for most applications while keeping ISR overhead
 * low. For ultra-low-power applications, consider increasing this value.
 */
#ifndef SUPERLOOP_TICK_PERIOD_MS
#define SUPERLOOP_TICK_PERIOD_MS    (1U)
#endif

/**
 * @brief Maximum task name length for debugging
 */
#define SUPERLOOP_TASK_NAME_LEN     (16U)

/**
 * @brief Enable task execution time monitoring
 * 
 * When enabled, the scheduler tracks how long each task executes and can
 * flag tasks that exceed their declared worst-case execution time.
 */
#ifndef SUPERLOOP_ENABLE_TIMING
#define SUPERLOOP_ENABLE_TIMING     (1U)
#endif

/**
 * @brief Enable watchdog integration
 * 
 * When enabled, tasks can report their health status and the scheduler
 * will only kick the watchdog if all registered tasks are healthy.
 */
#ifndef SUPERLOOP_ENABLE_WATCHDOG
#define SUPERLOOP_ENABLE_WATCHDOG   (1U)
#endif

/*============================================================================*/
/*                             TYPE DEFINITIONS                                */
/*============================================================================*/

/**
 * @brief Task priority levels
 * 
 * DESIGN NOTE: Using an enum with explicit values makes priority comparisons
 * efficient and self-documenting. Lower numbers = higher priority (run first).
 * 
 * PRIORITY GUIDELINES:
 * - CRITICAL: Safety-critical tasks (watchdog, fault handling)
 * - HIGH: Time-sensitive I/O (communication protocols)
 * - NORMAL: Regular application tasks
 * - LOW: Background tasks (logging, diagnostics)
 * - IDLE: Tasks that only run when nothing else needs to run
 */
typedef enum {
    TASK_PRIORITY_CRITICAL  = 0U,   /* Highest priority - always runs first */
    TASK_PRIORITY_HIGH      = 1U,
    TASK_PRIORITY_NORMAL    = 2U,   /* Default priority for most tasks */
    TASK_PRIORITY_LOW       = 3U,
    TASK_PRIORITY_IDLE      = 4U,   /* Lowest priority - runs when idle */
    TASK_PRIORITY_COUNT     = 5U    /* Number of priority levels */
} TaskPriority_t;

/**
 * @brief Task state enumeration
 * 
 * STATE TRANSITIONS:
 * - CREATED -> READY: After successful registration
 * - READY -> RUNNING: When scheduler selects the task
 * - RUNNING -> READY: Task returns and is rescheduled
 * - READY -> SUSPENDED: Task is explicitly suspended
 * - SUSPENDED -> READY: Task is resumed
 * - Any state -> ERROR: Task reports an error
 */
typedef enum {
    TASK_STATE_CREATED      = 0U,   /* Task structure initialized */
    TASK_STATE_READY        = 1U,   /* Task is ready to run */
    TASK_STATE_RUNNING      = 2U,   /* Task is currently executing */
    TASK_STATE_SUSPENDED    = 3U,   /* Task is temporarily disabled */
    TASK_STATE_ERROR        = 4U    /* Task has encountered an error */
} TaskState_t;

/**
 * @brief Task execution result
 * 
 * DESIGN NOTE: Tasks return a status to indicate their execution result.
 * This allows the scheduler to track task health and take appropriate action
 * if a task fails.
 */
typedef enum {
    TASK_RESULT_OK          = 0U,   /* Task completed successfully */
    TASK_RESULT_YIELD       = 1U,   /* Task yields voluntarily (cooperative) */
    TASK_RESULT_ERROR       = 2U,   /* Task encountered an error */
    TASK_RESULT_TIMEOUT     = 3U,   /* Task timed out waiting for resource */
    TASK_RESULT_NO_ACTION   = 4U    /* Task had nothing to do */
} TaskResult_t;

/**
 * @brief Task health status for watchdog integration
 */
typedef enum {
    TASK_HEALTH_UNKNOWN     = 0U,   /* Health status not yet determined */
    TASK_HEALTH_OK          = 1U,   /* Task is healthy */
    TASK_HEALTH_WARNING     = 2U,   /* Task has minor issues */
    TASK_HEALTH_CRITICAL    = 3U    /* Task has serious issues */
} TaskHealth_t;

/**
 * @brief Task function pointer type
 * 
 * DESIGN NOTE: Task functions take no parameters and return a TaskResult_t.
 * This keeps the interface simple and consistent. If a task needs parameters,
 * it should use a static or global structure (common in embedded systems).
 * 
 * IMPORTANT: Task functions MUST be non-blocking. They should:
 * - Check if work needs to be done and return immediately if not
 * - Perform a small chunk of work and return if more work remains
 * - Never wait indefinitely for external events (use timeouts)
 */
typedef TaskResult_t (*TaskFunction_t)(void);

/**
 * @brief Task initialization function pointer type
 * 
 * Called once during scheduler initialization to set up task resources.
 * Returns true if initialization succeeded, false otherwise.
 */
typedef bool (*TaskInitFunction_t)(void);

/**
 * @brief Task control block
 * 
 * DESIGN NOTE: This structure contains all the state information for a task.
 * It is designed to be efficient in memory usage while providing all necessary
 * information for scheduling and monitoring.
 * 
 * MEMORY LAYOUT: Fields are ordered to minimize padding on 32-bit systems.
 * - Pointer fields first (4 or 8 bytes aligned)
 * - 32-bit fields next
 * - 16-bit fields
 * - 8-bit fields last
 */
typedef struct TaskControlBlock {
    /* Function pointers */
    TaskFunction_t          function;           /* Task execution function */
    TaskInitFunction_t      initFunction;       /* Optional init function (can be NULL) */
    
    /* Task identification */
    const char              name[SUPERLOOP_TASK_NAME_LEN]; /* Task name for debugging */
    
    /* Scheduling parameters */
    uint32_t                periodMs;           /* Task period (0 = run every iteration) */
    uint32_t                lastRunTime;        /* Last execution time (system ticks) */
    uint32_t                nextRunTime;        /* Next scheduled execution time */
    uint32_t                worstCaseTimeUs;    /* Declared WCET in microseconds */
    
    /* Runtime statistics */
#if (SUPERLOOP_ENABLE_TIMING == 1U)
    uint32_t                actualWcetUs;       /* Measured worst-case execution time */
    uint32_t                totalExecutions;    /* Number of times task has run */
    uint32_t                totalExecTimeUs;    /* Cumulative execution time */
#endif
    
    /* Task state */
    TaskState_t             state;              /* Current task state */
    TaskPriority_t          priority;           /* Task priority level */
    TaskHealth_t            health;             /* Task health for watchdog */
    
    /* Flags */
    bool                    enabled;            /* Task is enabled for scheduling */
    bool                    runOnce;            /* Run task once, then disable */
    
} TaskControlBlock_t;

/**
 * @brief Scheduler statistics structure
 */
typedef struct SchedulerStats {
    uint32_t                totalIterations;    /* Total scheduler loop iterations */
    uint32_t                totalTaskRuns;      /* Total task executions */
    uint32_t                missedDeadlines;    /* Tasks that missed their period */
    uint32_t                watchdogFeeds;      /* Number of watchdog feeds */
    uint32_t                maxLoopTimeUs;      /* Maximum loop iteration time */
    uint32_t                idleTimeMs;         /* Accumulated idle time */
} SchedulerStats_t;

/**
 * @brief Scheduler configuration structure
 */
typedef struct SchedulerConfig {
    bool                    enableWatchdog;     /* Enable watchdog integration */
    bool                    enableStatistics;   /* Enable statistics collection */
    uint32_t                watchdogTimeoutMs;  /* Watchdog timeout period */
    void                    (*watchdogFeedFn)(void); /* Watchdog feed function */
    uint32_t                (*getTickFn)(void); /* System tick getter function */
    uint32_t                (*getTimeUsFn)(void); /* Microsecond timer function */
} SchedulerConfig_t;

/*============================================================================*/
/*                             API FUNCTIONS                                   */
/*============================================================================*/

/**
 * @brief Initialize the superloop scheduler
 * 
 * @param config Pointer to configuration structure
 * @return true if initialization succeeded, false otherwise
 * 
 * DESIGN NOTE: Must be called before any task registration. Initializes
 * all task control blocks to a known state and sets up system resources.
 */
bool Superloop_Init(const SchedulerConfig_t *config);

/**
 * @brief Register a task with the scheduler
 * 
 * @param name Task name (for debugging)
 * @param function Task execution function
 * @param initFunction Optional initialization function (can be NULL)
 * @param priority Task priority level
 * @param periodMs Task period in milliseconds (0 = run every iteration)
 * @param worstCaseTimeUs Declared worst-case execution time in microseconds
 * @return Task ID (>= 0) on success, -1 on failure
 * 
 * DESIGN NOTE: Tasks should be registered during system initialization,
 * before the scheduler starts running. The scheduler uses static allocation,
 * so the maximum number of tasks is fixed at compile time.
 */
int32_t Superloop_RegisterTask(
    const char              *name,
    TaskFunction_t          function,
    TaskInitFunction_t      initFunction,
    TaskPriority_t          priority,
    uint32_t                periodMs,
    uint32_t                worstCaseTimeUs
);

/**
 * @brief Start the scheduler main loop
 * 
 * @note This function does not return under normal operation
 * 
 * DESIGN NOTE: Enters an infinite loop that:
 * 1. Checks for tasks ready to run
 * 2. Executes the highest priority ready task
 * 3. Feeds the watchdog if all tasks are healthy
 * 4. Repeats forever
 * 
 * The scheduler can be stopped by calling Superloop_Stop() from within
 * a task or interrupt handler.
 */
void Superloop_Run(void);

/**
 * @brief Request the scheduler to stop
 * 
 * DESIGN NOTE: Sets a flag that is checked at the beginning of each
 * scheduler iteration. The scheduler will exit its main loop on the
 * next iteration after this is called.
 */
void Superloop_Stop(void);

/**
 * @brief Enable a task by ID
 * 
 * @param taskId Task ID returned from registration
 * @return true if task was enabled, false if invalid ID
 */
bool Superloop_EnableTask(int32_t taskId);

/**
 * @brief Disable a task by ID
 * 
 * @param taskId Task ID returned from registration
 * @return true if task was disabled, false if invalid ID
 */
bool Superloop_DisableTask(int32_t taskId);

/**
 * @brief Suspend a task temporarily
 * 
 * @param taskId Task ID returned from registration
 * @return true if task was suspended, false if invalid ID
 */
bool Superloop_SuspendTask(int32_t taskId);

/**
 * @brief Resume a suspended task
 * 
 * @param taskId Task ID returned from registration
 * @return true if task was resumed, false if invalid ID
 */
bool Superloop_ResumeTask(int32_t taskId);

/**
 * @brief Get the current system tick count
 * 
 * @return Current tick count in milliseconds
 * 
 * DESIGN NOTE: This function wraps the system tick getter provided
 * during initialization. It is safe to call from tasks and ISRs.
 */
uint32_t Superloop_GetTick(void);

/**
 * @brief Check if a given timeout has elapsed
 * 
 * @param startTime Start time from Superloop_GetTick()
 * @param timeoutMs Timeout period in milliseconds
 * @return true if timeout has elapsed, false otherwise
 * 
 * DESIGN NOTE: Handles tick counter overflow correctly by using
 * unsigned arithmetic. This is a common pattern in embedded systems.
 */
bool Superloop_IsTimeoutElapsed(uint32_t startTime, uint32_t timeoutMs);

/**
 * @brief Get scheduler statistics
 * 
 * @param stats Pointer to statistics structure to fill
 * @return true if statistics were copied, false if disabled
 */
bool Superloop_GetStats(SchedulerStats_t *stats);

/**
 * @brief Update task health status
 * 
 * @param taskId Task ID returned from registration
 * @param health New health status
 * @return true if health was updated, false if invalid ID
 * 
 * DESIGN NOTE: Tasks should call this to report their health status.
 * The watchdog is only fed if all registered tasks report healthy status.
 */
bool Superloop_SetTaskHealth(int32_t taskId, TaskHealth_t health);

/**
 * @brief Process scheduler housekeeping tasks
 * 
 * DESIGN NOTE: Called automatically by the scheduler, but can also be
 * called manually if needed (e.g., in a custom main loop).
 */
void Superloop_Process(void);

#endif /* SUPERLOOP_H */
