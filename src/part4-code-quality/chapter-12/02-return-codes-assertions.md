# 12.2 Return Codes and Assertions

## 1. The Explicit Nature of C Error Handling
Unlike high-level languages (C++, Java, Python), the C language has no built-in exception handling mechanism (`try/catch`). When a function encounters an error, it cannot "throw" an exception to secretly unroll the stack and find a handler.

In embedded systems, this lack of exceptions is actually a profound architectural strength. Exceptions hide control flow. They make it impossible to determine exactly how long a function will take to execute (destroying real-time determinism), and they require dynamic memory allocation (heap) to instantiate the exception objects.

In C, error handling is explicit. Every error must be manually checked, deliberately handled, or consciously propagated up the call stack. 

### 1.1 The Explicit Return Code (`status_t`)
For Recoverable Faults (hardware failures, timeouts), we use an explicit return code. Returning `-1` or `0` is a legacy anti-pattern that provides no context. We must use a strongly typed enumeration.

```c
/* PRODUCTION-GRADE: The explicit status enumeration */
typedef enum {
    STATUS_OK = 0,               // Success
    STATUS_ERR_TIMEOUT = 1,      // Hardware did not respond
    STATUS_ERR_BUSY = 2,         // Hardware is currently locked
    STATUS_ERR_CHECKSUM = 3,     // Data corruption detected
    STATUS_ERR_NOT_FOUND = 4     // Requested resource does not exist
} status_t;
```

#### The `warn_unused_result` Compiler Attribute
The greatest weakness of return codes is human negligence. A developer calls `i2c_write()`, assumes it works, and ignores the return code. 

To enforce safety, we instruct the compiler to flag ignored return codes as errors. In GCC/Clang, this is done using the `__attribute__((warn_unused_result))` extension, which we wrap in a macro.

```c
/* PRODUCTION-GRADE: Forcing the caller to check the return code */

// Define the macro for portability
#if defined(__GNUC__) || defined(__clang__)
    #define MUST_CHECK_RESULT __attribute__((warn_unused_result))
#else
    #define MUST_CHECK_RESULT
#endif

// Apply it to the function prototype
MUST_CHECK_RESULT status_t eeprom_write(uint32_t addr, const uint8_t *data, uint16_t len);

void application_task(void) {
    uint8_t buffer[10] = {0};
    
    // ANTI-PATTERN: The compiler will throw an ERROR here because we ignored the result!
    // eeprom_write(0x100, buffer, 10); 
    
    // PRODUCTION-GRADE: We explicitly check and handle the fault.
    status_t err = eeprom_write(0x100, buffer, 10);
    if (err != STATUS_OK) {
        system_log_error("EEPROM write failed", err);
        // Implement retry logic or degrade functionality...
    }
}
```

### 1.2 The Failure of the Standard `assert()`
For Fatal Logic Defects (programmer errors, null pointers), we use Assertions. The C standard library provides an `<assert.h>` header. **You must never use the standard `assert()` macro in embedded systems.**

**The Deep Technical Rationale:**
The standard `assert(condition)` macro expands into a check. If the condition is false, it prints a string to `stderr` (which usually doesn't exist on a microcontroller) and calls the standard library `abort()` function. In most embedded toolchains (like newlib-nano), `abort()` simply disables interrupts and enters an infinite `while(1)` loop. 

When your remote IoT device hits a standard `assert()`, it silently freezes forever. The watchdog timer *might* reset it, but all context is lost. You have no idea what file failed, what line failed, or what the CPU registers looked like.

Furthermore, standard `assert()` is completely compiled out of the code if the `NDEBUG` macro is defined (which is standard practice for "Release" builds). Removing your safety net exactly when you ship to production is a terrifyingly dangerous paradigm.

### 1.3 Building a Robust Embedded Assert
A production-grade embedded assert must do three things:
1.  **Always Compile:** It must remain in the production firmware (unless explicitly disabled by the architect to save Flash space).
2.  **Capture Context:** It must capture the Program Counter (PC), Link Register (LR), Filename, and Line Number.
3.  **Halt Safely:** It must log the data to a non-volatile memory (e.g., Flash crash log) and trigger a controlled system reset, *not* a silent infinite loop.

#### Concrete Implementation: The Embedded Assert
This requires deep silicon knowledge. On an ARM Cortex-M, we can use compiler intrinsics to grab the Link Register (which contains the return address of the function that called the assert).

```c
/* PRODUCTION-GRADE: A robust custom Assert for ARM Cortex-M */
// file: system_assert.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

// The actual handler function. 'noreturn' tells the compiler this function never exits.
__attribute__((noreturn)) void system_assert_handler(
    const char *file, 
    uint32_t line, 
    uint32_t link_register
);

// We use __builtin_return_address(0) to get the Link Register (LR) on ARM.
// This tells us exactly which function called the macro.
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            system_assert_handler(__FILE__, __LINE__, (uint32_t)__builtin_return_address(0)); \
        } \
    } while(0)
```

The corresponding `.c` file handles the fatal fault:

```c
/* file: system_assert.c */
#include "system_assert.h"
#include "hardware_watchdog.h"
#include "crash_logger.h"

// Disable interrupts immediately. We cannot trust the system state.
static inline void disable_interrupts(void) {
    __asm volatile("cpsid i" : : : "memory");
}

__attribute__((noreturn)) void system_assert_handler(
    const char *file, 
    uint32_t line, 
    uint32_t link_register) 
{
    disable_interrupts();
    
    // 1. Write the failure to non-volatile memory (e.g., an EEPROM sector)
    // The next time the device boots, it will transmit this crash report.
    crash_log_t log = {
        .magic = 0xDEADBEEF,
        .line = line,
        .lr = link_register
    };
    // We only save the first 16 chars of the filename to save EEPROM space
    strncpy(log.filename, file, 16); 
    crash_logger_write(&log);
    
    // 2. Put the hardware into a physically safe state.
    // E.g., disable PWM outputs to stop motors from spinning out of control.
    hardware_motors_disable_all();
    
    // 3. Trigger a soft reset via the AIRCR register (ARM specific),
    // or let the watchdog expire.
    hardware_system_reset();
    
    // 4. Fallback infinite loop (should never be reached)
    while(1) { }
}
```

With this architecture, if a null pointer is passed to a function, the system safely shuts down the hardware, logs the exact line of code and the calling function's address to EEPROM, and reboots. The engineering team can download the crash log and instantly pinpoint the exact location of the logic bug.

## 2. Company Standard Rules

**Rule 12.2.1:** All hardware interactions, memory allocations, and external communications MUST return an explicit `status_t` enumeration (or equivalent strongly-typed struct) indicating success or failure. Returning boolean values for complex error states is forbidden.
**Rule 12.2.2:** Functions returning a `status_t` MUST be decorated with the compiler attribute `__attribute__((warn_unused_result))` to force callers to explicitly handle or cast the return value.
**Rule 12.2.3:** The standard library `<assert.h>` MUST NOT be used. Developers MUST use the project-specific `ASSERT()` macro.
**Rule 12.2.4:** The custom `ASSERT()` macro MUST capture the filename (`__FILE__`), line number (`__LINE__`), and the caller's address (Link Register) to provide actionable diagnostics.
**Rule 12.2.5:** Upon an assertion failure, the system MUST immediately disable interrupts, place physical actuators (motors, heaters, lasers) into a safe off-state, log the diagnostic data to non-volatile memory, and trigger a hardware reset. The system MUST NOT enter a silent infinite loop unless explicitly connected to a JTAG debugger.