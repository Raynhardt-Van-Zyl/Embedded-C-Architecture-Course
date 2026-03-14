# 13.1 What Makes Code Reviewable

## 1. The Human Element of Code Quality
Writing C code that compiles without warnings and passes unit tests is only half the job of a software engineer. The other half is writing code that another human being can confidently verify is correct without having to spend hours deciphering it.

The purpose of a Code Review (or Pull Request) is not merely to catch bugs. Tools like `clang-tidy` and `cppcheck` are far superior at catching syntax errors, memory leaks, and missing switch cases than any human reviewer. The true purpose of a Code Review is to verify the **Architecture**, the **Domain Logic**, and the **Hardware Interactions**.

If a reviewer has to spend 15 minutes figuring out *what* a 200-line function is doing, they will have zero mental energy left to figure out *why* it is doing it, or to spot the subtle race condition hiding on line 140. Code reviewability is about minimizing cognitive load.

### 1.1 The Curse of the "God Function"
The most prominent enemy of reviewability is the "God Function"—a massive block of procedural C code that initializes a peripheral, reads data, performs mathematical scaling, formats a network packet, and sends it over UART, all within a single 500-line `while` loop.

When a reviewer opens a Pull Request and sees a 500-line function, their eyes glaze over. They skim the code, assume the author tested it, and hit "Approve." This is known as the "Looks Good To Me" (LGTM) anti-pattern. The code is unreviewable.

**The Deep Technical Rationale:**
The human brain can only hold about 7 (plus or minus 2) pieces of information in its working memory at one time. A God Function forces the reviewer to hold the state of 20 different local variables, 5 nested `if` statements, and 3 hardware register states simultaneously. It is an impossible cognitive task.

### 1.2 The Single Responsibility Principle (SRP) in C
To make code reviewable, we must aggressively apply the Single Responsibility Principle. A function should do exactly one thing, and its name should describe exactly what it does.

If you have a function named `process_sensor_and_transmit()`, it is already violating SRP. It should be split into `read_sensor_data()`, `scale_sensor_value()`, and `transmit_telemetry()`.

#### Concrete Anti-Pattern: Unreviewable Spagetti
```c
/* ANTI-PATTERN: High cognitive load. Unreviewable. */
#include "adc_driver.h"
#include "uart_driver.h"

void handle_system(void) {
    uint16_t raw = adc_read(0);
    // Magic numbers. What is 3300? What is 4095?
    float voltage = (raw * 3300.0f) / 4095.0f; 
    
    // What is 1.5? Why are we checking it?
    if (voltage > 1.5f) {
        // We are formatting a string inline with the math.
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "ALARM:%f\n", voltage);
        
        // Wait, why are we disabling interrupts just to send a UART string?
        __disable_irq(); 
        for(int i=0; i < strlen(buffer); i++) {
            while (!(UART1->SR & (1<<7))); 
            UART1->DR = buffer[i];
        }
        __enable_irq();
    }
}
```

This code is a nightmare to review. The reviewer has to verify the ADC math, understand the alarm threshold logic, and audit a raw hardware UART polling loop complete with direct register manipulation and global interrupt disabling—all in one place.

#### The Production-Grade Solution: Decomposed and Reviewable
```c
/* PRODUCTION-GRADE: Low cognitive load. Easily reviewable. */
#include "adc_driver.h"
#include "uart_driver.h"
#include "system_alerts.h"

// Constants give names to magic numbers.
static const float ADC_VREF_MV = 3300.0f;
static const float ADC_MAX_TICKS = 4095.0f;
static const float VOLTAGE_ALARM_THRESHOLD_MV = 1500.0f; // 1.5V

// 1. A pure mathematical function. Easy to unit test.
static float convert_ticks_to_mv(uint16_t ticks) {
    return (ticks * ADC_VREF_MV) / ADC_MAX_TICKS;
}

// 2. The business logic. Easy to read.
void monitor_voltage_task(void) {
    uint16_t raw_ticks = adc_read_channel(ADC_CHANNEL_0);
    float voltage_mv = convert_ticks_to_mv(raw_ticks);
    
    if (voltage_mv > VOLTAGE_ALARM_THRESHOLD_MV) {
        // The implementation details of the UART transmission and 
        // interrupt safety are hidden inside this well-named function.
        // The reviewer doesn't need to look at it unless they are specifically
        // reviewing the alert module.
        system_alerts_transmit_alarm(voltage_mv);
    }
}
```

The refactored code reads like plain English. The reviewer can instantly verify the business logic (`if voltage > threshold, transmit alarm`) without getting bogged down in how the UART data register (`DR`) works.

### 1.3 Self-Documenting Code vs. Comments
Another major barrier to reviewability is the over-reliance on comments to explain bad code. 

If you write `// Multiply by 100 to convert to centimeters` above a line of code, the comment is a failure. It is compensating for a poorly named variable. Comments should only explain *why* something is done, never *what* is being done. The code itself must explain the *what*.

*   **Bad:** `uint32_t delay = 50; // Delay for 50 milliseconds`
*   **Good:** `uint32_t delay_ms = 50;` (The unit is in the variable name. No comment needed.)

**The "Why" Comment (The Only Good Comment):**
A reviewer needs to know the historical or physical context that the C compiler cannot express.
```c
/* Good Comment: Explains the WHY. */
// The datasheet for the BME280 sensor (Page 14) states that the chip requires a 
// 2ms delay after a soft-reset command before it will respond to I2C traffic.
system_delay_ms(2);
i2c_write(BME280_ADDR, CONFIG_REG, 0x01);
```

## 2. Company Standard Rules

**Rule 13.1.1:** Functions MUST adhere to the Single Responsibility Principle. A function that calculates data MUST NOT also transmit that data. A function that reads from hardware MUST NOT also parse the business logic of that data.
**Rule 13.1.2:** Magic numbers (unnamed numerical literals) are strictly forbidden in the executable code logic. All constants MUST be defined using `static const` variables or enumerations with descriptive names. (e.g., `if (speed > MAX_SPEED_RPM)` instead of `if (speed > 4000)`).
**Rule 13.1.3:** Variables representing physical quantities or time MUST include their unit of measurement as a suffix in the variable name (e.g., `timeout_ms`, `current_ma`, `pressure_kpa`).
**Rule 13.1.4:** Comments MUST be restricted to explaining the "Why" (intent, hardware quirks, datasheet references, algorithm choices). Comments that explain the "What" (translating C syntax into English) MUST be deleted, and the code MUST be refactored to be self-documenting.
**Rule 13.1.5:** Pull Requests exceeding 400 lines of changed code (excluding auto-generated files or vendor HAL updates) MUST be rejected by the reviewer and broken down into smaller, logically cohesive commits to prevent cognitive overload.