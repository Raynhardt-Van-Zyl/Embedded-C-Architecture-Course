/**
 * @file isr_safe.h
 * @brief ISR-Safe Programming Patterns Header
 */

#ifndef ISR_SAFE_H
#define ISR_SAFE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef ISR_ENABLE_NESTING
#define ISR_ENABLE_NESTING          (1U)
#endif

#ifndef ISR_RING_BUFFER_SIZE
#define ISR_RING_BUFFER_SIZE        (64U)
#endif

#if (ISR_RING_BUFFER_SIZE & (ISR_RING_BUFFER_SIZE - 1U)) != 0U
#error "ISR_RING_BUFFER_SIZE must be a power of 2"
#endif

#if defined(__arm__) || defined(__thumb__)
    /* ARM Cortex-M implementation */
    static inline uint32_t __get_PRIMASK(void) {
        uint32_t result;
        __asm volatile ("MRS %0, primask" : "=r" (result) :: "memory");
        return result;
    }
    static inline void __set_PRIMASK(uint32_t priMask) {
        __asm volatile ("MSR primask, %0" : : "r" (priMask) : "memory");
    }
    static inline void __disable_irq(void) {
        __asm volatile ("cpsid i" : : : "memory");
    }
    static inline void __enable_irq(void) {
        __asm volatile ("cpsie i" : : : "memory");
    }
    
    #if (ISR_ENABLE_NESTING == 1U)
        extern volatile uint32_t g_criticalNestingDepth;
        extern volatile uint32_t g_savedPrimask;
        
        #define ISR_CRITICAL_ENTER()                                              \
            do {                                                                   \
                uint32_t _primask = __get_PRIMASK();                              \
                __disable_irq();                                                   \
                if (g_criticalNestingDepth == 0U) {                                \
                    g_savedPrimask = _primask;                                     \
                }                                                                  \
                g_criticalNestingDepth++;                                          \
            } while (0)
            
        #define ISR_CRITICAL_EXIT()                                               \
            do {                                                                   \
                if (g_criticalNestingDepth > 0) {                                  \
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
    /* Generic fallback for Host/Simulation */
    extern volatile uint32_t g_criticalNestingDepth;
    #define ISR_CRITICAL_ENTER()      (g_criticalNestingDepth++)
    #define ISR_CRITICAL_EXIT()       (g_criticalNestingDepth > 0 ? g_criticalNestingDepth-- : 0)
#endif

#define ISR_CRITICAL_SECTION_BEGIN()    ISR_CRITICAL_ENTER()
#define ISR_CRITICAL_SECTION_END()      ISR_CRITICAL_EXIT()

#if defined(__GNUC__) || defined(__clang__)
    #define ISR_COMPILER_BARRIER()      __asm__ __volatile__("" ::: "memory")
#elif defined(__ARMCC_VERSION)
    #define ISR_COMPILER_BARRIER()      __schedule_barrier()
#else
    #define ISR_COMPILER_BARRIER()
#endif

#if defined(__arm__) || defined(__thumb__)
    #define ISR_MEMORY_BARRIER()        do { ISR_COMPILER_BARRIER(); __asm volatile ("dmb sy" ::: "memory"); } while (0)
    #define ISR_DATA_SYNC_BARRIER()     do { ISR_COMPILER_BARRIER(); __asm volatile ("dsb sy" ::: "memory"); } while (0)
    #define ISR_INSTRUCTION_SYNC_BARRIER() do { ISR_COMPILER_BARRIER(); __asm volatile ("isb sy" ::: "memory"); } while (0)
#else
    #define ISR_MEMORY_BARRIER()         ISR_COMPILER_BARRIER()
    #define ISR_DATA_SYNC_BARRIER()      ISR_COMPILER_BARRIER()
    #define ISR_INSTRUCTION_SYNC_BARRIER() ISR_COMPILER_BARRIER()
#endif

typedef uint8_t RingBufferData_t;

typedef struct {
    RingBufferData_t        buffer[ISR_RING_BUFFER_SIZE];
    volatile uint32_t       head;
    volatile uint32_t       tail;
    uint32_t                size;
    uint32_t                mask;
} RingBuffer_t;

typedef enum {
    RING_BUFFER_OK          = 0U,
    RING_BUFFER_EMPTY       = 1U,
    RING_BUFFER_FULL        = 2U,
    RING_BUFFER_INVALID     = 3U
} RingBufferResult_t;

typedef volatile uint32_t AtomicFlag_t;

typedef enum {
    FLAG_CLEAR              = 0U,
    FLAG_SET                = 1U
} FlagValue_t;

void ISR_Init(void);
bool ISR_IsInInterrupt(void);
RingBufferResult_t RingBuffer_Init(RingBuffer_t *rb);
RingBufferResult_t RingBuffer_Write(RingBuffer_t *rb, uint8_t data);
RingBufferResult_t RingBuffer_Read(RingBuffer_t *rb, uint8_t *data);
uint32_t RingBuffer_GetCount(const RingBuffer_t *rb);
bool RingBuffer_IsEmpty(const RingBuffer_t *rb);
bool RingBuffer_IsFull(const RingBuffer_t *rb);
void RingBuffer_Flush(RingBuffer_t *rb);
void Flag_Set(AtomicFlag_t *flag);
void Flag_Clear(AtomicFlag_t *flag);
bool Flag_TestAndClear(AtomicFlag_t *flag);
FlagValue_t Flag_Get(const AtomicFlag_t *flag);

#endif /* ISR_SAFE_H */
