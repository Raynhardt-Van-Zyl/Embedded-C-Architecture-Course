# Writing Framework V1

Armed with the assessment of the legacy codebase, it is time to write Version 1 of the internal framework. The critical mindset here is constraint. You are not building the ultimate, omnipotent framework. You are building **Framework V1**: the smallest set of standardized tools and rules necessary to begin untangling the legacy code.

## The YAGNI Anti-Pattern

**YAGNI** stands for "You Aren't Gonna Need It."

### Anti-Pattern Example
Building a complex, thread-safe, priority-queued, dynamically allocated logging framework with TCP/IP forwarding... when the current product is a bare-metal smart thermostat that only outputs via a single UART debug port.

**Rationale:** Over-engineering Framework V1 delays its deployment, frustrates developers who have to learn a massive API for simple tasks, and often results in abstractions that don't actually fit the hardware constraints once deployed.

## Core Pillars of Framework V1

Framework V1 should focus almost entirely on standardizing **Communication, Errors, and Core Types**.

### 1. Standardized Error Handling

Legacy codebases often have a dozen different ways of reporting errors. Framework V1 must unify this.

```c
// fw_errors.h
// A single, unified result type used by EVERY function in the framework.
typedef enum {
    FW_OK = 0,
    FW_ERR_TIMEOUT,
    FW_ERR_INVALID_PARAM,
    FW_ERR_NO_MEMORY,
    FW_ERR_HW_FAULT,
    // Add domain-specific errors sparsely
} fw_result_t;

// Macro to easily bubble up errors without deep nesting
#define FW_RETURN_IF_ERROR(expr) \
    do { \
        fw_result_t __res = (expr); \
        if (__res != FW_OK) { return __res; } \
    } while(0)
```

### 2. Standardized Basic Types

Never rely on standard C types (`int`, `char`) in an embedded framework, as their sizes vary between compilers and architectures. Enforce the use of `stdint.h` and `stdbool.h`.

```c
// fw_types.h
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// If specific platform macros are needed, define them here.
```

### 3. The Core Logging Utility

Logging is essential for debugging, but it must be decoupled from the physical transport (UART, RTT, USB).

```c
// fw_log.h
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} fw_log_level_t;

// Function pointer for the backend transport
typedef void (*fw_log_backend_fn)(const char *msg, size_t len);

// Framework initialization
void fw_log_init(fw_log_backend_fn backend, fw_log_level_t min_level);

// Standardized Macros
#define FW_LOG_INFO(fmt, ...) fw_log_printf(LOG_LEVEL_INFO, "INFO: " fmt "\n", ##__VA_ARGS__)
```

### 4. Hardware Interfaces (HAL V1)

Instead of rewriting the entire hardware layer, define the interfaces for the 1 or 2 peripherals identified as "Seams" during the assessment (e.g., I2C or SPI).

Use the Dependency Inversion pattern discussed in Chapter 19 to define `fw_spi_interface_t` and `fw_i2c_interface_t`.

## Architectural Rules for Framework V1

1. **Rule of Uniform Error Returns:** Every function in the framework that can fail MUST return a `fw_result_t`. Output data must be passed back via pointers in the arguments.
2. **Rule of the Transport Agnostic:** No core framework utility (Logging, Assertions, Event Queues) is permitted to directly call a hardware driver. They must rely on injected function pointers (backends).
3. **Rule of Static Allocation First:** Framework V1 must be designed such that it can operate entirely without `malloc()`. If dynamic behavior is required, use static memory pools provided by the application layer.