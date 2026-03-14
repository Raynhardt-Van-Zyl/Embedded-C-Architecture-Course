# 2.2 Responsibilities of Each Layer: Segregating Concerns

## The Importance of Segregation

To maintain a healthy architecture over decades, every layer must have a strictly defined, singular set of responsibilities. When production bugs occur, or new features are requested, it should be immediately obvious to any engineer *which layer* should house the new code. If a module seems to span multiple layers—for instance, a UART driver that also parses JSON payloads—it is poorly designed, violates the Single Responsibility Principle, and must be refactored.

Let's dissect the exact responsibilities—and explicitly the *non-responsibilities*—of each tier to establish clear boundaries.

### 1. Layer 3: The Application Layer

The Application Layer is the brain. It is the only part of the codebase that actually cares about the product you are building. It is pure math, logic, and state management.

**Responsibilities:**
- Implementing the core business rules of the product (e.g., "If the safety interlock button is released while the motor is spinning faster than 1000 RPM, engage the emergency brake").
- High-level state machine management (e.g., Booting, Idle, Active, Fault states).
- Complex mathematical processing, digital signal processing (DSP), and control theory algorithms (e.g., PID controllers, Kalman filters).
- Orchestrating the interaction between various Layer 2 Services (e.g., commanding the Network Service to transmit data read from the Sensor Service).

**What it MUST NOT do:**
- Configure MCU clock trees, initialize peripheral registers, or setup DMA channels.
- Implement low-level communication protocols (like bit-banging an I2C or 1-Wire protocol).
- Contain `#ifdef STM32` or `#if defined(NRF52)` macros. If the Application layer needs conditional compilation based on the silicon, the HAL layer has failed its job.

### 2. Layer 2: The Services / Middleware Layer

The Services Layer is the nervous system. It provides complex, highly capable, but fundamentally hardware-independent utilities to the application. It acts as the bridge between raw bytes and meaningful data structures.

**Responsibilities:**
- Providing complex operational capabilities.
- **OS Abstraction (OSAL):** Wrapping RTOS primitives (tasks, mutexes, queues) so the application isn't tightly coupled to FreeRTOS or ThreadX APIs.
- **Storage Services:** Implementing wear-leveling algorithms, key-value stores (NVS), or FAT file systems over raw Flash memory.
- **Protocol & Network Stacks:** Assembling/Parsing packets, handling retries, managing MQTT/HTTP connections, parsing JSON/Protobufs.
- **External Peripheral Drivers:** Drivers for external ICs connected via buses (e.g., an external Display driver over SPI, or an EEPROM driver over I2C).

**What it MUST NOT do:**
- Contain application-specific business logic (e.g., A generic MQTT service should format and send messages; it should not decide *when* a high-temperature alarm message needs to be sent).
- Directly access MCU registers. It must use the Layer 1 HAL to talk to the outside world.

### 3. Layer 1: The Hardware Abstraction Layer (HAL)

The HAL is the translator. It abstracts the infinite complexity and variation of microcontroller internal peripherals into a standardized, company-wide C API.

**Responsibilities:**
- Providing a uniform API for the MCU's internal peripherals (UART, SPI, I2C, ADC, GPIO, Timers, DMA).
- Abstracting away the physical reality of the silicon into logical concepts (e.g., an API `Hal_Gpio_SetHigh(LED_RED)` abstracts away the reality that the LED is connected to `GPIOA, PIN5`).
- Handling low-level hardware interrupts, clearing physical interrupt flags, and safely deferring the data payload to higher layers via Queues or callbacks, without blocking.

**What it MUST NOT do:**
- Do not put application logic or data interpretation in the HAL.

#### ❌ Anti-Pattern: The Smart HAL (Protocol Parsing in the Interrupt)

A common architectural disaster is putting protocol logic inside a Layer 1 hardware driver.

```c
// ANTI-PATTERN: HAL parsing application protocols
void UART1_IRQHandler(void) {
    uint8_t byte = USART1->DR;
    USART1->SR &= ~USART_SR_RXNE; // Clear flag
    
    // FATAL ARCHITECTURAL FLAW: Layer 1 is parsing Layer 3 application data!
    if (byte == '{') {
        g_json_started = true;
    } else if (byte == '}' && g_json_started) {
        ProcessJsonPayload(g_buffer); // Calling Layer 3 from Layer 1!
    }
}
```
**Rationale:** The UART HAL is no longer a generic UART driver. It is now a JSON parser specifically built for one application. You can never reuse this UART driver in a project that uses Protobufs or raw binary data. Furthermore, `ProcessJsonPayload` is likely a slow, blocking function executing inside an Interrupt Service Routine, destroying real-time performance.

### ✅ Good Pattern: External Component Driver Allocation

Where does a driver for an external I2C Temperature Sensor (e.g., the Texas Instruments TMP102) belong?

**Wrong:** Putting it in Layer 3 (Application) clutters pure business logic with I2C register maps and hex addresses.
**Wrong:** Putting it in Layer 1 (HAL) violates the definition of the HAL, which is strictly for abstracting *internal* MCU silicon.

**Right:** It belongs in **Layer 2 (Services)**, often in a dedicated `devices/` or `components/` sub-folder. It acts as a Service that the Application consumes, and it relies on the HAL to communicate.

```c
// GOOD: TMP102 Driver (Layer 2: Services)
#include "tmp102.h"
#include "hal_i2c.h" // Depends strictly on Layer 1 Firewall

// The Service knows the specific register map of the external chip
#define TMP102_REG_TEMP 0x00
#define TMP102_I2C_ADDR 0x48

// Returns a meaningful float to the Application (Layer 3)
SysStatus_t TMP102_ReadTemperature(I2C_Handle_t* i2c_bus, float* out_temp) {
    uint8_t buffer[2] = {0};
    
    // Uses the generic HAL. Doesn't care if the underlying MCU is ST, NXP, or TI.
    SysStatus_t status = Hal_I2c_ReadRegister(i2c_bus, TMP102_I2C_ADDR, TMP102_REG_TEMP, buffer, 2);
    
    if (status == SYS_OK) {
        // Service Layer translates raw bytes into application-meaningful data
        int16_t raw_temp = (buffer[0] << 4) | (buffer[1] >> 4);
        *out_temp = (float)raw_temp * 0.0625f;
    }
    
    return status;
}
```

### Company Standards & Rules: Layer Responsibilities

1. **The "Data Agnostic" Rule for HAL:** Layer 1 HAL APIs MUST be completely agnostic to the *meaning* of the data they are transferring. A UART HAL function simply moves raw bytes into a ring buffer; it MUST NOT parse command strings, search for delimiters, or interpret JSON. All protocol parsing belongs in Layer 2 or Layer 3.
2. **The "External Component" Rule:** Drivers for any IC external to the main microcontroller (e.g., sensors, memory chips, displays connected via SPI/I2C/UART) MUST reside in the Services Layer (Layer 2). They MUST utilize the internal MCU HAL (Layer 1) for communication.
3. **OS Abstraction (OSAL) Mandate:** The Application Layer (Layer 3) SHOULD NOT interact directly with vendor-specific RTOS APIs (like `xQueueSend` or `vTaskDelay` in FreeRTOS). It MUST use an OS Abstraction Layer (e.g., `Osal_Queue_Send`, `Osal_Thread_Sleep`) defined in the Services Layer to ensure the RTOS can be swapped or simulated on a PC without modifying business logic.
4. **The "No Logic in ISR" Rule:** Layer 1 Interrupt Service Routines (ISRs) MUST NOT perform data processing, parsing, or floating-point math. They MUST ONLY read/write hardware registers, clear interrupt flags, and immediately defer the data payload to a higher layer via a non-blocking queue or semaphore.