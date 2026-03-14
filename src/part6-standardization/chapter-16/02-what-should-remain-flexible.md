# 16.2 What should remain flexible

While a strong framework enforces rules on foundational elements like logging, error handling, and hardware abstraction, an overzealous framework can paralyze a development team. If an architecture tries to control every single aspect of software design, it transforms from a helpful standard into a bureaucratic bottleneck.

To maintain developer velocity and allow for project-specific optimization, certain areas of the codebase must remain entirely flexible. The framework should provide *guidance* for these areas, but never *strict enforcement*.

## 1. Application-Specific Business Logic

The framework's primary job is to deal with the realities of the hardware and the operating system so that the application logic doesn't have to. Therefore, the application layer should be a sandbox where developers have the freedom to implement algorithms, math, and business rules in the way that best fits the product.

**Anti-Pattern: The God-Framework State Machine**
Avoid forcing all applications to use a complex, centrally defined state-machine engine or event-driven framework (unless that is the specific core competency of your system, like in some automotive architectures). 

If a simple PID controller or a linear sequence of operations suffices for a specific module, developers shouldn't be forced to register event callbacks and allocate framework-specific message structures just to toggle a GPIO based on a sensor reading.

*Rule:* The framework provides the *inputs* (sensor HAL) and *outputs* (actuator HAL), but leaves the "brain" (the algorithm) to the developer.

## 2. Specific Peripheral Optimizations (The "Escape Hatch")

While a generic `company_spi_interface_t` is fantastic for 90% of use cases (e.g., reading a temperature sensor or writing to an EEPROM), it will inevitably fail when a project requires bleeding-edge performance.

Imagine a project driving a high-resolution TFT display over SPI using complex, chained DMA descriptors linked to timer hardware triggers. A standard, generic, blocking `sys_spi_transmit()` function is useless here.

**The Escape Hatch Principle:**
The framework must allow developers to bypass the standard HAL and write highly optimized, bare-metal peripheral code *when mathematically necessary*. 

*Rule:* If a developer needs to bypass the framework, they must isolate the optimized code into a specific, well-documented module, and it must still adhere to the overarching error handling and logging standards. Do not prevent them from accessing hardware registers directly if the product's performance constraints demand it.

## 3. Third-Party Library Integration

Embedded systems often rely on massive third-party stacks: LwIP for networking, FatFS for file systems, or custom crypto libraries. 

A common mistake architects make is trying to write complete "wrapper" layers around these massive libraries to make them look like "company standard" code. 

**Anti-Pattern: The Infinite Wrapper**
Writing a `sys_socket()` wrapper around LwIP's `lwip_socket()` just to conform to the company's `sys_err_t` return types is usually a massive waste of time. It introduces a maintenance burden (you must update the wrapper every time LwIP updates) and hides standard documentation from developers who already know the standard LwIP APIs.

*Rule:* Provide standard initialization routines for third-party stacks (e.g., `company_network_init()`), but let developers use the third-party API directly for the application logic.

## 4. Local Module Internals

The framework dictates the *public* face of a module (how it interacts with the rest of the system via standard headers, error codes, and dependency injection). However, the framework should not dictate the *private* internals of how a module achieves its goals.

*   **Allowed Flexibility:** 
    *   Naming of `static` private helper functions.
    *   Choice of internal data structures (e.g., choosing an array vs. a linked list for internal state tracking).
    *   Whether to use `switch` statements or function pointer arrays for internal state machines.

Trying to regulate the inside of a `.c` file to an extreme degree leads to endless arguments in pull requests over stylistic minutiae rather than architectural substance.

## Summary: The Boundary of Abstraction

A successful embedded architecture acts as an **exoskeleton**, not a straitjacket. It provides strength, support, and structure to the vulnerable parts of the system (concurrency, hardware interfaces, memory). But inside that exoskeleton, the application logic must be free to move, adapt, and solve the specific business problems of the product.