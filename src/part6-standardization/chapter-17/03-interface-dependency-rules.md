# 17.3 Interface and dependency rules

Spaghetti code in C usually isn't caused by complex `while` loops or deeply nested `if` statements. It is caused by uncontrolled dependencies. When Module A directly calls a specific function in Module B, they become tightly coupled. If Module B is a hardware driver, Module A is now hopelessly tied to that specific hardware.

To achieve a modular, testable embedded architecture, the framework must enforce strict rules regarding how modules interface with one another.

## Rule 1: The Dependency Inversion Principle (DIP)

High-level modules (application logic) must not depend on low-level modules (hardware drivers or OS primitives). Both should depend on abstractions (interfaces).

In C, an abstraction is realized as a structure of function pointers. The framework must mandate that application code uses these interfaces rather than concrete includes.

**Anti-Pattern (Tightly Coupled):**
```c
// pid_controller.c
#include "stm32f4_pwm.h" // BAD: Direct hardware dependency

void pid_update() {
    float output = calculate_pid();
    stm32f4_pwm_set_duty_cycle(TIMER_2, output); // Tightly bound to STM32
}
```

**Framework Standard (Loosely Coupled):**
```c
// pid_controller.h
#include "pwm_interface.h" // GOOD: Abstraction dependency

// pid_controller.c
void pid_update(pid_instance_t* inst) {
    float output = calculate_pid();
    // Hardware agnostic. Could be STM32, ESP32, or a Mock in a unit test.
    inst->pwm_interface->set_duty_cycle(inst->pwm_channel, output); 
}
```

## Rule 2: Constructor Injection

If an application module depends on a hardware interface, how does it get access to the actual hardware implementation? The framework must enforce **Dependency Injection**, specifically via the module's initialization function (constructor injection).

Global access to hardware resources (e.g., `extern I2C_HandleTypeDef hi2c1;`) must be strictly banned.

**Framework Standard:**
Dependencies are passed explicitly through a configuration struct during `init()`.

```c
// system_main.c (The Composition Root)
#include "bsp_stm32_i2c.h"
#include "app_sensor.h"

void system_init() {
    // 1. Initialize low-level concrete hardware
    i2c_interface_t* stm32_i2c = bsp_i2c_init(I2C_PORT_1);

    // 2. Inject the concrete dependency into the high-level application module
    sensor_config_t config = {
        .i2c_bus = stm32_i2c  // Injection happens here
    };
    
    sensor_t my_sensor;
    sensor_init(&my_sensor, &config);
}
```
This rule guarantees that modules are context-agnostic. The `sensor` module has no idea it is running on an STM32; it just knows it has an I2C bus.

## Rule 3: No Circular Dependencies

A circular dependency occurs when Header A includes Header B, and Header B includes Header A. This usually causes horrific compile-time errors or requires messy pre-processor hacks. More importantly, it signals a major architectural flaw: the modules are inherently fused together and cannot be tested independently.

The framework must dictate that dependencies flow in one direction:
`Application -> Middleware -> HAL -> BSP`

If Module X needs to send data to Module Y, but Module Y also needs to trigger an event in Module X, you must use **Callbacks (Observer Pattern)** to break the cycle.

**Breaking cycles with Callbacks:**
Module X provides a way for Module Y to register a function pointer.

```c
// module_x.h
typedef void (*event_callback_t)(uint32_t event_id);

void module_x_register_callback(event_callback_t cb);

// module_y.c
#include "module_x.h"

static void my_event_handler(uint32_t event_id) {
    // Handle event from X without X knowing about Y
}

void module_y_init() {
    module_x_register_callback(my_event_handler);
}
```
Here, X knows nothing about Y. The dependency points from Y to X, breaking the circle.

## Summary of Dependency Rules
1.  **Never include BSP or SDK headers in Application code.**
2.  **Use structs of function pointers for hardware and OS abstraction.**
3.  **Pass dependencies via initialization structs; ban `extern` singletons.**
4.  **Break circular dependencies using Callbacks/Observer patterns.**

Enforcing these rules guarantees an architecture that is highly testable (via mocking) and easily portable to new microcontrollers.