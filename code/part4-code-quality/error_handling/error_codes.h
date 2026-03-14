/**
 * @file error_codes.h
 * @brief Standardized Error Handling System for Embedded C Applications
 * 
 * This module provides a comprehensive error handling framework designed for
 * resource-constrained embedded systems. It implements a hierarchical error
 * code system with context capture, error propagation, and recovery hooks.
 * 
 * Design Principles:
 * - Error codes are 32-bit values with embedded category and module information
 * - Error context captures system state at the point of failure
 * - Callback-based architecture allows flexible error handling strategies
 * - Module-specific error ranges prevent code collisions
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*============================================================================*/
/*                          CONFIGURATION CONSTANTS                           */
/*============================================================================*/

/**
 * @brief Maximum depth of the error call stack to capture
 * 
 * This defines how many return addresses we store when an error occurs.
 * Adjust based on available RAM and debugging requirements.
 */
#define ERROR_MAX_STACK_DEPTH           (8U)

/**
 * @brief Maximum number of registered error callbacks
 * 
 * Each callback can handle specific error categories or all errors.
 * Memory-constrained systems should keep this small.
 */
#define ERROR_MAX_CALLBACKS             (4U)

/**
 * @brief Maximum length for error message strings
 * 
 * Keep messages concise for embedded systems with limited logging bandwidth.
 */
#define ERROR_MAX_MESSAGE_LENGTH        (64U)

/**
 * @brief Enable/disable extended error context capture
 * 
 * When enabled, captures additional system state (registers, timestamps).
 * Disable for production builds to reduce overhead.
 */
#ifndef ERROR_EXTENDED_CONTEXT
#define ERROR_EXTENDED_CONTEXT          (1)
#endif

/*============================================================================*/
/*                          ERROR CODE ARCHITECTURE                            */
/*============================================================================*/

/**
 * @brief Error code bit field layout (32-bit total)
 * 
 * ┌─────────────┬─────────────┬─────────────────────────────┐
 * │  Category   │   Module    │      Specific Error         │
 * │   (8 bits)  │  (8 bits)   │        (16 bits)            │
 * └─────────────┴─────────────┴─────────────────────────────┘
 * 
 * This structure allows:
 * - Quick filtering by error category
 * - Module-specific error handling
 * - Up to 65536 unique errors per module
 */

/**
 * @brief Error category definitions (bits 31-24)
 * 
 * Categories group errors by severity and handling strategy.
 * Higher values indicate more severe conditions.
 */
typedef enum {
    ERROR_CATEGORY_NONE         = 0x00U,    /**< No error / success */
    ERROR_CATEGORY_INFO         = 0x01U,    /**< Informational, no action needed */
    ERROR_CATEGORY_WARNING      = 0x10U,    /**< Warning, operation may continue */
    ERROR_CATEGORY_ERROR        = 0x40U,    /**< Error, operation failed */
    ERROR_CATEGORY_CRITICAL     = 0x80U,    /**< Critical, system may be unstable */
    ERROR_CATEGORY_FATAL        = 0xC0U,    /**< Fatal, system must reset */
} ErrorCategory_t;

/**
 * @brief Module identifier definitions (bits 23-16)
 * 
 * Each module in the system gets a unique identifier. This allows
 * error handlers to route errors to appropriate recovery routines.
 * Reserve ranges for future expansion.
 */
typedef enum {
    ERROR_MODULE_SYSTEM         = 0x00U,    /**< Core system / kernel */
    ERROR_MODULE_MEMORY         = 0x01U,    /**< Memory management */
    ERROR_MODULE_DRIVER         = 0x02U,    /**< Hardware drivers */
    ERROR_MODULE_COMM           = 0x03U,    /**< Communication protocols */
    ERROR_MODULE_APPLICATION    = 0x04U,    /**< Application logic */
    ERROR_MODULE_FILESYSTEM     = 0x05U,    /**< File system operations */
    ERROR_MODULE_SECURITY       = 0x06U,    /**< Security / authentication */
    ERROR_MODULE_POWER          = 0x07U,    /**< Power management */
    
    /* Reserved ranges for application-specific modules */
    ERROR_MODULE_APP_START      = 0x10U,    /**< Application modules start */
    ERROR_MODULE_APP_END        = 0x7FU,    /**< Application modules end */
    
    /* Reserved for future expansion */
    ERROR_MODULE_RESERVED_START = 0x80U,
    ERROR_MODULE_RESERVED_END   = 0xFEU,
    
    ERROR_MODULE_UNKNOWN        = 0xFFU,    /**< Unknown / unassigned module */
} ErrorModule_t;

/*============================================================================*/
/*                          SYSTEM-WIDE ERROR DEFINITIONS                      */
/*============================================================================*/

/**
 * @brief Generic error codes applicable across all modules
 * 
 * These errors use module 0x00 (SYSTEM) and provide common failure
 * modes that any module can reference.
 */
typedef enum {
    /* Success / no error */
    ERROR_NONE                  = 0x00000000U,  /**< Operation successful */
    
    /* System-level errors (0x00 module, 0x40 category) */
    ERROR_INVALID_PARAMETER     = 0x40000001U,  /**< Invalid function parameter */
    ERROR_NULL_POINTER          = 0x40000002U,  /**< NULL pointer encountered */
    ERROR_BUFFER_OVERFLOW       = 0x40000003U,  /**< Buffer size exceeded */
    ERROR_BUFFER_UNDERFLOW      = 0x40000004U,  /**< Insufficient data available */
    ERROR_TIMEOUT               = 0x40000005U,  /**< Operation timed out */
    ERROR_NOT_INITIALIZED       = 0x40000006U,  /**< Module not initialized */
    ERROR_ALREADY_INITIALIZED   = 0x40000007U,  /**< Module already initialized */
    ERROR_INVALID_STATE         = 0x40000008U,  /**< Invalid state for operation */
    ERROR_NOT_SUPPORTED         = 0x40000009U,  /**< Operation not supported */
    ERROR_BUSY                  = 0x4000000AU,  /**< Resource busy, retry later */
    
    /* Memory errors (0x01 module, 0x40 category) */
    ERROR_OUT_OF_MEMORY         = 0x40010001U,  /**< Memory allocation failed */
    ERROR_MEMORY_CORRUPTION     = 0xC0010002U,  /**< Memory corruption detected */
    ERROR_STACK_OVERFLOW        = 0xC0010003U,  /**< Stack overflow detected */
    
    /* Driver errors (0x02 module, 0x40 category) */
    ERROR_HARDWARE_FAULT        = 0x80020001U,  /**< Hardware fault detected */
    ERROR_DEVICE_NOT_FOUND      = 0x40020002U,  /**< Device not present */
    ERROR_DEVICE_BUSY           = 0x40020003U,  /**< Device busy */
    ERROR_DMA_ERROR             = 0x80020004U,  /**< DMA transfer error */
    ERROR_INTERRUPT_ERROR       = 0x80020005U,  /**< Interrupt handling error */
    
    /* Communication errors (0x03 module, 0x40 category) */
    ERROR_CRC_MISMATCH          = 0x40030001U,  /**< CRC check failed */
    ERROR_FRAMING_ERROR         = 0x40030002U,  /**< Communication framing error */
    ERROR_PROTOCOL_ERROR        = 0x40030003U,  /**< Protocol violation */
    ERROR_CONNECTION_LOST       = 0x40030004U,  /**< Connection lost */
    
    /* Critical system errors */
    ERROR_ASSERTION_FAILED      = 0xC0000001U,  /**< Assertion failure */
    ERROR_WATCHDOG_TIMEOUT      = 0xC0000002U,  /**< Watchdog timeout occurred */
    ERROR_UNHANDLED_EXCEPTION   = 0xC0000003U,  /**< Unhandled CPU exception */
    
} ErrorCode_t;

/*============================================================================*/
/*                          ERROR CONTEXT STRUCTURE                            */
/*============================================================================*/

/**
 * @brief Error context structure
 * 
 * Captures the complete state when an error occurs, enabling
 * effective debugging and error analysis. Structure is designed
 * to be efficiently copied and stored in error logs.
 */
typedef struct {
    ErrorCode_t         code;                           /**< The error code */
    uint32_t            timestamp;                      /**< System tick at error */
    uint32_t            line;                           /**< Source line number */
    const char         *file;                           /**< Source file name */
    const char         *function;                       /**< Function name */
    
    /**
     * @brief Program counter at point of error
     * 
     * Captured automatically on ARM Cortex-M via inline assembly.
     * Useful for post-mortem analysis in production systems.
     */
    uint32_t            program_counter;
    
    /**
     * @brief Link register (return address)
     * 
     * Shows where the error-generating function was called from.
     * Critical for understanding error propagation paths.
     */
    uint32_t            link_register;
    
    /**
     * @brief Call stack capture
     * 
     * Array of return addresses leading to the error. Depth is
     * configurable via ERROR_MAX_STACK_DEPTH.
     */
    uint32_t            call_stack[ERROR_MAX_STACK_DEPTH];
    uint8_t             stack_depth;                    /**< Valid entries in stack */
    
    /**
     * @brief User-provided error context
     * 
     * Optional additional data specific to the error. Can contain
     * sensor values, buffer pointers, or other debugging info.
     */
    uint32_t            user_data_0;
    uint32_t            user_data_1;
    uint32_t            user_data_2;
    uint32_t            user_data_3;
    
#if (ERROR_EXTENDED_CONTEXT == 1)
    /**
     * @brief Extended context (optional)
     * 
     * Additional system state for detailed debugging.
     * Exclude from production builds to save memory.
     */
    uint32_t            psr;                            /**< Program status register */
    uint32_t            msp;                            /**< Main stack pointer */
    uint32_t            psp;                            /**< Process stack pointer */
    uint32_t            control;                        /**< CONTROL register */
#endif
    
    /**
     * @brief Optional error message
     * 
     * Human-readable description of the error. May be NULL
     * to save memory in resource-constrained builds.
     */
    char                message[ERROR_MAX_MESSAGE_LENGTH];
    
} ErrorContext_t;

/*============================================================================*/
/*                          ERROR CALLBACK TYPES                               */
/*============================================================================*/

/**
 * @brief Error callback function type
 * 
 * Called when an error is reported. Callbacks can:
 * - Log the error to persistent storage
 * - Trigger system recovery actions
 * - Update error statistics
 * - Notify external systems
 * 
 * @param context Pointer to the error context (read-only)
 * @param user_data Pointer registered with the callback
 * @return true if error was handled, false to continue propagation
 */
typedef bool (*ErrorCallback_t)(const ErrorContext_t *context, void *user_data);

/**
 * @brief Recovery action function type
 * 
 * Called to attempt recovery from an error. Can perform actions
 * like resetting peripherals, reloading configuration, or
 * transitioning to a safe state.
 * 
 * @param error_code The error to recover from
 * @param context Error context for recovery decisions
 * @return true if recovery successful, false otherwise
 */
typedef bool (*ErrorRecovery_t)(ErrorCode_t error_code, const ErrorContext_t *context);

/*============================================================================*/
/*                          ERROR STATISTICS                                    */
/*============================================================================*/

/**
 * @brief Error statistics structure
 * 
 * Tracks error occurrence counts and patterns for system health
 * monitoring and predictive maintenance.
 */
typedef struct {
    uint32_t            total_errors;                   /**< Total errors recorded */
    uint32_t            errors_by_category[6];          /**< Count per category */
    uint32_t            last_error_timestamp;           /**< Timestamp of last error */
    ErrorCode_t         last_error_code;                /**< Most recent error code */
    uint32_t            error_rate;                     /**< Errors per hour */
    uint32_t            recovery_attempts;              /**< Recovery attempts made */
    uint32_t            recovery_successes;             /**< Successful recoveries */
} ErrorStats_t;

/*============================================================================*/
/*                          PUBLIC API FUNCTIONS                               */
/*============================================================================*/

/**
 * @brief Extract category from error code
 * 
 * @param code Error code to analyze
 * @return ErrorCategory_t The category of the error
 */
static inline ErrorCategory_t Error_GetCategory(ErrorCode_t code) {
    return (ErrorCategory_t)((code >> 24U) & 0xFFU);
}

/**
 * @brief Extract module from error code
 * 
 * @param code Error code to analyze
 * @return ErrorModule_t The module that generated the error
 */
static inline ErrorModule_t Error_GetModule(ErrorCode_t code) {
    return (ErrorModule_t)((code >> 16U) & 0xFFU);
}

/**
 * @brief Extract specific error number from error code
 * 
 * @param code Error code to analyze
 * @return uint16_t The specific error number within the module
 */
static inline uint16_t Error_GetSpecific(ErrorCode_t code) {
    return (uint16_t)(code & 0xFFFFU);
}

/**
 * @brief Construct an error code from components
 * 
 * Use this macro to create module-specific error codes:
 *   #define MY_MODULE_ERROR_xxx MAKE_ERROR(ERROR_MODULE_APP, ERROR_CATEGORY_ERROR, 1)
 * 
 * @param category ErrorCategory_t value
 * @param module ErrorModule_t value
 * @param specific Specific error number (0-65535)
 */
#define MAKE_ERROR(category, module, specific) \
    ((ErrorCode_t)(((uint32_t)(category) << 24U) | \
                   ((uint32_t)(module) << 16U) | \
                   ((uint32_t)(specific) & 0xFFFFU)))

/**
 * @brief Check if error code indicates success
 * 
 * @param code Error code to check
 * @return true if no error, false otherwise
 */
static inline bool Error_IsSuccess(ErrorCode_t code) {
    return (code == ERROR_NONE);
}

/**
 * @brief Check if error requires immediate attention
 * 
 * @param code Error code to check
 * @return true if critical or fatal, false otherwise
 */
static inline bool Error_IsCritical(ErrorCode_t code) {
    ErrorCategory_t cat = Error_GetCategory(code);
    return (cat >= ERROR_CATEGORY_CRITICAL);
}

/**
 * @brief Register an error callback
 * 
 * Multiple callbacks can be registered and will be called in
 * registration order until one returns true (handled).
 * 
 * @param callback Function to call on error
 * @param user_data User data passed to callback
 * @return true if registered successfully, false if table full
 */
bool Error_RegisterCallback(ErrorCallback_t callback, void *user_data);

/**
 * @brief Unregister an error callback
 * 
 * @param callback Function to unregister
 * @return true if found and removed, false if not found
 */
bool Error_UnregisterCallback(ErrorCallback_t callback);

/**
 * @brief Register a recovery action for an error
 * 
 * @param code Error code to recover from (use category/module to match groups)
 * @param recovery Recovery function to call
 * @return true if registered successfully
 */
bool Error_RegisterRecovery(ErrorCode_t code, ErrorRecovery_t recovery);

/**
 * @brief Get human-readable name for error code
 * 
 * @param code Error code to translate
 * @return const char* String name of error (or "UNKNOWN")
 */
const char *Error_GetName(ErrorCode_t code);

/**
 * @brief Get human-readable description for error code
 * 
 * @param code Error code to describe
 * @return const char* Description string (or empty string)
 */
const char *Error_GetDescription(ErrorCode_t code);

/**
 * @brief Get current error statistics
 * 
 * @param stats Pointer to structure to fill
 */
void Error_GetStats(ErrorStats_t *stats);

/**
 * @brief Reset error statistics
 */
void Error_ResetStats(void);

/*============================================================================*/
/*                          MODULE ERROR RANGE MACROS                          */
/*============================================================================*/

/**
 * @brief Define a module-specific error range
 * 
 * Use these macros to define error codes for your module:
 * 
 * // In my_module_errors.h:
 * #define MY_MODULE_ERROR(code) \
 *     MODULE_ERROR(ERROR_MODULE_APP_START, ERROR_CATEGORY_ERROR, code)
 * 
 * enum {
 *     MY_ERROR_INIT_FAILED = MY_MODULE_ERROR(1),
 *     MY_ERROR_TIMEOUT     = MY_MODULE_ERROR(2),
 * };
 */
#define MODULE_ERROR(module, category, specific) \
    MAKE_ERROR((category), (module), (specific))

/**
 * @brief Convenience macro for application module errors
 */
#define APP_ERROR(module_offset, category, specific) \
    MODULE_ERROR((ERROR_MODULE_APP_START + (module_offset)), (category), (specific))

#ifdef __cplusplus
}
#endif

#endif /* ERROR_CODES_H */
