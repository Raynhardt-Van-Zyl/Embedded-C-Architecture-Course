/**
 * @file app_assert.c
 * @brief Embedded Assertion System Implementation
 * 
 * This file implements the assertion handling system. Key features:
 * 
 * - Captures processor state at failure point
 * - Maintains circular log of recent assertions
 * - Supports custom handlers for production recovery
 * - Provides statistics for health monitoring
 * 
 * Platform Adaptation:
 * - PC/LR capture is ARM Cortex-M specific
 * - Reset mechanism uses NVIC SystemReset
 * - Debug break uses BKPT instruction
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 */

#include "app_assert.h"
#include <string.h>

/*============================================================================*/
/*                          PLATFORM ADAPTATION                                */
/*============================================================================*/

#ifdef __ARM_ARCH
    #include "cmsis_compiler.h"
    #include "core_cm4.h"
    
    #define GET_TIMESTAMP()         (SysTick->VAL)
    #define CAPTURE_PC(pc)          __asm volatile("mov %0, pc" : "=r"(pc))
    #define CAPTURE_LR(lr)          __asm volatile("mov %0, lr" : "=r"(lr))
    #define SYSTEM_RESET()          NVIC_SystemReset()
    #define DEBUG_BREAK()           __BKPT(0)
    #define DISABLE_INTERRUPTS()    __disable_irq()
    #define ENABLE_INTERRUPTS()     __enable_irq()
    
#else
    #include <stdio.h>
    #include <stdlib.h>
    
    #define GET_TIMESTAMP()         (0)
    #define CAPTURE_PC(pc)          ((void)((pc) = 0))
    #define CAPTURE_LR(lr)          ((void)((lr) = 0))
    #define SYSTEM_RESET()          exit(1)
    #define DEBUG_BREAK()           ((void)0)
    #define DISABLE_INTERRUPTS()    ((void)0)
    #define ENABLE_INTERRUPTS()     ((void)0)
    
#endif

/*============================================================================*/
/*                          INTERNAL DATA STRUCTURES                           */
/*============================================================================*/

/**
 * @brief Assertion log storage
 * 
 * Circular buffer of recent assertion failures. Useful for
 * post-mortem analysis and identifying recurring issues.
 */
typedef struct {
    AssertRecord_t      records[APP_ASSERT_LOG_SIZE];
    uint16_t            head;
    uint16_t            count;
} AssertLog_t;

/*============================================================================*/
/*                          MODULE-LOCAL DATA                                  */
/*============================================================================*/

/** @brief Assertion log buffer */
static AssertLog_t s_assert_log = {0};

/** @brief Assertion statistics */
static AssertStats_t s_assert_stats = {0};

/** @brief Custom assertion handler (may be NULL) */
static AssertHandler_t s_custom_handler = NULL;

/** @brief Initialization flag */
static bool s_initialized = false;

/*============================================================================*/
/*                          PRIVATE HELPER FUNCTIONS                           */
/*============================================================================*/

/**
 * @brief Add assertion to log
 * 
 * Stores the assertion in the circular buffer and updates statistics.
 * 
 * @param record Assertion record to store
 */
static void log_assertion(const AssertRecord_t *record)
{
    /* Check for duplicate (same location) */
    for (uint32_t i = 0; i < s_assert_log.count && i < APP_ASSERT_LOG_SIZE; i++) {
        uint32_t idx = (s_assert_log.head - 1 - i + APP_ASSERT_LOG_SIZE) % APP_ASSERT_LOG_SIZE;
        
        if ((s_assert_log.records[idx].file != NULL) && 
            (s_assert_log.records[idx].file == record->file) &&
            (s_assert_log.records[idx].line == record->line)) {
            /* Same location - just increment count */
            s_assert_log.records[idx].count++;
            s_assert_log.records[idx].timestamp = record->timestamp;
            s_assert_stats.total_count++;
            return;
        }
    }
    
    /* New assertion - add to log */
    s_assert_log.records[s_assert_log.head] = *record;
    s_assert_log.records[s_assert_log.head].count = 1;
    
    s_assert_log.head = (s_assert_log.head + 1U) % APP_ASSERT_LOG_SIZE;
    
    if (s_assert_log.count < APP_ASSERT_LOG_SIZE) {
        s_assert_log.count++;
    }
    
    s_assert_stats.unique_count++;
    s_assert_stats.total_count++;
}

/**
 * @brief Execute default assertion action
 * 
 * Called when no handler is registered or handler returns false.
 * Action depends on APP_ASSERT_ACTION configuration.
 */
static void execute_default_action(void)
{
    /*
     * Default action is typically to reset the system.
     * Before reset, we ensure:
     * 1. Log is persisted (if using NVRAM)
     * 2. Any pending UART output is flushed
     * 3. Hardware is in safe state
     */
    
#if (APP_ASSERT_ACTION == 0)
    /* No action - just return (DANGEROUS) */
    
#elif (APP_ASSERT_ACTION == 1)
    /* Hang in infinite loop */
    DISABLE_INTERRUPTS();
    while (1) {
        /* Wait for watchdog to reset, or for debugger */
        DEBUG_BREAK();
    }
    
#elif (APP_ASSERT_ACTION == 2)
    /* System reset */
    SYSTEM_RESET();
    
#elif (APP_ASSERT_ACTION == 3)
    /* Custom handler should have been called */
    /* Fall through to reset */
    SYSTEM_RESET();
    
#endif
}

/**
 * @brief Output assertion information
 * 
 * Sends assertion details to the configured output channel.
 * This could be UART, RTT, semihosting, or a log buffer.
 * 
 * @param record Assertion record to output
 */
static void output_assertion(const AssertRecord_t *record)
{
    /*
     * Platform-specific output implementation.
     * 
     * Options:
     * - UART printf (blocking)
     * - Segger RTT (non-blocking, requires debugger)
     * - ITM stimulus ports (requires SWO)
     * - Memory buffer for debugger inspection
     * 
     * This example uses a simple approach suitable for most systems.
     */
    
#ifdef APP_ASSERT_OUTPUT_FUNC
    /* If platform provides output function, use it */
    extern void APP_ASSERT_OUTPUT_FUNC(const char *fmt, ...);
    
    APP_ASSERT_OUTPUT_FUNC("\n=== ASSERTION FAILED ===\n");
    
    if (record->condition != NULL) {
        APP_ASSERT_OUTPUT_FUNC("Condition: %s\n", record->condition);
    }
    if (record->file != NULL) {
        APP_ASSERT_OUTPUT_FUNC("File: %s:%lu\n", record->file, record->line);
    }
    if (record->function != NULL) {
        APP_ASSERT_OUTPUT_FUNC("Function: %s\n", record->function);
    }
    
    APP_ASSERT_OUTPUT_FUNC("PC: 0x%08lX  LR: 0x%08lX\n", 
                           record->program_counter, 
                           record->link_register);
    APP_ASSERT_OUTPUT_FUNC("Timestamp: %lu\n", record->timestamp);
    APP_ASSERT_OUTPUT_FUNC("========================\n");
    
#else
    /* No output - just ensure compiler doesn't optimize away */
    (void)record;
#endif
}

/*============================================================================*/
/*                          PUBLIC API IMPLEMENTATION                          */
/*============================================================================*/

/**
 * @brief Initialize assertion system
 */
void App_Assert_Init(void)
{
    if (s_initialized) {
        return;
    }
    
    memset(&s_assert_log, 0, sizeof(s_assert_log));
    memset(&s_assert_stats, 0, sizeof(s_assert_stats));
    s_custom_handler = NULL;
    
    s_initialized = true;
}

/**
 * @brief Set custom assertion handler
 */
void App_Assert_SetHandler(AssertHandler_t handler)
{
    s_custom_handler = handler;
}

/**
 * @brief Get assertion statistics
 */
void App_Assert_GetStats(AssertStats_t *stats)
{
    if (stats != NULL) {
        *stats = s_assert_stats;
    }
}

/**
 * @brief Get assertion log entry
 */
bool App_Assert_GetLogEntry(uint32_t index, AssertRecord_t *record)
{
    if ((record == NULL) || (index >= s_assert_log.count)) {
        return false;
    }
    
    uint32_t actual = (s_assert_log.head - 1 - index + APP_ASSERT_LOG_SIZE) % APP_ASSERT_LOG_SIZE;
    *record = s_assert_log.records[actual];
    
    return true;
}

/**
 * @brief Get assertion log count
 */
uint32_t App_Assert_GetLogCount(void)
{
    return s_assert_log.count;
}

/**
 * @brief Clear assertion log
 */
void App_Assert_ClearLog(void)
{
    s_assert_log.head = 0;
    s_assert_log.count = 0;
    /* Don't reset stats - they're useful across clears */
}

/**
 * @brief Internal assertion failure handler
 * 
 * This function is called by the APP_ASSERT macros. It:
 * 1. Captures processor state
 * 2. Logs the assertion
 * 3. Outputs diagnostic information
 * 4. Calls custom handler if registered
 * 5. Executes default action
 */
void App_Assert_Fail(const char *condition, 
                     const char *file, 
                     uint32_t line, 
                     const char *function)
{
    AssertRecord_t record = {0};
    
    /* Disable interrupts to prevent recursion */
    DISABLE_INTERRUPTS();
    
    /* Capture context */
    record.condition = condition;
    record.file = file;
    record.line = line;
    record.function = function;
    record.timestamp = GET_TIMESTAMP();
    
#if (APP_ASSERT_ENABLE_PC_LR == 1)
    CAPTURE_PC(record.program_counter);
    CAPTURE_LR(record.link_register);
#else
    record.program_counter = 0;
    record.link_register = 0;
#endif
    
    /* Update last timestamp in stats */
    s_assert_stats.last_timestamp = record.timestamp;
    
    /* Log the assertion */
    log_assertion(&record);
    
    /* Output assertion details */
    output_assertion(&record);
    
    /* Break into debugger if attached (debug builds) */
#if (APP_ASSERT_DEBUG_BREAK == 1)
    DEBUG_BREAK();
#endif
    
    /* Call custom handler if registered */
    bool handled = false;
    if (s_custom_handler != NULL) {
        handled = s_custom_handler(&record);
    }
    
    if (handled) {
        s_assert_stats.ignored_count++;
        /* Handler said to continue - re-enable interrupts */
        ENABLE_INTERRUPTS();
        return;
    }
    
    /* Execute default action (typically reset) */
    execute_default_action();
    
    /* Should never reach here, but just in case... */
    while (1) {
        /* Infinite loop as safety net */
    }
}

/**
 * @brief Internal assertion failure handler with message
 */
void App_Assert_FailMsg(const char *condition, 
                        const char *file, 
                        uint32_t line, 
                        const char *function,
                        const char *message)
{
    /*
     * For now, we just call the standard fail handler.
     * The message parameter is available for future enhancement
     * (e.g., storing in extended record or outputting separately).
     */
    
    (void)message;  /* Suppress unused parameter warning */
    
    /* 
     * In a more sophisticated implementation, we could:
     * 1. Store message in extended record structure
     * 2. Output message to diagnostic channel
     * 3. Include message in any NVRAM logging
     */
    
    App_Assert_Fail(condition, file, line, function);
}

/*============================================================================*/
/*                          COMPILER-SPECIFIC HANDLING                         */
/*============================================================================*/

/*
 * Some compilers have special assertion built-ins. We can optionally
 * use these for better integration with IDEs and static analyzers.
 */

#if defined(__GNUC__) && !defined(__clang__)
    /* GCC-specific: Mark this function as no-return */
    __attribute__((noreturn))
#endif
void App_Assert_Halt(void)
{
    DISABLE_INTERRUPTS();
    while (1) {
        DEBUG_BREAK();
    }
}

/*============================================================================*/
/*                          USAGE EXAMPLES                                     */
/*============================================================================*/

/*
 * The following are examples of how to use the assertion system.
 * These are commented out but show typical usage patterns.
 */

#if 0

/* Example 1: Basic pointer validation */
void process_data(uint8_t *data, size_t length)
{
    APP_ASSERT_NOT_NULL(data);
    APP_ASSERT(length > 0);
    APP_ASSERT(length <= MAX_DATA_LENGTH);
    
    /* Process data... */
}

/* Example 2: State machine validation */
void state_machine_event(StateMachine_t *sm, Event_t event)
{
    APP_ASSERT_NOT_NULL(sm);
    APP_ASSERT_RANGE(sm->state, STATE_MIN, STATE_MAX);
    
    switch (sm->state) {
        case STATE_IDLE:
            /* Handle idle state */
            break;
        case STATE_RUNNING:
            /* Handle running state */
            break;
        default:
            APP_ASSERT_FAIL("Invalid state in state machine");
    }
}

/* Example 3: Critical section validation */
void modify_shared_data(SharedData_t *data)
{
    /* Ensure we're in a critical section */
    APP_ASSERT_INTERRUPTS_DISABLED();
    
    data->counter++;
    data->last_update = get_timestamp();
}

/* Example 4: Compile-time size verification */
typedef struct {
    uint32_t header;
    uint8_t  data[28];
    uint32_t checksum;
} Packet_t;

APP_STATIC_ASSERT(sizeof(Packet_t) == 36, packet_must_be_36_bytes);

/* Example 5: Debug-only expensive check */
void sort_array(int *arr, size_t n)
{
    APP_ASSERT_NOT_NULL(arr);
    
    /* Expensive verification only in debug builds */
    APP_DEBUG_ASSERT(is_sorted(arr, n) == false);
    
    /* Sort implementation... */
}

/* Example 6: Custom handler for production recovery */
bool my_assert_handler(const AssertRecord_t *record)
{
    log_to_nvram(record);
    
    if (record->line == KNOWN_BENIGN_ASSERT_LINE) {
        return true;  /* Continue execution */
    }
    
    return false;  /* Halt/reset */
}

void setup_assertions(void)
{
    App_Assert_Init();
    App_Assert_SetHandler(my_assert_handler);
}

#endif
