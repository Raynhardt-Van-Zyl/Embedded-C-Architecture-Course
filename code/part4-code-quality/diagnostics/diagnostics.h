/**
 * @file diagnostics.h
 * @brief Unified Diagnostics System for Embedded C Applications
 *
 * This module provides a comprehensive diagnostics framework that integrates
 * with the existing error handling and superloop systems. It offers:
 *
 * - Runtime trace/event logging with ring buffer storage
 * - System health monitoring (stack usage, heap, CPU load)
 * - Performance counters and timing measurements
 * - Structured diagnostic dump output for post-mortem analysis
 * - ISR-safe trace recording
 *
 * Design Principles:
 * - Zero dynamic allocation - all storage is statically allocated
 * - ISR-safe trace recording - can be called from interrupt context
 * - Configurable memory footprint via compile-time constants
 * - Structured output format suitable for automated parsing
 * - Integration hooks for existing error/assert/superloop systems
 *
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*============================================================================*/
/*                          CONFIGURATION                                      */
/*============================================================================*/

/**
 * @brief Maximum number of trace entries in the circular buffer
 *
 * Must be power of 2 for efficient wrap-around. Each entry is 16 bytes.
 * 128 entries = 2KB of trace storage.
 */
#ifndef DIAG_TRACE_BUFFER_SIZE
#define DIAG_TRACE_BUFFER_SIZE          (128U)
#endif

/**
 * @brief Maximum number of performance counters
 */
#ifndef DIAG_MAX_COUNTERS
#define DIAG_MAX_COUNTERS               (16U)
#endif

/**
 * @brief Maximum length of a diagnostic event message
 */
#ifndef DIAG_MAX_EVENT_MSG_LEN
#define DIAG_MAX_EVENT_MSG_LEN          (32U)
#endif

/**
 * @brief Enable stack usage monitoring
 *
 * When enabled, the diagnostics system tracks stack high-water mark
 * by filling unused stack with a pattern and checking how far it was
 * overwritten.
 */
#ifndef DIAG_ENABLE_STACK_MONITOR
#define DIAG_ENABLE_STACK_MONITOR       (1U)
#endif

/**
 * @brief Enable CPU load estimation
 *
 * Tracks the ratio of idle time to total time for CPU load percentage.
 * Requires the scheduler to report idle time.
 */
#ifndef DIAG_ENABLE_CPU_LOAD
#define DIAG_ENABLE_CPU_LOAD            (1U)
#endif

/**
 * @brief Stack watermark fill pattern
 *
 * Pattern used to fill unused stack space for high-water mark detection.
 */
#ifndef DIAG_STACK_FILL_PATTERN
#define DIAG_STACK_FILL_PATTERN         (0xA5A5A5A5U)
#endif

/* Validate trace buffer size is power of 2 */
#if (DIAG_TRACE_BUFFER_SIZE & (DIAG_TRACE_BUFFER_SIZE - 1U)) != 0U
#error "DIAG_TRACE_BUFFER_SIZE must be a power of 2"
#endif

/*============================================================================*/
/*                          TRACE EVENT TYPES                                  */
/*============================================================================*/

/**
 * @brief Trace event severity levels
 */
typedef enum {
    DIAG_SEVERITY_DEBUG     = 0x00U,    /**< Debug-level information */
    DIAG_SEVERITY_INFO      = 0x01U,    /**< Informational event */
    DIAG_SEVERITY_WARNING   = 0x02U,    /**< Warning condition */
    DIAG_SEVERITY_ERROR     = 0x03U,    /**< Error condition */
    DIAG_SEVERITY_CRITICAL  = 0x04U,    /**< Critical error */
} DiagSeverity_t;

/**
 * @brief Trace event source modules
 *
 * Maps to existing module identifiers for consistency.
 */
typedef enum {
    DIAG_MODULE_SYSTEM      = 0x00U,    /**< Core system */
    DIAG_MODULE_SCHEDULER   = 0x01U,    /**< Task scheduler */
    DIAG_MODULE_HAL         = 0x02U,    /**< Hardware abstraction */
    DIAG_MODULE_DRIVER      = 0x03U,    /**< Device drivers */
    DIAG_MODULE_COMM        = 0x04U,    /**< Communication */
    DIAG_MODULE_APP         = 0x05U,    /**< Application logic */
    DIAG_MODULE_POWER       = 0x06U,    /**< Power management */
    DIAG_MODULE_DIAG        = 0x07U,    /**< Diagnostics itself */
} DiagModule_t;

/**
 * @brief Predefined trace event IDs
 *
 * Common events that modules can use. Module-specific events
 * should start at DIAG_EVENT_MODULE_BASE.
 */
typedef enum {
    DIAG_EVENT_NONE             = 0x0000U,
    DIAG_EVENT_INIT             = 0x0001U,    /**< Module initialization */
    DIAG_EVENT_DEINIT           = 0x0002U,    /**< Module deinitialization */
    DIAG_EVENT_ERROR            = 0x0003U,    /**< Error occurred */
    DIAG_EVENT_TIMEOUT          = 0x0004U,    /**< Timeout occurred */
    DIAG_EVENT_STATE_CHANGE     = 0x0005U,    /**< State machine transition */
    DIAG_EVENT_TASK_START       = 0x0006U,    /**< Task execution started */
    DIAG_EVENT_TASK_END         = 0x0007U,    /**< Task execution ended */
    DIAG_EVENT_ISR_ENTER        = 0x0008U,    /**< ISR entry */
    DIAG_EVENT_ISR_EXIT         = 0x0009U,    /**< ISR exit */
    DIAG_EVENT_WATCHDOG_FEED    = 0x000AU,    /**< Watchdog fed */
    DIAG_EVENT_ASSERT_FAIL      = 0x000BU,    /**< Assertion failure */
    DIAG_EVENT_WCET_VIOLATION   = 0x000CU,    /**< WCET violation detected */
    DIAG_EVENT_HEALTH_CHANGE    = 0x000DU,    /**< Task health changed */
    DIAG_EVENT_MODULE_BASE      = 0x8000U,    /**< Module-specific events start here */
} DiagEventId_t;

/*============================================================================*/
/*                          TRACE ENTRY STRUCTURE                              */
/*============================================================================*/

/**
 * @brief Trace entry structure (16 bytes)
 *
 * Compact structure for efficient ring buffer storage.
 * Designed to be 16-byte aligned for cache efficiency.
 */
typedef struct {
    uint32_t            timestamp;      /**< System tick at event */
    uint16_t            event_id;       /**< Event identifier */
    uint8_t             severity;       /**< DiagSeverity_t */
    uint8_t             module;         /**< DiagModule_t */
    uint32_t            data_0;         /**< Event-specific data word 0 */
    uint32_t            data_1;         /**< Event-specific data word 1 */
} DiagTraceEntry_t;

/**
 * @brief Trace buffer structure
 */
typedef struct {
    DiagTraceEntry_t    entries[DIAG_TRACE_BUFFER_SIZE];  /**< Circular buffer */
    volatile uint32_t   head;           /**< Write index (producer) */
    volatile uint32_t   count;          /**< Total entries written */
} DiagTraceBuffer_t;

/*============================================================================*/
/*                          SYSTEM HEALTH STRUCTURES                           */
/*============================================================================*/

/**
 * @brief Stack monitoring information
 */
typedef struct {
    uint32_t            stack_start;        /**< Stack start address */
    uint32_t            stack_end;          /**< Stack end address */
    uint32_t            stack_size;         /**< Total stack size in bytes */
    uint32_t            high_water_mark;    /**< Highest stack usage in bytes */
    uint32_t            current_usage;      /**< Current stack usage estimate */
    uint32_t            percent_used;       /**< Stack usage percentage (0-100) */
} DiagStackInfo_t;

/**
 * @brief CPU load information
 */
typedef struct {
    uint32_t            total_time_ms;      /**< Total measurement period */
    uint32_t            idle_time_ms;       /**< Time spent idle */
    uint32_t            load_percent;       /**< CPU load percentage (0-100) */
    uint32_t            peak_load_percent;  /**< Peak CPU load observed */
} DiagCpuLoad_t;

/**
 * @brief Overall system health status
 */
typedef enum {
    DIAG_HEALTH_UNKNOWN     = 0x00U,    /**< Health not yet determined */
    DIAG_HEALTH_OK          = 0x01U,    /**< System is healthy */
    DIAG_HEALTH_DEGRADED    = 0x02U,    /**< System has warnings */
    DIAG_HEALTH_CRITICAL    = 0x03U,    /**< System has critical issues */
    DIAG_HEALTH_FAILING     = 0x04U,    /**< System is about to fail */
} DiagHealthStatus_t;

/**
 * @brief System health report
 */
typedef struct {
    DiagHealthStatus_t  status;             /**< Overall health status */
    uint32_t            uptime_ms;          /**< System uptime in milliseconds */
    uint32_t            reset_count;        /**< Number of resets since power-on */
    uint32_t            error_count;        /**< Total errors logged */
    uint32_t            warning_count;      /**< Total warnings logged */
    uint32_t            assert_count;       /**< Total assertion failures */
    DiagCpuLoad_t       cpu_load;           /**< CPU load information */
    DiagStackInfo_t     stack_info;         /**< Stack usage information */
} DiagHealthReport_t;

/*============================================================================*/
/*                          PERFORMANCE COUNTERS                               */
/*============================================================================*/

/**
 * @brief Performance counter structure
 */
typedef struct {
    const char          *name;              /**< Counter name for display */
    uint32_t            count;              /**< Invocation count */
    uint32_t            total_time_us;      /**< Total accumulated time */
    uint32_t            min_time_us;        /**< Minimum observed time */
    uint32_t            max_time_us;        /**< Maximum observed time */
    uint32_t            last_time_us;       /**< Last observed time */
    bool                active;             /**< Counter is in use */
} DiagPerfCounter_t;

/**
 * @brief Performance measurement context
 *
 * Used with DIAG_PERF_START/END macros for scoped measurements.
 */
typedef struct {
    uint32_t            counter_id;         /**< Associated counter ID */
    uint32_t            start_time;         /**< Measurement start time */
} DiagPerfContext_t;

/*============================================================================*/
/*                          DUMP OUTPUT CALLBACK                               */
/*============================================================================*/

/**
 * @brief Output function callback for diagnostic dumps
 *
 * All diagnostic output goes through this callback, allowing
 * the application to route output to UART, RAM buffer, or any
 * other destination.
 *
 * @param data Pointer to data to output
 * @param length Length of data in bytes
 * @return Number of bytes actually written
 */
typedef uint32_t (*DiagOutputFunc_t)(const char *data, uint32_t length);

/*============================================================================*/
/*                          PUBLIC API                                         */
/*============================================================================*/

/*----------------------------------------------------------------------------*/
/*                          INITIALIZATION                                     */
/*----------------------------------------------------------------------------*/

/**
 * @brief Initialize the diagnostics system
 *
 * Must be called before any other diagnostics functions.
 * Sets up trace buffers, performance counters, and health monitoring.
 *
 * @param output_func Output function for diagnostic dumps (can be NULL initially)
 */
void Diag_Init(DiagOutputFunc_t output_func);

/**
 * @brief Set the output function for diagnostic dumps
 *
 * @param output_func Output function (NULL to disable output)
 */
void Diag_SetOutput(DiagOutputFunc_t output_func);

/*----------------------------------------------------------------------------*/
/*                          TRACE RECORDING                                    */
/*----------------------------------------------------------------------------*/

/**
 * @brief Record a trace event (ISR-safe)
 *
 * @param severity Event severity level
 * @param module Source module
 * @param event_id Event identifier
 * @param data_0 Event-specific data word 0
 * @param data_1 Event-specific data word 1
 */
void Diag_Trace(DiagSeverity_t severity,
                DiagModule_t module,
                DiagEventId_t event_id,
                uint32_t data_0,
                uint32_t data_1);

/**
 * @brief Record a simple trace event with no data
 */
void Diag_TraceSimple(DiagSeverity_t severity,
                      DiagModule_t module,
                      DiagEventId_t event_id);

/**
 * @brief Record a trace event with an error code
 */
void Diag_TraceError(DiagModule_t module,
                     DiagEventId_t event_id,
                     uint32_t error_code);

/**
 * @brief Get number of trace entries available
 */
uint32_t Diag_TraceGetCount(void);

/**
 * @brief Read trace entry by index (0 = oldest)
 *
 * @param index Index into trace buffer
 * @param entry Pointer to store trace entry
 * @return true if entry found, false if index out of range
 */
bool Diag_TraceGetEntry(uint32_t index, DiagTraceEntry_t *entry);

/**
 * @brief Clear the trace buffer
 */
void Diag_TraceClear(void);

/*----------------------------------------------------------------------------*/
/*                          PERFORMANCE COUNTERS                               */
/*----------------------------------------------------------------------------*/

/**
 * @brief Register a performance counter
 *
 * @param name Counter name (string is not copied, must be persistent)
 * @return Counter ID (>= 0) on success, -1 on failure
 */
int32_t Diag_PerfRegister(const char *name);

/**
 * @brief Start a performance measurement
 *
 * @param counter_id Counter ID from Diag_PerfRegister
 * @return Start timestamp (for Diag_PerfEnd)
 */
uint32_t Diag_PerfStart(int32_t counter_id);

/**
 * @brief End a performance measurement
 *
 * @param counter_id Counter ID
 * @param start_time Start timestamp from Diag_PerfStart
 */
void Diag_PerfEnd(int32_t counter_id, uint32_t start_time);

/**
 * @brief Get performance counter data
 *
 * @param counter_id Counter ID
 * @param counter Pointer to store counter data
 * @return true if counter found, false otherwise
 */
bool Diag_PerfGetCounter(int32_t counter_id, DiagPerfCounter_t *counter);

/**
 * @brief Reset a performance counter
 *
 * @param counter_id Counter ID
 * @return true if counter found and reset, false otherwise
 */
bool Diag_PerfReset(int32_t counter_id);

/**
 * @brief Reset all performance counters
 */
void Diag_PerfResetAll(void);

/*----------------------------------------------------------------------------*/
/*                          SYSTEM HEALTH MONITORING                           */
/*----------------------------------------------------------------------------*/

/**
 * @brief Update system health metrics
 *
 * Should be called periodically (e.g., from scheduler idle task)
 * to refresh CPU load and stack usage estimates.
 */
void Diag_UpdateHealth(void);

/**
 * @brief Get current system health report
 *
 * @param report Pointer to store health report
 */
void Diag_GetHealthReport(DiagHealthReport_t *report);

/**
 * @brief Get current health status
 *
 * @return Current DiagHealthStatus_t
 */
DiagHealthStatus_t Diag_GetHealthStatus(void);

/**
 * @brief Initialize stack monitoring
 *
 * Fills unused stack with pattern for high-water mark detection.
 * Must be called early in startup, before stack is heavily used.
 *
 * @param stack_start Start address of stack region
 * @param stack_size Size of stack in bytes
 */
void Diag_InitStackMonitor(uint32_t stack_start, uint32_t stack_size);

/**
 * @brief Update CPU load from scheduler statistics
 *
 * @param total_time_ms Total measurement period
 * @param idle_time_ms Time spent idle
 */
void Diag_UpdateCpuLoad(uint32_t total_time_ms, uint32_t idle_time_ms);

/*----------------------------------------------------------------------------*/
/*                          DIAGNOSTIC DUMP                                    */
/*----------------------------------------------------------------------------*/

/**
 * @brief Dump complete diagnostic information
 *
 * Outputs a structured report including:
 * - System health status
 * - Stack usage
 * - CPU load
 * - Recent trace entries
 * - Performance counter summary
 * - Error statistics (from error handler integration)
 */
void Diag_DumpAll(void);

/**
 * @brief Dump only the trace buffer
 *
 * @param count Number of recent entries to dump (0 = all)
 */
void Diag_DumpTrace(uint32_t count);

/**
 * @brief Dump system health information only
 */
void Diag_DumpHealth(void);

/**
 * @brief Dump performance counter summary
 */
void Diag_DumpPerformance(void);

/**
 * @brief Dump a formatted header line
 *
 * @param title Header title
 */
void Diag_DumpHeader(const char *title);

/**
 * @brief Dump a separator line
 */
void Diag_DumpSeparator(void);

/*----------------------------------------------------------------------------*/
/*                          UTILITY FUNCTIONS                                  */
/*----------------------------------------------------------------------------*/

/**
 * @brief Get severity name string
 *
 * @param severity Severity level
 * @return Human-readable name string
 */
const char *Diag_GetSeverityName(DiagSeverity_t severity);

/**
 * @brief Get module name string
 *
 * @param module Module identifier
 * @return Human-readable name string
 */
const char *Diag_GetModuleName(DiagModule_t module);

/**
 * @brief Get event name string
 *
 * @param event_id Event identifier
 * @return Human-readable name string
 */
const char *Diag_GetEventName(DiagEventId_t event_id);

/**
 * @brief Get current timestamp in microseconds
 *
 * Platform-specific implementation required. Returns 0 if not implemented.
 *
 * @return Current time in microseconds
 */
uint32_t Diag_GetTimeUs(void);

/**
 * @brief Get current timestamp in milliseconds
 *
 * @return Current time in milliseconds
 */
uint32_t Diag_GetTimeMs(void);

/*============================================================================*/
/*                          CONVENIENCE MACROS                                 */
/*============================================================================*/

/**
 * @brief Quick trace macros for common events
 */
#define DIAG_TRACE_INIT(module) \
    Diag_TraceSimple(DIAG_SEVERITY_INFO, (module), DIAG_EVENT_INIT)

#define DIAG_TRACE_ERROR(module, err) \
    Diag_TraceError((module), DIAG_EVENT_ERROR, (err))

#define DIAG_TRACE_WARNING(module, event, data) \
    Diag_Trace(DIAG_SEVERITY_WARNING, (module), (event), (data), 0)

#define DIAG_TRACE_DEBUG(module, event, d0, d1) \
    Diag_Trace(DIAG_SEVERITY_DEBUG, (module), (event), (d0), (d1))

/**
 * @brief Performance measurement macros
 *
 * Usage:
 *   int32_t counter = Diag_PerfRegister("MyFunction");
 *   // In measured code:
 *   DIAG_PERF_SCOPE(counter);
 *   // Or manually:
 *   uint32_t t = Diag_PerfStart(counter);
 *   ... measured code ...
 *   Diag_PerfEnd(counter, t);
 */
#define DIAG_PERF_SCOPE(counter_id) \
    DiagPerfContext_t _perf_ctx = { (counter_id), Diag_GetTimeUs() }; \
    (void)_perf_ctx

#define DIAG_PERF_SCOPE_END() \
    do { \
        Diag_PerfEnd(_perf_ctx.counter_id, _perf_ctx.start_time); \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* DIAGNOSTICS_H */
