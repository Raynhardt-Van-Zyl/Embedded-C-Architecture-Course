/**
 * @file app_assert.c
 * @brief Embedded Assertion System Implementation
 * 
 * @author Embedded C Architecture Course
 */

#include "app_assert.h"
#include <string.h>

/*============================================================================*/
/*                          PLATFORM ADAPTATION                                */
/*============================================================================*/

#if defined(__arm__) || defined(__thumb__)
    /* ARM Cortex-M Specific (GCC/Clang) */
    #define GET_TIMESTAMP()         (0) // Placeholder for Systick
    #define CAPTURE_PC(pc)          __asm volatile("mov %0, pc" : "=r"(pc))
    #define CAPTURE_LR(lr)          __asm volatile("mov %0, lr" : "=r"(lr))
    #define SYSTEM_RESET()          NVIC_SystemReset()
    #define DEBUG_BREAK()           __asm volatile("bkpt #0")
    #define DISABLE_INTERRUPTS()    __asm volatile("cpsid i" : : : "memory")
    #define ENABLE_INTERRUPTS()     __asm volatile("cpsie i" : : : "memory")
    
    /* External Reset Function (Standard CMSIS) */
    extern void NVIC_SystemReset(void);

#elif defined(_WIN32) || defined(__linux__)
    /* Host Simulation */
    #include <stdio.h>
    #include <stdlib.h>
    #define GET_TIMESTAMP()         (0)
    #define CAPTURE_PC(pc)          ((void)((pc) = 0))
    #define CAPTURE_LR(lr)          ((void)((lr) = 0))
    #define SYSTEM_RESET()          exit(1)
    #define DEBUG_BREAK()           ((void)0)
    #define DISABLE_INTERRUPTS()    ((void)0)
    #define ENABLE_INTERRUPTS()     ((void)0)

#else
    /* Generic/Unknown Platform */
    #define GET_TIMESTAMP()         (0)
    #define CAPTURE_PC(pc)          ((pc) = 0)
    #define CAPTURE_LR(lr)          ((lr) = 0)
    #define SYSTEM_RESET()          while(1)
    #define DEBUG_BREAK()           ((void)0)
    #define DISABLE_INTERRUPTS()    ((void)0)
    #define ENABLE_INTERRUPTS()     ((void)0)
#endif

/*============================================================================*/
/*                          MODULE-LOCAL DATA                                  */
/*============================================================================*/

static AssertLog_t s_assert_log = {0};
static AssertStats_t s_assert_stats = {0};
static AssertHandler_t s_custom_handler = NULL;
static bool s_initialized = false;

/*============================================================================*/
/*                          PRIVATE FUNCTIONS                                  */
/*============================================================================*/

/**
 * @brief Provide a hook for the application to perform emergency shutdown
 * or kick the watchdog during a halt.
 */
__attribute__((weak)) void App_Assert_OnHaltHook(void) {
    /* Default implementation does nothing. 
       User should override to kick WDT or set safe outputs. */
}

static void log_assertion(const AssertRecord_t *record) {
    s_assert_log.records[s_assert_log.head] = *record;
    s_assert_log.head = (s_assert_log.head + 1U) % APP_ASSERT_LOG_SIZE;
    if (s_assert_log.count < APP_ASSERT_LOG_SIZE) {
        s_assert_log.count++;
    }
    s_assert_stats.unique_count++;
    s_assert_stats.total_count++;
}

static void execute_default_action(void) {
#if (APP_ASSERT_ACTION == 1)
    /* Hang in infinite loop - SAFELY */
    DISABLE_INTERRUPTS();
    while (1) {
        App_Assert_OnHaltHook(); // ALLOW WDT KICK OR SAFE STATE
        DEBUG_BREAK();
    }
#elif (APP_ASSERT_ACTION == 2)
    SYSTEM_RESET();
#else
    SYSTEM_RESET();
#endif
}

/*============================================================================*/
/*                          PUBLIC API                                         */
/*============================================================================*/

void App_Assert_Init(void) {
    if (s_initialized) return;
    memset(&s_assert_log, 0, sizeof(s_assert_log));
    memset(&s_assert_stats, 0, sizeof(s_assert_stats));
    s_initialized = true;
}

void App_Assert_Fail(const char *condition, const char *file, uint32_t line, const char *function) {
    AssertRecord_t record = {0};
    DISABLE_INTERRUPTS();
    
    record.condition = condition;
    record.file = file;
    record.line = line;
    record.function = function;
    
    CAPTURE_PC(record.program_counter);
    CAPTURE_LR(record.link_register);
    
    log_assertion(&record);
    
    bool handled = false;
    if (s_custom_handler) {
        handled = s_custom_handler(&record);
    }
    
    if (handled) {
        ENABLE_INTERRUPTS();
        return;
    }
    
    execute_default_action();
}

void App_Assert_SetHandler(AssertHandler_t handler) {
    s_custom_handler = handler;
}
