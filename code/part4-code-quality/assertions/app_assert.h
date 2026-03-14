/**
 * @file app_assert.h
 * @brief Embedded Assertion System for Production and Debug
 * 
 * This module provides a comprehensive assertion system designed specifically
 * for embedded systems. Unlike standard assert.h, this system provides:
 * 
 * - Compile-time assertions (static_assert equivalent)
 * - Runtime assertions with configurable behavior
 * - PC/LR capture for post-mortem analysis
 * - Graceful degradation options for production
 * - Assertion statistics and logging
 * 
 * Design Philosophy:
 * - Assertions are for programmer errors, not runtime errors
 * - Assertions should never be disabled in production (unlike NDEBUG)
 * - Assertion failures should provide maximum debugging information
 * - System should attempt to recover or fail safely
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#ifndef APP_ASSERT_H
#define APP_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "error_codes.h"

/*============================================================================*/
/*                          CONFIGURATION OPTIONS                              */
/*============================================================================*/

/**
 * @brief Enable/disable assertion message strings
 * 
 * When disabled, assertions still trigger but don't store the
 * condition string. Saves ROM at the cost of debugging info.
 */
#ifndef APP_ASSERT_ENABLE_MESSAGES
#define APP_ASSERT_ENABLE_MESSAGES      (1)
#endif

/**
 * @brief Enable/disable file name in assertions
 * 
 * File names can be lengthy. Disable to save ROM in production.
 */
#ifndef APP_ASSERT_ENABLE_FILE
#define APP_ASSERT_ENABLE_FILE          (1)
#endif

/**
 * @brief Enable/disable function name in assertions
 * 
 * Function names are usually shorter than file names.
 */
#ifndef APP_ASSERT_ENABLE_FUNCTION
#define APP_ASSERT_ENABLE_FUNCTION      (1)
#endif

/**
 * @brief Enable/disable PC/LR capture
 * 
 * Captures program counter and link register at assertion point.
 * Very useful for post-mortem analysis but adds small overhead.
 */
#ifndef APP_ASSERT_ENABLE_PC_LR
#define APP_ASSERT_ENABLE_PC_LR         (1)
#endif

/**
 * @brief Assertion action on failure
 * 
 * 0 = No action (just log) - NOT RECOMMENDED
 * 1 = Hang in infinite loop
 * 2 = Trigger software reset
 * 3 = Call custom handler
 */
#ifndef APP_ASSERT_ACTION
#define APP_ASSERT_ACTION               (2)
#endif

/**
 * @brief Maximum assertions to log
 * 
 * Circular buffer size for assertion history. Useful for
 * analyzing assertion patterns without consuming too much RAM.
 */
#ifndef APP_ASSERT_LOG_SIZE
#define APP_ASSERT_LOG_SIZE             (8U)
#endif

/**
 * @brief Break into debugger on assertion (debug builds)
 * 
 * When using a debugger, this causes a breakpoint at the assertion.
 * Has no effect when no debugger is attached.
 */
#ifndef APP_ASSERT_DEBUG_BREAK
#ifdef DEBUG
#define APP_ASSERT_DEBUG_BREAK          (1)
#else
#define APP_ASSERT_DEBUG_BREAK          (0)
#endif
#endif

/*============================================================================*/
/*                          COMPILE-TIME ASSERTIONS                            */
/*============================================================================*/

/**
 * @defgroup CompileTimeAsserts Compile-Time Assertions
 * @brief Assertions evaluated at compile time
 * @{
 */

/**
 * @brief Compile-time assertion macro
 * 
 * Causes a compile error if condition is false. The error message
 * includes the condition text for easy identification.
 * 
 * Usage:
 *   APP_STATIC_ASSERT(sizeof(my_struct) == 32, my_struct_must_be_32_bytes);
 * 
 * @param condition Expression that must be true
 * @param name Unique identifier for this assertion
 */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    /* C11 has _Static_assert built-in */
    #define APP_STATIC_ASSERT(condition, name) \
        _Static_assert((condition), "Static assertion failed: " #name)
#else
    /* Pre-C11: Use negative array size trick */
    #define APP_STATIC_ASSERT(condition, name) \
        typedef char static_assert_##name[(condition) ? 1 : -1]
#endif

/**
 * @brief Assert that two types have the same size
 * 
 * Useful for ensuring structure layouts match hardware registers
 * or protocol specifications.
 * 
 * @param type1 First type
 * @param type2 Second type
 * @param name Unique identifier
 */
#define APP_STATIC_ASSERT_SIZE(type1, type2, name) \
    APP_STATIC_ASSERT(sizeof(type1) == sizeof(type2), name)

/**
 * @brief Assert that a type has a specific size
 * 
 * @param type The type to check
 * @param expected_size Expected size in bytes
 * @param name Unique identifier
 */
#define APP_STATIC_ASSERT_TYPE_SIZE(type, expected_size, name) \
    APP_STATIC_ASSERT(sizeof(type) == (expected_size), name)

/**
 * @brief Assert that a type is properly aligned
 * 
 * @param type The type to check
 * @param alignment Required alignment in bytes
 * @param name Unique identifier
 */
#define APP_STATIC_ASSERT_ALIGNMENT(type, alignment, name) \
    APP_STATIC_ASSERT(((alignment) & ((alignment) - 1)) == 0, name##_must_be_power_of_2); \
    APP_STATIC_ASSERT(__alignof__(type) >= (alignment), name)

/** @} */

/*============================================================================*/
/*                          RUNTIME ASSERTION STRUCTURES                       */
/*============================================================================*/

/**
 * @brief Assertion failure record
 * 
 * Captures complete information about an assertion failure for
 * debugging and logging purposes.
 */
typedef struct {
    const char         *condition;      /**< The failed condition string */
    const char         *file;           /**< Source file name */
    const char         *function;       /**< Function name */
    uint32_t            line;           /**< Line number */
    uint32_t            program_counter;/**< PC at failure */
    uint32_t            link_register;  /**< LR at failure */
    uint32_t            timestamp;      /**< System tick at failure */
    uint32_t            count;          /**< Occurrence count */
} AssertRecord_t;

/**
 * @brief Assertion handler function type
 * 
 * Custom handler called on assertion failure. Handler can:
 * - Log the assertion
 * - Attempt recovery
 * - Trigger system reset
 * - Return to continue execution (NOT RECOMMENDED)
 * 
 * @param record Pointer to assertion record
 * @return true to continue execution, false to halt/reset
 */
typedef bool (*AssertHandler_t)(const AssertRecord_t *record);

/**
 * @brief Assertion statistics
 */
typedef struct {
    uint32_t            total_count;    /**< Total assertions triggered */
    uint32_t            unique_count;   /**< Unique assertion locations */
    uint32_t            last_timestamp; /**< Most recent assertion time */
    uint32_t            ignored_count;  /**< Assertions ignored by handler */
} AssertStats_t;

/*============================================================================*/
/*                          RUNTIME ASSERTION MACROS                           */
/*============================================================================*/

/**
 * @defgroup RuntimeAsserts Runtime Assertions
 * @brief Assertions evaluated at runtime
 * @{
 */

/**
 * @brief Standard runtime assertion
 * 
 * Evaluates condition and triggers assertion handler if false.
 * In production, this still executes - assertions are NOT disabled.
 * 
 * Usage:
 *   APP_ASSERT(ptr != NULL);
 *   APP_ASSERT(index < array_size);
 * 
 * @param condition Boolean expression to verify
 */
#if (APP_ASSERT_ENABLE_MESSAGES == 1)
    #define APP_ASSERT(condition) \
        do { \
            if (!(condition)) { \
                App_Assert_Fail(#condition, __FILE__, __LINE__, __func__); \
            } \
        } while (0)
#else
    #define APP_ASSERT(condition) \
        do { \
            if (!(condition)) { \
                App_Assert_Fail(NULL, NULL, __LINE__, NULL); \
            } \
        } while (0)
#endif

/**
 * @brief Assertion with error code
 * 
 * Combines assertion with error reporting. Useful when an assertion
 * failure should also set an error code for higher-level handling.
 * 
 * Usage:
 *   APP_ASSERT_ERROR(ptr != NULL, ERROR_NULL_POINTER);
 * 
 * @param condition Boolean expression to verify
 * @param error_code ErrorCode_t to report on failure
 */
#define APP_ASSERT_ERROR(condition, error_code) \
    do { \
        if (!(condition)) { \
            App_Assert_Fail(#condition, __FILE__, __LINE__, __func__); \
            Error_Report(error_code); \
        } \
    } while (0)

/**
 * @brief Assertion with message
 * 
 * Provides additional context about why the assertion failed.
 * 
 * Usage:
 *   APP_ASSERT_MSG(state == STATE_READY, "State machine not ready for operation");
 * 
 * @param condition Boolean expression to verify
 * @param message Descriptive message string
 */
#define APP_ASSERT_MSG(condition, message) \
    do { \
        if (!(condition)) { \
            App_Assert_FailMsg(#condition, __FILE__, __LINE__, __func__, message); \
        } \
    } while (0)

/**
 * @brief Assert that a pointer is not NULL
 * 
 * Common case optimized for clarity.
 * 
 * @param ptr Pointer to check
 */
#define APP_ASSERT_NOT_NULL(ptr) \
    APP_ASSERT((ptr) != NULL)

/**
 * @brief Assert that a value is within range
 * 
 * Verifies: min <= value < max
 * 
 * @param value Value to check
 * @param min Minimum value (inclusive)
 * @param max Maximum value (exclusive)
 */
#define APP_ASSERT_RANGE(value, min, max) \
    APP_ASSERT(((value) >= (min)) && ((value) < (max)))

/**
 * @brief Assert that a value is aligned
 * 
 * @param value Value to check
 * @param alignment Required alignment (must be power of 2)
 */
#define APP_ASSERT_ALIGNED(value, alignment) \
    APP_ASSERT(((uint32_t)(value) & ((alignment) - 1U)) == 0U)

/**
 * @brief Assert that an interrupt is disabled
 * 
 * Useful for verifying critical section entry.
 */
#if defined(__ARM_ARCH)
    #define APP_ASSERT_INTERRUPTS_DISABLED() \
        APP_ASSERT((__get_PRIMASK() & 1U) != 0U)
#else
    #define APP_ASSERT_INTERRUPTS_DISABLED() ((void)0)
#endif

/**
 * @brief Assert that an interrupt is enabled
 * 
 * Useful for verifying we're not in a critical section.
 */
#if defined(__ARM_ARCH)
    #define APP_ASSERT_INTERRUPTS_ENABLED() \
        APP_ASSERT((__get_PRIMASK() & 1U) == 0U)
#else
    #define APP_ASSERT_INTERRUPTS_ENABLED() ((void)0)
#endif

/**
 * @brief Unconditional assertion failure
 * 
 * Use for code paths that should never be reached.
 * 
 * Usage:
 *   default:
 *       APP_ASSERT_FAIL("Unhandled switch case");
 */
#define APP_ASSERT_FAIL(message) \
    App_Assert_Fail(message, __FILE__, __LINE__, __func__)

/**
 * @brief Assert that code is never reached
 * 
 * Marks code paths that should be unreachable. Helps compilers
 * optimize and catches logic errors.
 */
#define APP_ASSERT_NOT_REACHED() \
    APP_ASSERT_FAIL("Code path should never be reached")

/** @} */

/*============================================================================*/
/*                          DEBUG-ONLY ASSERTIONS                              */
/*============================================================================*/

/**
 * @defgroup DebugAsserts Debug-Only Assertions
 * @brief Assertions that are removed in release builds
 * @{
 */

/**
 * @brief Debug-only assertion
 * 
 * Only evaluates condition in debug builds. Use for expensive
 * checks or checks that are not critical in production.
 * 
 * WARNING: Do not use for safety-critical checks!
 */
#ifdef DEBUG
    #define APP_DEBUG_ASSERT(condition)  APP_ASSERT(condition)
#else
    #define APP_DEBUG_ASSERT(condition)  ((void)0)
#endif

/**
 * @brief Debug-only assertion with message
 */
#ifdef DEBUG
    #define APP_DEBUG_ASSERT_MSG(condition, message)  APP_ASSERT_MSG(condition, message)
#else
    #define APP_DEBUG_ASSERT_MSG(condition, message)  ((void)0)
#endif

/** @} */

/*============================================================================*/
/*                          PUBLIC API FUNCTIONS                               */
/*============================================================================*/

/**
 * @brief Initialize the assertion system
 * 
 * Clears assertion log and statistics. Must be called before
 * any assertions are triggered (usually in startup code).
 */
void App_Assert_Init(void);

/**
 * @brief Set custom assertion handler
 * 
 * The handler is called on assertion failure. If handler returns
 * true, execution continues. If false, system halts/resets.
 * 
 * @param handler Handler function (NULL for default behavior)
 */
void App_Assert_SetHandler(AssertHandler_t handler);

/**
 * @brief Get assertion statistics
 * 
 * @param stats Pointer to receive statistics
 */
void App_Assert_GetStats(AssertStats_t *stats);

/**
 * @brief Get assertion from log by index
 * 
 * @param index Index (0 = most recent)
 * @param record Pointer to receive assertion record
 * @return true if found, false if index out of range
 */
bool App_Assert_GetLogEntry(uint32_t index, AssertRecord_t *record);

/**
 * @brief Get number of assertions in log
 */
uint32_t App_Assert_GetLogCount(void);

/**
 * @brief Clear assertion log
 */
void App_Assert_ClearLog(void);

/*============================================================================*/
/*                          INTERNAL FUNCTIONS (Do Not Call)                   */
/*============================================================================*/

/**
 * @brief Internal assertion failure handler
 * 
 * Do not call directly - use APP_ASSERT macros.
 * 
 * @param condition Condition string (may be NULL)
 * @param file Source file (may be NULL)
 * @param line Line number
 * @param function Function name (may be NULL)
 */
void App_Assert_Fail(const char *condition, 
                     const char *file, 
                     uint32_t line, 
                     const char *function);

/**
 * @brief Internal assertion failure handler with message
 * 
 * @param condition Condition string
 * @param file Source file
 * @param line Line number
 * @param function Function name
 * @param message Additional message
 */
void App_Assert_FailMsg(const char *condition, 
                        const char *file, 
                        uint32_t line, 
                        const char *function,
                        const char *message);

#ifdef __cplusplus
}
#endif

#endif /* APP_ASSERT_H */
