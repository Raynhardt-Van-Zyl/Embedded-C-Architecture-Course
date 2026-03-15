/**
 * @file isr_safe.h
 * @brief ISR-Safe Programming Patterns Header
 * 
 * ARCHITECTURE OVERVIEW:
 * =====================
 * This module provides safe patterns for communication between Interrupt
 * Service Routines (ISRs) and main-loop code. These patterns are essential
 * for reliable embedded systems.
 * 
 * KEY CONCEPTS:
 * 
 * 1. RING BUFFER (Lock-Free SPSC):
 *    - Single Producer, Single Consumer queue
 *    - Lock-free design using atomic operations
 *    - Safe for ISR-to-main communication
 *    - No critical sections required in single-core systems
 * 
 * 2. FLAG MANAGEMENT:
 *    - Atomic flag operations for event signaling
 *    - Memory barrier guarantees for visibility
 *    - Proper volatile usage for shared variables
 * 
 * 3. CRITICAL SECTIONS:
 *    - Portable macros for interrupt disable/enable
 *    - Nested critical section support
 *    - Minimal critical section duration
 * 
 * SAFETY CONSIDERATIONS:
 * - All shared data must use volatile qualifier
 * - Memory barriers ensure correct ordering
 * - Critical sections must be as short as possible
 * - ISRs must never block or call blocking functions
 * 
 * PORTABILITY:
 * - Critical section macros can be adapted to any platform
 * - Ring buffer works on any architecture with atomic 8/16/32-bit access
 * - For multi-core systems, additional cache coherency measures needed
 * 
 * @author Embedded C Architecture Course
 * @version 1.0
 */

#ifndef ISR_SAFE_H
#define ISR_SAFE_H

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
 * @brief Enable critical section nesting tracking
 * 
 * When enabled, nested critical sections are tracked and only the outermost
 * exit re-enables interrupts. This is safer but has slight overhead.
 */
#ifndef ISR_ENABLE_NESTING
#define ISR_ENABLE_NESTING          (1U)
#endif

/**
 * @brief Enable memory barrier instrumentation (for debugging)
 */
#ifndef ISR_ENABLE_BARRIER_DEBUG
#define ISR_ENABLE_BARRIER_DEBUG    (0U)
#endif

/**
 * @brief Ring buffer size (must be power of 2 for efficient modulo)
 * 
 * DESIGN NOTE: Power-of-2 sizes allow using bitwise AND instead of modulo,
 * which is much faster on most processors.
 */
#ifndef ISR_RING_BUFFER_SIZE
#define ISR_RING_BUFFER_SIZE        (64U)
#endif

/* Validate ring buffer size is power of 2 */
#if (ISR_RING_BUFFER_SIZE & (ISR_RING_BUFFER_SIZE - 1U)) != 0U
#error "ISR_RING_BUFFER_SIZE must be a power of 2"
#endif

/*============================================================================*/
/*                             CRITICAL SECTION MACROS                         */
/*============================================================================*/

/**
 * CRITICAL SECTION DESIGN
 * =======================
 * 
 * Critical sections disable interrupts to create atomic operations. They must:
 * 
 * 1. Be as SHORT as possible - every cycle with interrupts disabled is
 *    a cycle that can't respond to external events.
 * 
 * 2. Never call BLOCKING functions - this would deadlock the system.
 * 
 * 3. Be BALANCED - every ENTER must have a matching EXIT.
 * 
 * 4. Use NESTING support when critical sections might overlap.
 * 
 * EXAMPLE USAGE:
 * @code
 * uint32_t sharedData;
 * 
 * void UpdateSharedData(uint32_t newValue)
 * {
 *     ISR_CRITICAL_ENTER();
 *     sharedData = newValue;  // Atomic update
 *     ISR_CRITICAL_EXIT();
 * }
 * @endcode
 */

/**
 * @brief Platform-specific interrupt disable function
 * 
 * IMPLEMENTATION NOTES:
 * - ARM Cortex-M: Use __disable_irq() or PRIMASK manipulation
 * - AVR: Use cli() or SREG manipulation
 * - PIC: Use INTCON register manipulation
 * 
 * For ARM Cortex-M (replace with your platform's implementation):
 */
#if defined(__ARM_ARCH) || defined(__ARMCC_VERSION) || defined(__GNUC__)
    /* ARM Cortex-M implementation */
    #include "cmsis_compiler.h"  /* Or define your own */
    
    /* Store previous interrupt state for nesting support */
    #if (ISR_ENABLE_NESTING == 1U)
        extern volatile uint32_t g_criticalNestingDepth;
        extern volatile uint32_t g_savedPrimask;

        /*
         * State transitions:
         * - First enter: save PRIMASK, disable IRQs, depth 0 -> 1
         * - Nested enter: depth increments while IRQs remain disabled
         * - Nested exit: depth decrements, IRQ state unchanged
         * - Final exit: depth 1 -> 0, restore saved PRIMASK
         */
        
        #define ISR_CRITICAL_ENTER()                                              \
            do {                                                                   \
                uint32_t _primask = __get_PRIMASK();                              \
                __disable_irq();                                                   \
                if (g_criticalNestingDepth == 0U) {                                 \
                    g_savedPrimask = _primask;                                      \
                }                                                                  \
                g_criticalNestingDepth++;                                          \
            } while (0)
            
        #define ISR_CRITICAL_EXIT()                                               \
            do {                                                                   \
                if (g_criticalNestingDepth > 0U) {                                 \
                    g_criticalNestingDepth--;                                      \
                    if (g_criticalNestingDepth == 0U) {                            \
                        __set_PRIMASK(g_savedPrimask);                             \
                    }                                                              \
                }                                                                  \
            } while (0)
    #else
        #define ISR_CRITICAL_ENTER()      __disable_irq()
        #define ISR_CRITICAL_EXIT()       __enable_irq()
    #endif
    
#else
    /* Generic fallback - implement for your platform */
    extern volatile uint32_t g_criticalNestingDepth;
    
    #define ISR_CRITICAL_ENTER()                                              \
        do {                                                                   \
            /* TODO: Implement platform-specific interrupt disable */          \
            g_criticalNestingDepth++;                                          \
        } while (0)
        
    #define ISR_CRITICAL_EXIT()                                               \
        do {                                                                   \
            if (g_criticalNestingDepth > 0U) {                                 \
                g_criticalNestingDepth--;                                      \
            }                                                                  \
            if (g_criticalNestingDepth == 0U) {                                \
                /* TODO: Implement platform-specific interrupt enable */       \
            }                                                                  \
        } while (0)
#endif

/**
 * @brief Helper macro for scoped critical sections
 * 
 * USAGE:
 * @code
 * void UpdateData(void)
 * {
 *     ISR_CRITICAL_SECTION_BEGIN();
 *     sharedData = newValue;
 *     ISR_CRITICAL_SECTION_END();
 * }
 * @endcode
 */
#define ISR_CRITICAL_SECTION_BEGIN()    ISR_CRITICAL_ENTER()
#define ISR_CRITICAL_SECTION_END()      ISR_CRITICAL_EXIT()

/*============================================================================*/
/*                             MEMORY BARRIERS                                 */
/*============================================================================*/

/**
 * MEMORY BARRIER DESIGN
 * =====================
 * 
 * Memory barriers (also called fences) ensure that memory operations occur
 * in the correct order. This is critical when:
 * 
 * 1. SHARED VARIABLES: Main code and ISR access the same variables
 * 2. MULTI-CORE SYSTEMS: Different cores need to see consistent memory
 * 3. DMA OPERATIONS: CPU and DMA controller share memory
 * 
 * WHY BARRIERS ARE NEEDED:
 * - Compilers may reorder memory operations for optimization
 * - Processors may execute memory operations out of order
 * - Write buffers may delay visible effects of stores
 * 
 * VOLATILE vs BARRIERS:
 * - volatile prevents compiler optimizations on a variable
 * - volatile does NOT prevent hardware reordering
 * - Barriers ensure ordering at both compiler and hardware level
 */

/**
 * @brief Compiler memory barrier
 * 
 * Prevents compiler from reordering memory operations across this point.
 * Does not generate any instructions, just constrains optimization.
 */
#if defined(__GNUC__) || defined(__clang__)
    #define ISR_COMPILER_BARRIER()      __asm__ __volatile__("" ::: "memory")
#elif defined(__ARMCC_VERSION)
    #define ISR_COMPILER_BARRIER()      __schedule_barrier()
#else
    #define ISR_COMPILER_BARRIER()      /* Fallback - may not work on all compilers */
#endif

/**
 * @brief Full memory barrier (compiler + hardware)
 * 
 * Ensures all memory operations before this point complete before
 * any memory operations after this point begin.
 * 
 * On ARM Cortex-M, DSB (Data Synchronization Barrier) ensures:
 * - All explicit memory accesses complete
 * - All cache maintenance operations complete
 */
#if defined(__ARM_ARCH) || defined(__ARMCC_VERSION) || defined(__GNUC__)
    #define ISR_MEMORY_BARRIER()                                              \
        do {                                                                   \
            ISR_COMPILER_BARRIER();                                            \
            __DMB();  /* ARM Data Memory Barrier */                            \
        } while (0)
#else
    #define ISR_MEMORY_BARRIER()         ISR_COMPILER_BARRIER()
#endif

/**
 * @brief Data synchronization barrier
 * 
 * Stronger than DMB - waits for all memory operations to complete.
 * Use before triggering operations that depend on memory state
 * (e.g., before starting DMA).
 */
#if defined(__ARM_ARCH) || defined(__ARMCC_VERSION) || defined(__GNUC__)
    #define ISR_DATA_SYNC_BARRIER()                                           \
        do {                                                                   \
            ISR_COMPILER_BARRIER();                                            \
            __DSB();  /* ARM Data Synchronization Barrier */                   \
        } while (0)
#else
    #define ISR_DATA_SYNC_BARRIER()      ISR_COMPILER_BARRIER()
#endif

/**
 * @brief Instruction synchronization barrier
 * 
 * Ensures instruction cache is synchronized. Use after modifying
 * code (e.g., flash programming) or changing processor state.
 */
#if defined(__ARM_ARCH) || defined(__ARMCC_VERSION) || defined(__GNUC__)
    #define ISR_INSTRUCTION_SYNC_BARRIER()                                    \
        do {                                                                   \
            ISR_COMPILER_BARRIER();                                            \
            __ISB();  /* ARM Instruction Synchronization Barrier */            \
        } while (0)
#else
    #define ISR_INSTRUCTION_SYNC_BARRIER()  ISR_COMPILER_BARRIER()
#endif

/*============================================================================*/
/*                             RING BUFFER TYPES                               */
/*============================================================================*/

/**
 * RING BUFFER DESIGN
 * ==================
 * 
 * This is a Single Producer, Single Consumer (SPSC) lock-free ring buffer.
 * It is designed for ISR-to-main communication where:
 * - ISR (producer) writes data
 * - Main code (consumer) reads data
 * - Only one ISR and one main context access the buffer
 * 
 * LOCK-FREE OPERATION:
 * The buffer uses separate head and tail indices that are only modified
 * by one context each. This eliminates the need for locks:
 * - ISR modifies head (write pointer)
 * - Main modifies tail (read pointer)
 * - Both read both pointers, but only write their own
 * 
 * ATOMICITY GUARANTEES:
 * On single-core systems, 8/16/32-bit reads and writes are atomic
 * (cannot be interrupted mid-operation). This is key to lock-free design.
 * 
 * MEMORY ORDERING:
 * Memory barriers ensure that:
 * 1. Writes to buffer data complete before head is updated
 * 2. Reads from buffer data occur after tail is read
 */

/**
 * @brief Ring buffer data type
 * 
 * DESIGN NOTE: Using uint8_t allows the buffer to store any data type.
 * For structured data, serialize/deserialize or use a typed wrapper.
 */
typedef uint8_t RingBufferData_t;

/**
 * @brief Ring buffer structure
 * 
 * LAYOUT: Fields are ordered to minimize false sharing (though this
 * is less relevant on single-core systems).
 * 
 * volatile QUALIFIER: Essential for correct operation. Without it,
 * the compiler might cache values in registers, breaking the
 * communication between ISR and main code.
 */
typedef struct {
    /* Buffer storage */
    RingBufferData_t        buffer[ISR_RING_BUFFER_SIZE];   /* Data storage */
    
    /* Indices - volatile is CRITICAL */
    volatile uint32_t       head;   /* Write index (modified by producer/ISR) */
    volatile uint32_t       tail;   /* Read index (modified by consumer/main) */
    
    /* Configuration */
    uint32_t                size;   /* Buffer size (cached for speed) */
    uint32_t                mask;   /* Size - 1 (for fast modulo with AND) */
    
} RingBuffer_t;

/**
 * @brief Ring buffer operation result
 */
typedef enum {
    RING_BUFFER_OK          = 0U,   /* Operation succeeded */
    RING_BUFFER_EMPTY       = 1U,   /* Buffer is empty (no data to read) */
    RING_BUFFER_FULL        = 2U,   /* Buffer is full (cannot write) */
    RING_BUFFER_INVALID     = 3U,   /* Invalid parameter */
    RING_BUFFER_PARTIAL     = 4U    /* Partial read/write (buffer full/empty) */
} RingBufferResult_t;

/*============================================================================*/
/*                             FLAG MANAGEMENT                                 */
/*============================================================================*/

/**
 * FLAG MANAGEMENT DESIGN
 * ======================
 * 
 * Flags are single-bit variables used for event signaling between
 * ISR and main code. They are simpler than ring buffers when only
 * notification is needed (no data transfer).
 * 
 * ATOMIC FLAG OPERATIONS:
 * Single-bit operations require read-modify-write sequences that
 * are NOT atomic by default. Use critical sections or atomic bit
 * manipulation instructions (if available).
 */

/**
 * @brief Atomic flag type
 * 
 * DESIGN NOTE: Using a full 32-bit value for a single flag might seem
 * wasteful, but it ensures atomic access on all architectures.
 * For multiple flags, use a bit field with atomic operations.
 */
typedef volatile uint32_t AtomicFlag_t;

/**
 * @brief Flag value enumeration
 */
typedef enum {
    FLAG_CLEAR              = 0U,
    FLAG_SET                = 1U
} FlagValue_t;

/*============================================================================*/
/*                             API FUNCTIONS                                   */
/*============================================================================*/

/**
 * @brief Initialize critical section support
 * 
 * DESIGN NOTE: Must be called before any critical section operations.
 * Initializes nesting counter and any platform-specific state.
 */
void ISR_Init(void);

/**
 * @brief Check if currently executing in interrupt context
 * 
 * @return true if in ISR, false if in main context
 * 
 * DESIGN NOTE: Useful for code that can be called from both contexts.
 * On ARM Cortex-M, check IPSR register or VECTACTIVE field.
 */
bool ISR_IsInInterrupt(void);

/*----------------------------------------------------------------------------*/
/*                         RING BUFFER FUNCTIONS                               */
/*----------------------------------------------------------------------------*/

/**
 * @brief Initialize a ring buffer
 * 
 * @param rb Pointer to ring buffer structure
 * @return RING_BUFFER_OK on success, error code otherwise
 * 
 * DESIGN NOTE: Must be called before using the buffer. Both ISR
 * and main code can safely check if buffer is initialized by
 * verifying size > 0.
 */
RingBufferResult_t RingBuffer_Init(RingBuffer_t *rb);

/**
 * @brief Write a single byte to the ring buffer (ISR-safe)
 * 
 * @param rb Pointer to ring buffer structure
 * @param data Byte to write
 * @return RING_BUFFER_OK on success, RING_BUFFER_FULL if buffer is full
 * 
 * USAGE: Call from ISR or main code to add data.
 * 
 * ATOMICITY: This function is safe to call from ISR without additional
 * protection, as long as only one context calls Write.
 */
RingBufferResult_t RingBuffer_Write(RingBuffer_t *rb, uint8_t data);

/**
 * @brief Write multiple bytes to the ring buffer
 * 
 * @param rb Pointer to ring buffer structure
 * @param data Pointer to data to write
 * @param length Number of bytes to write
 * @param bytesWritten Pointer to store number of bytes actually written
 * @return RING_BUFFER_OK if all bytes written, RING_BUFFER_PARTIAL if partial
 * 
 * DESIGN NOTE: More efficient than multiple single-byte writes.
 */
RingBufferResult_t RingBuffer_WriteMultiple(
    RingBuffer_t            *rb,
    const uint8_t           *data,
    uint32_t                length,
    uint32_t                *bytesWritten
);

/**
 * @brief Read a single byte from the ring buffer (ISR-safe)
 * 
 * @param rb Pointer to ring buffer structure
 * @param data Pointer to store read byte
 * @return RING_BUFFER_OK on success, RING_BUFFER_EMPTY if buffer is empty
 * 
 * USAGE: Call from main code (consumer) to read data written by ISR.
 */
RingBufferResult_t RingBuffer_Read(RingBuffer_t *rb, uint8_t *data);

/**
 * @brief Read multiple bytes from the ring buffer
 * 
 * @param rb Pointer to ring buffer structure
 * @param data Pointer to buffer for read data
 * @param maxLength Maximum bytes to read
 * @param bytesRead Pointer to store number of bytes actually read
 * @return RING_BUFFER_OK if any bytes read, RING_BUFFER_EMPTY if none
 */
RingBufferResult_t RingBuffer_ReadMultiple(
    RingBuffer_t            *rb,
    uint8_t                 *data,
    uint32_t                maxLength,
    uint32_t                *bytesRead
);

/**
 * @brief Get number of bytes available to read
 * 
 * @param rb Pointer to ring buffer structure
 * @return Number of bytes available
 * 
 * DESIGN NOTE: This is a snapshot that may be stale by the time you
 * act on it. Always check return value of Read operations.
 */
uint32_t RingBuffer_GetCount(const RingBuffer_t *rb);

/**
 * @brief Get free space available for writing
 * 
 * @param rb Pointer to ring buffer structure
 * @return Number of bytes that can be written
 */
uint32_t RingBuffer_GetFree(const RingBuffer_t *rb);

/**
 * @brief Check if buffer is empty
 * 
 * @param rb Pointer to ring buffer structure
 * @return true if empty, false otherwise
 */
bool RingBuffer_IsEmpty(const RingBuffer_t *rb);

/**
 * @brief Check if buffer is full
 * 
 * @param rb Pointer to ring buffer structure
 * @return true if full, false otherwise
 */
bool RingBuffer_IsFull(const RingBuffer_t *rb);

/**
 * @brief Flush the ring buffer (discard all data)
 * 
 * @param rb Pointer to ring buffer structure
 * 
 * DESIGN NOTE: Only safe to call from consumer context.
 */
void RingBuffer_Flush(RingBuffer_t *rb);

/**
 * @brief Peek at data without removing it
 * 
 * @param rb Pointer to ring buffer structure
 * @param data Pointer to store peeked byte
 * @param offset Offset from head (0 = oldest data)
 * @return RING_BUFFER_OK on success, error code otherwise
 */
RingBufferResult_t RingBuffer_Peek(
    const RingBuffer_t      *rb,
    uint8_t                 *data,
    uint32_t                offset
);

/*----------------------------------------------------------------------------*/
/*                         FLAG MANAGEMENT FUNCTIONS                           */
/*----------------------------------------------------------------------------*/

/**
 * @brief Set a flag (ISR-safe)
 * 
 * @param flag Pointer to flag variable
 * 
 * DESIGN NOTE: Uses memory barrier to ensure visibility.
 */
void Flag_Set(AtomicFlag_t *flag);

/**
 * @brief Clear a flag (ISR-safe)
 * 
 * @param flag Pointer to flag variable
 */
void Flag_Clear(AtomicFlag_t *flag);

/**
 * @brief Check and clear a flag atomically
 * 
 * @param flag Pointer to flag variable
 * @return true if flag was set (now cleared), false if was clear
 * 
 * DESIGN NOTE: This is a test-and-clear operation that is atomic.
 * It prevents race conditions where an ISR might set the flag
 * between your test and clear operations.
 */
bool Flag_TestAndClear(AtomicFlag_t *flag);

/**
 * @brief Read flag value (non-destructive)
 * 
 * @param flag Pointer to flag variable
 * @return Current flag value
 */
FlagValue_t Flag_Get(const AtomicFlag_t *flag);

#endif /* ISR_SAFE_H */
