/**
 * @file error_handler.c
 * @brief Error Handling System Implementation
 * 
 * This file implements the centralized error handling system for embedded
 * applications. It provides:
 * - Error reporting with automatic context capture
 * - Error logging to circular buffer
 * - Callback-based error notification
 * - Recovery action dispatching
 * - Error statistics tracking
 * 
 * Thread Safety Considerations:
 * - Error_Report() is interrupt-safe for basic logging
 * - Callback invocation may be deferred to main context
 * - Statistics updates use atomic operations where available
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 */

#include "error_codes.h"
#include <string.h>

/*============================================================================*/
/*                          PLATFORM ADAPTATION LAYER                          */
/*============================================================================*/

/**
 * @brief Platform-specific includes and macros
 * 
 * Adapt these for your target platform. The examples shown are
 * for ARM Cortex-M processors using CMSIS.
 */
#ifdef __ARM_ARCH
    #include "cmsis_compiler.h"
    #include "core_cm4.h"  /* Adjust for your Cortex variant */
    
    /* Get current timestamp from system tick */
    #define GET_TIMESTAMP()         (SysTick->VAL)  /* Or use RTOS tick */
    
    /* Capture program counter - ARM Cortex-M specific */
    #define CAPTURE_PC(pc)          __asm volatile("mov %0, pc" : "=r"(pc))
    
    /* Capture link register */
    #define CAPTURE_LR(lr)          __asm volatile("mov %0, lr" : "=r"(lr))
    
    /* Memory barrier for multi-core systems */
    #define MEMORY_BARRIER()        __DMB()
    
#else
    /* Fallback definitions for simulation/testing */
    #include <time.h>
    
    #define GET_TIMESTAMP()         ((uint32_t)clock())
    #define CAPTURE_PC(pc)          ((void)((pc) = 0))
    #define CAPTURE_LR(lr)          ((void)((lr) = 0))
    #define MEMORY_BARRIER()        ((void)0)
    
#endif

/*============================================================================*/
/*                          ERROR LOG CONFIGURATION                            */
/*============================================================================*/

/**
 * @brief Size of the circular error log buffer
 * 
 * Each entry is sizeof(ErrorContext_t) bytes. Adjust based on
 * available RAM and debugging requirements.
 */
#define ERROR_LOG_SIZE              (16U)

/**
 * @brief Enable verbose logging (increases code size)
 */
#define ERROR_VERBOSE_LOGGING       (1)

/*============================================================================*/
/*                          INTERNAL DATA STRUCTURES                           */
/*============================================================================*/

/**
 * @brief Registered callback entry
 */
typedef struct {
    ErrorCallback_t     callback;       /**< Callback function pointer */
    void               *user_data;      /**< User-provided context */
    bool                active;         /**< Entry is in use */
} ErrorCallbackEntry_t;

/**
 * @brief Registered recovery action entry
 */
typedef struct {
    ErrorCode_t         error_mask;     /**< Error code or mask to match */
    ErrorRecovery_t     recovery;       /**< Recovery function */
    bool                active;         /**< Entry is in use */
} ErrorRecoveryEntry_t;

/**
 * @brief Circular error log buffer
 */
typedef struct {
    ErrorContext_t      entries[ERROR_LOG_SIZE];   /**< Log entries */
    uint16_t            head;                       /**< Write index */
    uint16_t            count;                      /**< Valid entries */
} ErrorLog_t;

/*============================================================================*/
/*                          MODULE-LOCAL DATA                                  */
/*============================================================================*/

/**
 * @brief Registered error callbacks
 * 
 * Stored in RAM for runtime modification. In some systems, you
 * might place callback tables in a dedicated memory section.
 */
static ErrorCallbackEntry_t s_callbacks[ERROR_MAX_CALLBACKS];

/**
 * @brief Registered recovery actions
 * 
 * Limited to ERROR_MAX_CALLBACKS entries to conserve RAM.
 * Increase if your system needs more recovery handlers.
 */
static ErrorRecoveryEntry_t s_recoveries[ERROR_MAX_CALLBACKS];

/**
 * @brief Circular error log
 * 
 * Stores the most recent errors for debugging. On systems with
 * non-volatile memory, consider persisting this across resets.
 */
static ErrorLog_t s_error_log;

/**
 * @brief Global error statistics
 */
static ErrorStats_t s_error_stats;

/**
 * @brief Initialization flag
 */
static bool s_initialized = false;

/*============================================================================*/
/*                          ERROR NAME TABLES                                  */
/*============================================================================*/

/**
 * @brief Error code name lookup entry
 */
typedef struct {
    ErrorCode_t         code;
    const char         *name;
    const char         *description;
} ErrorNameEntry_t;

/**
 * @brief Error name lookup table
 * 
 * This table provides human-readable names and descriptions for
 * system error codes. Module-specific errors should extend this
 * table or provide their own lookup function.
 * 
 * For production systems, consider placing this in ROM/Flash
 * using appropriate linker sections (e.g., const or PROGMEM).
 */
static const ErrorNameEntry_t s_error_names[] = {
    /* Success */
    { ERROR_NONE,               "SUCCESS",          "Operation completed successfully" },
    
    /* System errors */
    { ERROR_INVALID_PARAMETER,  "INVALID_PARAM",    "Invalid parameter passed to function" },
    { ERROR_NULL_POINTER,       "NULL_POINTER",     "NULL pointer dereference prevented" },
    { ERROR_BUFFER_OVERFLOW,    "BUF_OVERFLOW",     "Buffer overflow detected" },
    { ERROR_BUFFER_UNDERFLOW,   "BUF_UNDERFLOW",    "Buffer underflow detected" },
    { ERROR_TIMEOUT,            "TIMEOUT",          "Operation timed out" },
    { ERROR_NOT_INITIALIZED,    "NOT_INIT",         "Module not initialized" },
    { ERROR_ALREADY_INITIALIZED,"ALREADY_INIT",     "Module already initialized" },
    { ERROR_INVALID_STATE,      "INVALID_STATE",    "Invalid state for operation" },
    { ERROR_NOT_SUPPORTED,      "NOT_SUPPORTED",    "Operation not supported" },
    { ERROR_BUSY,               "BUSY",             "Resource busy" },
    
    /* Memory errors */
    { ERROR_OUT_OF_MEMORY,      "OUT_OF_MEMORY",    "Memory allocation failed" },
    { ERROR_MEMORY_CORRUPTION,  "MEM_CORRUPT",      "Memory corruption detected" },
    { ERROR_STACK_OVERFLOW,     "STACK_OVERFLOW",   "Stack overflow detected" },
    
    /* Driver errors */
    { ERROR_HARDWARE_FAULT,     "HW_FAULT",         "Hardware fault detected" },
    { ERROR_DEVICE_NOT_FOUND,   "DEV_NOT_FOUND",    "Device not found" },
    { ERROR_DEVICE_BUSY,        "DEV_BUSY",         "Device busy" },
    { ERROR_DMA_ERROR,          "DMA_ERROR",        "DMA transfer error" },
    { ERROR_INTERRUPT_ERROR,    "INT_ERROR",        "Interrupt handling error" },
    
    /* Communication errors */
    { ERROR_CRC_MISMATCH,       "CRC_MISMATCH",     "CRC check failed" },
    { ERROR_FRAMING_ERROR,      "FRAMING_ERROR",    "Communication framing error" },
    { ERROR_PROTOCOL_ERROR,     "PROTO_ERROR",      "Protocol violation" },
    { ERROR_CONNECTION_LOST,    "CONN_LOST",        "Connection lost" },
    
    /* Critical errors */
    { ERROR_ASSERTION_FAILED,   "ASSERT_FAILED",    "Assertion failure" },
    { ERROR_WATCHDOG_TIMEOUT,   "WDT_TIMEOUT",      "Watchdog timeout" },
    { ERROR_UNHANDLED_EXCEPTION,"UNHANDLED_EXC",    "Unhandled exception" },
};

#define ERROR_NAME_TABLE_SIZE   (sizeof(s_error_names) / sizeof(s_error_names[0]))

/*============================================================================*/
/*                          PRIVATE HELPER FUNCTIONS                           */
/*============================================================================*/

/**
 * @brief Capture call stack for error context
 * 
 * This function walks the stack to capture return addresses. The
 * implementation is highly platform-specific. The ARM Cortex-M
 * version uses the frame pointer chain.
 * 
 * @param context Context structure to populate
 * @param skip_frames Number of frames to skip (don't capture)
 */
static void capture_call_stack(ErrorContext_t *context, uint32_t skip_frames)
{
    /*
     * Stack walking is architecture-specific. This implementation
     * provides a framework - adapt for your target processor.
     * 
     * ARM Cortex-M Stack Frame:
     * - The stack grows downward
     * - Each frame contains saved registers
     * - LR (R14) in each frame is the return address
     * 
     * For safety, we validate each pointer before dereferencing.
     */
    
#if defined(__ARM_ARCH) && !defined(__CC_ARM)
    /* ARM Cortex-M stack walking implementation */
    uint32_t *frame_ptr;
    uint32_t depth = 0;
    
    /* Get current frame pointer */
    __asm volatile("mov %0, fp" : "=r"(frame_ptr));
    
    /* Walk the frame chain */
    while ((frame_ptr != NULL) && 
           (depth < ERROR_MAX_STACK_DEPTH) &&
           (((uint32_t)frame_ptr & 0x3) == 0)) {  /* Aligned check */
        
        /* Skip requested frames */
        if (skip_frames > 0) {
            skip_frames--;
            frame_ptr = (uint32_t *)frame_ptr[0];  /* Previous frame */
            continue;
        }
        
        /* Capture return address from previous frame */
        context->call_stack[depth] = frame_ptr[1];  /* Saved LR */
        depth++;
        
        /* Move to previous frame */
        frame_ptr = (uint32_t *)frame_ptr[0];
    }
    
    context->stack_depth = (uint8_t)depth;
    
#else
    /* Generic implementation - just zero the stack */
    (void)skip_frames;
    memset(context->call_stack, 0, sizeof(context->call_stack));
    context->stack_depth = 0;
#endif
}

/**
 * @brief Update error statistics
 * 
 * Called for each error to maintain running statistics. These
 * stats can be used for system health monitoring.
 * 
 * @param code The error code being reported
 */
static void update_statistics(ErrorCode_t code)
{
    ErrorCategory_t category = Error_GetCategory(code);
    
    s_error_stats.total_errors++;
    s_error_stats.last_error_timestamp = GET_TIMESTAMP();
    s_error_stats.last_error_code = code;
    
    /* Update category count */
    if (category < ERROR_CATEGORY_INFO) {
        /* Category 0 - not really an error, but track it */
    } else if (category < ERROR_CATEGORY_WARNING) {
        s_error_stats.errors_by_category[1]++;
    } else if (category < ERROR_CATEGORY_ERROR) {
        s_error_stats.errors_by_category[2]++;
    } else if (category < ERROR_CATEGORY_CRITICAL) {
        s_error_stats.errors_by_category[3]++;
    } else if (category < ERROR_CATEGORY_FATAL) {
        s_error_stats.errors_by_category[4]++;
    } else {
        s_error_stats.errors_by_category[5]++;
    }
    
    MEMORY_BARRIER();
}

/**
 * @brief Add error to circular log
 * 
 * @param context Error context to log
 */
static void log_error(const ErrorContext_t *context)
{
    /* Store in circular buffer */
    s_error_log.entries[s_error_log.head] = *context;
    
    /* Advance head with wrap */
    s_error_log.head = (s_error_log.head + 1U) % ERROR_LOG_SIZE;
    
    /* Update count (cap at buffer size) */
    if (s_error_log.count < ERROR_LOG_SIZE) {
        s_error_log.count++;
    }
}

/**
 * @brief Attempt recovery for an error
 * 
 * Searches registered recovery actions and executes matching handlers.
 * 
 * @param code Error code to recover from
 * @param context Error context
 * @return true if recovery was successful
 */
static bool attempt_recovery(ErrorCode_t code, const ErrorContext_t *context)
{
    s_error_stats.recovery_attempts++;
    
    for (uint32_t i = 0; i < ERROR_MAX_CALLBACKS; i++) {
        if (!s_recoveries[i].active) {
            continue;
        }
        
        /* Check for exact match or category match */
        ErrorCode_t mask = s_recoveries[i].error_mask;
        ErrorCode_t match_code = code & mask;
        
        if ((match_code == mask) || 
            (Error_GetCategory(code) == Error_GetCategory(mask))) {
            
            if (s_recoveries[i].recovery(code, context)) {
                s_error_stats.recovery_successes++;
                return true;
            }
        }
    }
    
    return false;
}

/*============================================================================*/
/*                          PUBLIC API IMPLEMENTATION                          */
/*============================================================================*/

/**
 * @brief Initialize the error handling system
 * 
 * Must be called before any other error handling functions.
 * Safe to call multiple times.
 */
void Error_Init(void)
{
    if (s_initialized) {
        return;
    }
    
    /* Clear callback table */
    memset(s_callbacks, 0, sizeof(s_callbacks));
    
    /* Clear recovery table */
    memset(s_recoveries, 0, sizeof(s_recoveries));
    
    /* Clear error log */
    memset(&s_error_log, 0, sizeof(s_error_log));
    
    /* Clear statistics */
    memset(&s_error_stats, 0, sizeof(s_error_stats));
    
    s_initialized = true;
}

/**
 * @brief Report an error with full context
 * 
 * This is the primary error reporting function. It captures context,
 * logs the error, invokes callbacks, and attempts recovery.
 * 
 * @param code Error code to report
 * @param file Source file name (use __FILE__)
 * @param line Source line number (use __LINE__)
 * @param function Function name (use __func__)
 * @param message Optional error message (may be NULL)
 * @return ErrorCode_t The reported error code (for chaining)
 */
ErrorCode_t Error_ReportEx(ErrorCode_t code, 
                           const char *file, 
                           uint32_t line,
                           const char *function,
                           const char *message)
{
    ErrorContext_t context = {0};
    
    /* Basic context */
    context.code = code;
    context.timestamp = GET_TIMESTAMP();
    context.line = line;
    context.file = file;
    context.function = function;
    
    /* Capture processor state */
    CAPTURE_PC(context.program_counter);
    CAPTURE_LR(context.link_register);
    
    /* Capture call stack */
    capture_call_stack(&context, 1);  /* Skip this function's frame */
    
    /* Copy message if provided */
    if (message != NULL) {
        strncpy(context.message, message, ERROR_MAX_MESSAGE_LENGTH - 1);
        context.message[ERROR_MAX_MESSAGE_LENGTH - 1] = '\0';
    }
    
    /* Update statistics */
    update_statistics(code);
    
    /* Log the error */
    log_error(&context);
    
    /* Invoke callbacks in registration order */
    for (uint32_t i = 0; i < ERROR_MAX_CALLBACKS; i++) {
        if (s_callbacks[i].active && s_callbacks[i].callback != NULL) {
            /* If callback returns true, error was handled */
            if (s_callbacks[i].callback(&context, s_callbacks[i].user_data)) {
                break;  /* Stop propagation */
            }
        }
    }
    
    /* Attempt automatic recovery for critical errors */
    if (Error_IsCritical(code)) {
        attempt_recovery(code, &context);
    }
    
    return code;
}

/**
 * @brief Report an error with minimal context
 * 
 * Simplified version for cases where file/line information
 * is not available or not needed.
 * 
 * @param code Error code to report
 * @return ErrorCode_t The reported error code
 */
ErrorCode_t Error_Report(ErrorCode_t code)
{
    return Error_ReportEx(code, NULL, 0, NULL, NULL);
}

/**
 * @brief Register an error callback
 */
bool Error_RegisterCallback(ErrorCallback_t callback, void *user_data)
{
    if (callback == NULL) {
        return false;
    }
    
    for (uint32_t i = 0; i < ERROR_MAX_CALLBACKS; i++) {
        if (!s_callbacks[i].active) {
            s_callbacks[i].callback = callback;
            s_callbacks[i].user_data = user_data;
            s_callbacks[i].active = true;
            return true;
        }
    }
    
    return false;  /* Table full */
}

/**
 * @brief Unregister an error callback
 */
bool Error_UnregisterCallback(ErrorCallback_t callback)
{
    for (uint32_t i = 0; i < ERROR_MAX_CALLBACKS; i++) {
        if (s_callbacks[i].callback == callback) {
            s_callbacks[i].active = false;
            s_callbacks[i].callback = NULL;
            s_callbacks[i].user_data = NULL;
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Register a recovery action
 */
bool Error_RegisterRecovery(ErrorCode_t code, ErrorRecovery_t recovery)
{
    if (recovery == NULL) {
        return false;
    }
    
    for (uint32_t i = 0; i < ERROR_MAX_CALLBACKS; i++) {
        if (!s_recoveries[i].active) {
            s_recoveries[i].error_mask = code;
            s_recoveries[i].recovery = recovery;
            s_recoveries[i].active = true;
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Get human-readable name for error code
 */
const char *Error_GetName(ErrorCode_t code)
{
    for (uint32_t i = 0; i < ERROR_NAME_TABLE_SIZE; i++) {
        if (s_error_names[i].code == code) {
            return s_error_names[i].name;
        }
    }
    
    return "UNKNOWN";
}

/**
 * @brief Get human-readable description for error code
 */
const char *Error_GetDescription(ErrorCode_t code)
{
    for (uint32_t i = 0; i < ERROR_NAME_TABLE_SIZE; i++) {
        if (s_error_names[i].code == code) {
            return s_error_names[i].description;
        }
    }
    
    return "";
}

/**
 * @brief Get current error statistics
 */
void Error_GetStats(ErrorStats_t *stats)
{
    if (stats != NULL) {
        *stats = s_error_stats;
    }
}

/**
 * @brief Reset error statistics
 */
void Error_ResetStats(void)
{
    memset(&s_error_stats, 0, sizeof(s_error_stats));
}

/**
 * @brief Get error from log by index
 * 
 * @param index Index into log (0 = most recent)
 * @param context Pointer to receive error context
 * @return true if entry found, false if index out of range
 */
bool Error_GetLogEntry(uint32_t index, ErrorContext_t *context)
{
    if (context == NULL || index >= s_error_log.count) {
        return false;
    }
    
    /* Calculate actual index (log is circular, most recent first) */
    uint32_t actual_index;
    if (s_error_log.count < ERROR_LOG_SIZE) {
        actual_index = (s_error_log.head - 1 - index + ERROR_LOG_SIZE) % ERROR_LOG_SIZE;
    } else {
        actual_index = (s_error_log.head - 1 - index + ERROR_LOG_SIZE) % ERROR_LOG_SIZE;
    }
    
    *context = s_error_log.entries[actual_index];
    return true;
}

/**
 * @brief Get number of errors in log
 */
uint32_t Error_GetLogCount(void)
{
    return s_error_log.count;
}

/**
 * @brief Clear the error log
 */
void Error_ClearLog(void)
{
    s_error_log.head = 0;
    s_error_log.count = 0;
}
