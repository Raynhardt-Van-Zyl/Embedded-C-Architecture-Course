# Chapter 16.1: What the Framework Should Control

To create a cohesive, scalable, and maintainable embedded software ecosystem within an organization, certain foundational elements must be rigidly controlled by an internal architectural framework. If every team (or worse, every individual developer) is allowed to invent their own paradigms for managing memory, handling hardware errors, or logging debug data, the codebase devolves into a fragmented, unintegratable mess. 

When we declare that the framework "controls" these elements, we mean the architecture provides standardized APIs, mandatory design patterns, and strictly enforced CI rules regarding their use. The goal is not to micromanage application logic, but to provide a robust, standardized foundation so that developers can focus entirely on solving business problems rather than debugging memory corruption or inconsistent UART drivers.

Here are the six core domains your embedded framework *must* completely control.

## 1. Error Handling and Assertions

In a fragmented legacy codebase, error handling is typically chaotic. One developer might return `-1` to indicate a failure. Another might return a boolean `false`. A third might set a global `errno` variable (which is notoriously non-thread-safe in embedded systems). A fourth might simply enter an infinite `while(1)` loop when an I2C transaction times out. This makes integrating disparate modules a nightmare of defensive programming.

Your framework must dictate a universal, unified error-handling strategy.

### Standardized Return Types
Adopt a universal standard `enum` for error codes, and mandate that all public APIs capable of failing must return this type.

```c
// company_errors.h
typedef enum {
    SYS_OK = 0,
    SYS_ERR_INVALID_ARG = 1,
    SYS_ERR_TIMEOUT = 2,
    SYS_ERR_NO_MEMORY = 3,
    SYS_ERR_HW_FAULT = 4,
    SYS_ERR_BUSY = 5
} sys_err_t;

// MANDATORY PATTERN: Return error code, pass data via pointer
sys_err_t i2c_sensor_read_register(uint8_t reg_addr, uint8_t* out_data);
```

### Assertions (Design by Contract)
The framework must provide a standardized assertion macro. Standard C `<assert.h>` usually calls `abort()`, which is undefined on bare-metal systems. The framework must dictate exactly what happens when a developer's contract assumption fails (e.g., logging the file/line to non-volatile EEPROM, executing a safe-state hardware shutdown, and triggering a watchdog reset).

```c
// company_assert.h
#define SYS_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            sys_fault_handler(__FILE__, __LINE__); \
        } \
    } while(0)
```

## 2. Memory Management

Dynamic memory allocation (`malloc` and `free`) is a notorious source of catastrophic bugs—fragmentation, memory leaks, and non-deterministic execution times—in long-running embedded systems. 

**The Rule:** The framework must dictate how memory is managed. If the company standard prohibits dynamic allocation after the boot initialization phase (a common safety-critical requirement), the framework must provide standard static allocation patterns.
This includes providing verified implementations of Memory Pools, Fixed-Size Block Allocators, and Ring Buffers. 

If an RTOS (like FreeRTOS) is used, the framework should control how task stacks and message queues are allocated, strictly enforcing static allocation APIs (e.g., wrapping `xTaskCreateStatic` rather than `xTaskCreate`).

## 3. Logging and Telemetry

Every embedded project requires a mechanism to log debug information. However, allowing developers to sprinkle raw `printf()` calls throughout the codebase is a devastating anti-pattern. 

### The `printf` Priority Inversion
Standard `printf()` is often blocking, non-reentrant, and heavily pollutes the application code with string formatting logic. If a high-priority motor control task calls `printf("Motor Speed: %d\n", speed)`, and the underlying UART driver blocks while transmitting those 20 bytes at 9600 baud, the high-priority task is blocked for 20 milliseconds. This causes a massive priority inversion and destroys real-time deadlines.

The framework must provide an asynchronous, standardized logging API:

```c
// company_log.h
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_ERROR 2

// Framework macro injects module name and routes to an async ring buffer
#define LOG_ERROR(fmt, ...) sys_log_write(LOG_LEVEL_ERROR, MODULE_NAME, fmt, ##__VA_ARGS__)
```
This ensures that all logs have a consistent format (Timestamp, Level, Thread ID, Module Name) and are pushed to a lock-free ring buffer, where a low-priority background task eventually flushes them to the physical backend (UART, SWO, Flash, Ethernet).

## 4. Hardware Abstraction Layer (HAL) Interfaces

If your company manufactures products that might need to migrate from an STM32 to an NXP microcontroller due to silicon shortages or cost optimizations, the framework must define the abstract *interfaces* for all common peripherals (GPIO, SPI, I2C, UART, ADC, PWM).

The framework does not necessarily provide the concrete implementation for every chip immediately, but it absolutely controls the *contract*.

```c
// company_spi_interface.h
// This contract is owned by the Framework, not the Silicon Vendor.
typedef struct spi_interface spi_interface_t;

struct spi_interface {
    sys_err_t (*init)(spi_interface_t* self);
    sys_err_t (*transmit)(spi_interface_t* self, const uint8_t* tx_data, size_t len);
    sys_err_t (*receive)(spi_interface_t* self, uint8_t* rx_data, size_t len);
    void* hw_context; // Opaque pointer to ST/NXP specific instance data
};
```
By forcing all application logic to depend on `company_spi_interface.h` rather than `stm32_hal_spi.h`, the framework guarantees that business logic remains entirely decoupled from silicon vendors.

## 5. RTOS Wrapping and Concurrency Primitives

If your systems utilize a Real-Time Operating System, allowing raw RTOS calls (like `xSemaphoreTake` or `osDelay`) to be scattered throughout application code tightly couples your logic to that specific OS. 

More dangerously, developers left to their own devices will misuse concurrency primitives. The framework must provide an OS Abstraction Layer (OSAL) and standardize *how* concurrency is applied.

The framework dictates the rules of engagement:
*   "Mutexes may only be used to protect shared hardware resources (like an I2C bus), never to protect application state variables."
*   "Task-to-task communication must utilize asynchronous message queues, absolutely never shared global variables."

## 6. Startup and Initialization Sequences

The "Wild West" approach to embedded initialization allows developers to initialize their modules in whatever order they please inside `main()`. This invariably leads to subtle, impossible-to-debug initialization dependency bugs (e.g., trying to log an error via UART before the system clock tree has been configured, resulting in a hard fault).

The framework must completely control the boot sequence. It should define rigid, non-negotiable phases:
1.  **Low-Level Hardware Init:** PLL Clocks, Flash wait states, hardware watchdog.
2.  **Framework Init:** Memory pools, async logging system, OSAL initialization.
3.  **Peripheral Init (BSP):** Concrete HAL implementations for SPI/I2C buses.
4.  **Application Init:** Spawning RTOS tasks, initializing state machines.
5.  **Scheduler Start:** Handing control over to the RTOS.

By standardizing these six pillars, the framework provides a robust, predictable safety net. It eliminates entire classes of concurrency and memory bugs, allowing the engineering team to focus on developing the actual product.

---

## Company Standard Rules

**Rule 16.1.1:** **Unified Error Handling.** All public API functions that perform an operation capable of failing (hardware interaction, memory allocation, timeout) must return the standardized `sys_err_t` enumeration. Returning generic integers or booleans for error status is prohibited.

**Rule 16.1.2:** **Prohibition of Raw `printf`.** The use of standard C library `printf`, `sprintf`, or `puts` is strictly forbidden in production code. All telemetry, debugging, and system logging must be routed through the framework's asynchronous `sys_log` API.

**Rule 16.1.3:** **Interface Ownership.** Silicon vendor HAL headers (e.g., ST, NXP, Microchip) shall not be exposed to the application layer. The application layer must interface with hardware exclusively through the framework-defined, object-oriented abstraction structs (e.g., `spi_interface_t`).

**Rule 16.1.4:** **Strict Boot Phasing.** The initialization sequence within `main()` must strictly adhere to the framework's defined phased boot process. Modules are forbidden from implicitly relying on the initialization state of other modules outside of their designated phase.