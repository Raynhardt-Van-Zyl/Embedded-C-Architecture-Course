# 18.3 Governance and exceptions

No framework is perfect, and no set of architectural rules can cover 100% of real-world embedded scenarios. Hardware has flaws, physics imposes strict latency constraints, and silicon vendors sometimes provide SDKs that actively fight against clean architectural principles.

If the framework is treated as a dogmatic religion where rules can never be broken, developers will be forced to write horribly inefficient code just to appease the CI pipeline.

To survive, a standard architecture requires **Governance** (a process for managing the rules) and a defined path for **Exceptions** (a safe way to break the rules).

## The Architecture Review Board (ARB)

Governance should not be handled by a single dictator. It should be managed by an Architecture Review Board (ARB) or a Technical Steering Committee.

The ARB is typically a small group (3-5 people) consisting of senior engineers, tech leads, and the chief architect. 
Their responsibilities include:
1.  **Reviewing Proposals:** When a team wants to add a new core module to the framework (e.g., standardizing a filesystem interface), the ARB reviews the design to ensure it aligns with the overall philosophy.
2.  **Updating the Rules:** As the company adopts new tools (e.g., migrating from C99 to C11, or adding Rust to the stack), the ARB updates the official standards and CI configurations.
3.  **Granting Exceptions:** This is their most crucial role.

## Handling Exceptions Safely

When an engineer encounters a situation where adhering to the framework is impossible or highly detrimental, they must request an exception. 

**Example Scenario:**
*The Rule:* "Application code in `src/app/` may never include hardware headers from `src/bsp/`. It must use the HAL interface."
*The Problem:* The application involves an incredibly tight 1MHz control loop for a power supply. Calling through the HAL's function pointers (`inst->adc->read()`) adds 15 clock cycles of overhead per reading, causing the control loop to miss its deadline. Direct register access (`ADC1->DR`) is mandatory for the product to function.

If the developer just hacks the include paths to make the build work, the architecture erodes.

### The Exception Process

1.  **Documentation of Need:** The developer writes a brief document outlining the problem, the required rule break, and why standard workarounds (like moving the logic to an ISR) won't work.
2.  **ARB Review:** The ARB reviews the request. They might suggest an alternative architectural pattern. If no alternative exists, they approve the exception.
3.  **Quarantine and Comments:** The exception must be highly localized and heavily documented.

```c
// power_control_loop.c (Inside src/app)

#include "power_control.h"

// =========================================================================
// ARCHITECTURAL EXCEPTION: ARB_TICKET_402
// Reason: 1MHz loop requirement precludes the use of standard HAL interfaces.
// This file is permitted to directly include STM32 registers.
// This module is no longer hardware-agnostic and cannot run on host PC tests.
// =========================================================================
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include "stm32g4xx.h" // Violation!
#pragma GCC diagnostic pop

void fast_control_loop() {
    // Direct register access for speed
    uint32_t raw_val = ADC1->DR;
    // ... math ...
    TIM1->CCR1 = result;
}
```

4.  **CI Override:** The build system must have a mechanism to allow specific, approved files to bypass the dependency checks. For example, adding `power_control_loop.c` to an `exception_list.txt` file read by the CI linter.

## The Danger of Exceptions

Exceptions are a necessary evil, but they must be painful to get. If the ARB rubber-stamps every exception request, the framework is dead.

Exceptions should be treated as technical debt. If you find that 50% of your projects require an exception to the HAL rules, it means your HAL is poorly designed and needs to be rewritten, not that your developers are lazy.

By implementing a formal governance and exception process, you create an architecture that is rigid enough to maintain order, but flexible enough to survive the brutal realities of embedded systems engineering.