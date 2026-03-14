# Chapter 14.2: Test Seams in C

To realize the "Uncompromising Separation of Concerns" mandated in the previous chapter, we must establish physical boundaries within our C code. In software engineering, these boundaries are called **Seams**. 

Michael Feathers, in his seminal work *Working Effectively with Legacy Code*, defines a seam as "a place where you can alter behavior in your program without editing in that place." For the embedded software architect, seams are the structural mechanisms that allow us to surgically amputate the hardware dependencies and substitute them with "mock" or "stub" implementations during host-based unit testing.

Because C is a procedural language rather than an object-oriented one, it lacks built-in language features like interfaces, virtual methods, or classes that make dependency injection trivial in languages like C#, Java, or C++. Consequently, embedded C engineers must deliberately and manually architect seams into the codebase. 

We employ three distinct types of seams in C, categorized by the stage of the build pipeline where the substitution occurs: Preprocessor (Compile-Time), Link-Time, and Object-Oriented (Run-Time). Understanding the deep technical implications of each is critical for maintaining a scalable 20-year architecture.

## 1. Preprocessor Seams (Compile-Time)

The most primitive form of a seam leverages the C preprocessor (`cpp`) to swap out implementations before the compiler even parses the syntax. 

```c
// Preprocessor Seam Example
void system_trigger_alarm(void) {
#ifdef ENV_HOST_TESTING
    MockAlarm_Trigger(); // Call a mock function during testing
#else
    GPIOA->BSRR = GPIO_PIN_5; // Manipulate physical hardware in production
#endif
}
```

### Technical Rationale and The "Two Codebases" Anti-Pattern
At the compiler level, the preprocessor strips away the inactive branch. When compiling for the host, the compiler *never sees* the `GPIOA->BSRR` logic. 

**Company Policy: Preprocessor seams are strictly classified as an Anti-Pattern for new development.**

Why? Because preprocessor seams create a literal split in reality. The code you test is physically different from the code you ship. If `#ifdef` blocks proliferate throughout the codebase, the cyclomatic complexity of understanding the code explodes. Furthermore, preprocessor seams destroy code readability, break static analysis tools (which often only parse one configuration at a time), and turn the source files into a tangled mess of compile-time conditionals. 

*Exception:* Preprocessor seams are temporarily permitted *only* when strangling deeply coupled legacy code (see Chapter 18 on Legacy Migration) where no other seam is immediately viable.

## 2. Link-Time Seams

Link-time seams are vastly superior to preprocessor seams. They leverage the Linker (e.g., `ld` in GCC) to substitute implementations. 

In a link-time seam, the application code calls a function based on a declared prototype (a header file), but the actual implementation of that function is resolved later by linking a different object (`.o`) file depending on the target environment.

### The Mechanism
The application module `#include`s a header defining the contract:
```c
// hardware_adc.h (The Contract)
uint16_t HW_ADC_ReadChannel(uint8_t channel);
```

The application uses it cleanly, with zero test-specific logic:
```c
// temperature_monitor.c (The Business Logic)
#include "hardware_adc.h"

float Monitor_GetTemperature(void) {
    uint16_t raw = HW_ADC_ReadChannel(1); // The Seam is the function call
    return (raw * 3.3f / 4096.0f) * 100.0f;
}
```

**The dual-target linker substitution:**
- **Target Build (Production):** The build system compiles `temperature_monitor.c` and `hardware_adc.c` (the real driver accessing MCU registers), and the linker joins them.
- **Host Build (Testing):** The build system compiles `temperature_monitor.c` and `mock_adc.c` (a test double), omitting the real hardware file entirely.

### GCC Weak Symbols (`__attribute__((weak))`)
At the ELF (Executable and Linkable Format) level, the linker resolves symbol names to memory addresses. Normally, if a linker finds two identical symbol names (e.g., two definitions of `HW_ADC_ReadChannel`), it throws a "Multiple Definition" fatal error. 

GCC and Clang provide a powerful extension: weak symbols.
```c
// In default_hardware_adc.c
__attribute__((weak)) uint16_t HW_ADC_ReadChannel(uint8_t channel) {
    // Default implementation
    return 0;
}
```
If the linker finds a "strong" symbol (your mock implementation in a test file), it silently overrides the weak symbol without an error. This allows you to compile an entire static library of hardware drivers, but easily override specific functions in your test executables. 

**Guideline for Link-Time Seams:** Link-time seams are excellent for mocking out singular, static hardware interfaces (like a global HAL). They keep the production C code 100% clean of testing logic. However, they struggle when you need to instantiate multiple different mocks of the same interface simultaneously, or if you need to dynamically change the mock behavior at runtime without recompiling.

## 3. Object-Oriented Seams (Run-Time Dependency Injection)

The most robust, flexible, and scalable architectural approach in C is the Object-Oriented Seam, also known as Run-Time Dependency Injection. This pattern utilizes `struct`s containing function pointers to emulate Object-Oriented interfaces (vtables).

### The Interface Pattern

First, we define an abstract interface. This header belongs to the Application domain. It defines what the application *needs*, not what the hardware *is*.

```c
// i_temperature_sensor.h (The Interface)
#include <stdint.h>
#include <stdbool.h>

// Opaque context pointer to hide implementation details
typedef struct temp_sensor_context temp_sensor_context_t; 

typedef struct {
    // Function pointer representing a method
    bool (*read_celsius)(temp_sensor_context_t* ctx, float* out_temp);
    
    // The instance data for the specific implementation
    temp_sensor_context_t* context; 
} i_temperature_sensor_t;
```

### Dependency Injection in the Application

The business logic no longer calls concrete global functions. Instead, it accepts a pointer to the interface. The dependency is *injected* into the application.

```c
// thermal_controller.c
#include "i_temperature_sensor.h"

static i_temperature_sensor_t* active_sensor = NULL;

// Constructor / Injector
void ThermalController_Init(i_temperature_sensor_t* sensor_interface) {
    active_sensor = sensor_interface;
}

void ThermalController_Process(void) {
    float current_temp = 0.0f;
    // The indirect function call is the Run-Time Seam
    if (active_sensor->read_celsius(active_sensor->context, &current_temp)) {
        if (current_temp > 85.0f) {
            // Trigger cooling
        }
    }
}
```

### Silicon-Level Impact: The Cost of the Indirect Branch
A common objection from embedded traditionalists is that function pointers introduce unacceptable overhead. Let us examine the silicon reality.
Calling `active_sensor->read_celsius()` generates an indirect branch instruction (e.g., `BLX` on ARM Cortex-M, where the target address is held in a register). 

This requires:
1. Loading the pointer address from RAM.
2. Executing the branch.
3. A potential pipeline flush if the branch predictor fails.

In a 16MHz 8-bit AVR microcontroller from 1998, this mattered. In a modern 100MHz+ Cortex-M33 with a multi-stage pipeline and branch prediction, the overhead of this indirect branch is typically single-digit nanoseconds. Unless this function pointer is being invoked inside a 1-microsecond motor-control ISR, the overhead is mathematically irrelevant compared to the massive architectural benefits of testability and decoupling.

### Production-Grade Mocking (CMock / FFF)
Because the interface is explicitly defined in a header, we can use industry-standard tools like **CMock** or **FFF (Fake Function Framework)** to automatically generate robust test doubles.

With FFF, testing the `ThermalController` becomes trivial:

```c
// test_thermal_controller.c (Host Unit Test)
#include "unity.h"
#include "fff.h"
#include "i_temperature_sensor.h"

DEFINE_FFF_GLOBALS;

// Generate a fake for the function pointer signature
FAKE_VALUE_FUNC(bool, fake_read_celsius, temp_sensor_context_t*, float*);

void setUp(void) {
    RESET_FAKE(fake_read_celsius);
}

// Custom fake behavior
bool mock_high_temp(temp_sensor_context_t* ctx, float* out_temp) {
    *out_temp = 100.0f;
    return true;
}

void test_ThermalController_TriggersCoolingOnHighTemp(void) {
    fake_read_celsius_fake.custom_fake = mock_high_temp;
    
    i_temperature_sensor_t mock_interface = {
        .read_celsius = fake_read_celsius,
        .context = NULL
    };
    
    ThermalController_Init(&mock_interface);
    ThermalController_Process();
    
    // Assert that the function pointer was called exactly once
    TEST_ASSERT_EQUAL(1, fake_read_celsius_fake.call_count);
    // Assert that cooling was triggered...
}
```

## Anti-Pattern: The "God Mock"
When teams first discover mocking frameworks, they often attempt to mock the entire Silicon Vendor HAL (e.g., creating a massive CMock file for `stm32f4xx_hal.h`). This is a disastrous anti-pattern. The resulting "God Mock" requires thousands of lines of configuration to simulate the exact register states expected by the HAL.
**Rule:** Mock at the architectural boundary (your custom `i_temperature_sensor_t` interface), NEVER at the vendor HAL boundary.

---

## Company Standard Rules

**Rule 14.2.1:** **Prohibition of Preprocessor Seams for New Code.** The use of `#ifdef ENV_TESTING` or similar preprocessor directives to alter code behavior for unit testing is strictly forbidden in all newly authored modules. Test variations must be handled via Link-Time or Run-Time seams.

**Rule 14.2.2:** **Run-Time Interfaces for Peripherals.** Any interaction with external peripherals (sensors, actuators, displays, external memory) must be abstracted behind an Object-Oriented Run-Time Seam (a struct of function pointers) to enable seamless dependency injection and dynamic mocking.

**Rule 14.2.3:** **Link-Time Seams for Core Silicon Features.** Link-time seams (swapping `.c` implementations via CMake/Make) are reserved exclusively for low-level, singleton silicon features that are intimately tied to the chip's core execution and do not require multiple instantiations (e.g., the core millisecond tick timer, global critical section locks, watchdog feeding).

**Rule 14.2.4:** **No Vendor HAL Mocking.** Developers shall not use automated mocking tools (CMock/FFF) to generate mocks of silicon vendor SDK headers. Mocks shall only be generated for internally owned, hardware-agnostic interface headers defined in the `hal/include` layer.