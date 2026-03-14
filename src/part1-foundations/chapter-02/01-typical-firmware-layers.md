# 2.1 Typical Firmware Layers: The Architectural Blueprint

## The Layered Architecture Blueprint

The most universally accepted and battle-tested architectural pattern in safety-critical embedded systems is the **Layered Architecture**. However, the theoretical concept of "layers" is often poorly executed in practice, resulting in a codebase where layers bleed into one another until the entire system resembles a monolithic ball of mud. 

The fundamental, non-negotiable law of a layered architecture is the **Strict Dependency Rule**: A layer is only allowed to depend on (i.e., call functions from, or include headers of) the layer immediately below it, or in some specific relaxed architectures, any layer below it. Dependencies MUST NEVER point upwards, and they MUST NEVER bypass an abstraction layer to reach the raw hardware.

When designing a codebase to survive 20 years of changing requirements and silicon obsolescence, we strictly divide the system into four primary tiers. Each tier has a distinct boundary enforced by the build system and the C Application Binary Interface (ABI).

### The Architecture Stack: A 20-Year Perspective

```mermaid
graph TD
    subgraph Layer 3: Application (Hardware Agnostic)
        APP[Core Business Logic & State Machines]
        MATH[Math Algorithms / DSP / Filters]
    end

    subgraph Layer 2: Middleware & Services
        OSAL[OS Abstraction Layer]
        FS[File System / NVM Wear Leveling]
        COMMS[Network Stacks / MQTT]
        EXT_DRV[External Device Drivers e.g. IMU]
    end

    subgraph Layer 1: Hardware Abstraction Layer (HAL)
        HAL_API[HAL Interfaces .h - The Firewall]
        HAL_IMPL[MCU-Agnostic HAL Logic .c]
    end

    subgraph Layer 0: Hardware & BSP (The Silicon Reality)
        BSP[Board Support Package]
        VENDOR[Untouched Vendor SDK / HAL]
        MCU[Microcontroller Registers]
    end

    APP --> OSAL
    APP --> FS
    APP --> COMMS
    APP --> EXT_DRV
    APP --> HAL_API
    
    OSAL --> HAL_API
    FS --> HAL_API
    COMMS --> HAL_API
    EXT_DRV --> HAL_API
    
    HAL_IMPL -.->|Fulfills Contract| HAL_API
    HAL_IMPL --> BSP
    BSP --> VENDOR
    VENDOR --> MCU
```

### Deep Dive: The Layer Breakdown

#### Layer 0: Target Hardware & BSP (Board Support Package)
This is the absolute bottom layer. It represents the brutal, inflexible reality of the silicon. It contains the memory-mapped register definitions (e.g., `stm32g4xx.h`), the specific board wiring (e.g., "Pin A5 is the Error LED"), and the untouched **Vendor SDK**. 
- **Silicon Volatility:** This layer is highly volatile. If the semiconductor supply chain forces you to switch from an STM32 to a NXP i.MX RT, this entire layer is thrown away and rewritten. Therefore, absolutely zero business logic is allowed to exist here.
- **Linker Reality:** Code in this layer is often tightly coupled to the linker script, managing interrupt vector tables, `.data` segment initialization, and early clock configuration before `main()` is even called.

#### Layer 1: Hardware Abstraction Layer (HAL)
This is the protective firewall for the rest of your codebase. Our custom, company-standard HAL provides a generic, MCU-agnostic interface for internal peripheral usage. 
- **The Contract:** It defines standard C ABIs for concepts like `UART_Send()`, `PWM_SetDutyCycle()`, and `ADC_Read()`. It translates these generic, high-level calls into the highly specific, often poorly written Vendor SDK calls of Layer 0.
- **Data Agnostic:** The HAL only moves bytes. It does not know what those bytes represent. An I2C HAL function transfers an array of `uint8_t`; it has no idea that those bytes represent a temperature reading from a specific sensor.

#### Layer 2: Middleware & Services
This layer transforms the raw, byte-level capabilities of the HAL into complex, highly useful features for the application. It contains the Operating System Abstraction (OSAL), File Systems (LittleFS), Network Stacks (LwIP), and crucially, drivers for *external* hardware components.
- **External vs Internal Drivers:** If you connect a Bosch BNO055 IMU to your MCU via I2C, the driver for that IMU lives in Layer 2, *not* Layer 1. The IMU is an external service that utilizes the generic internal I2C HAL (Layer 1) to communicate. The IMU driver knows about quaternions and gravity vectors; the I2C HAL only knows about clock stretching and ACK bits.

#### Layer 3: Application Layer
This is where the actual product lives. It contains the high-level state machines, the safety rules, and the core algorithms. Is it a surgical robot arm? A smart thermostat? That logic lives exclusively here.
- **Host PC Testability:** Because this layer only depends on the generic interfaces of Layer 2 and Layer 1, it is 100% hardware-agnostic. You can, and must, compile this layer on a standard x86/ARM64 desktop PC, link it against mock implementations of the HAL, and run your unit tests in milliseconds without flashing a single board.

### ❌ Anti-Pattern: Layer Skipping and The Bypass

The most common, catastrophic violation of this architecture is "Layer Skipping," where a developer in the Application layer bypasses the HAL firewall to directly access Layer 0, usually because "it's faster" or "the vendor HAL already does what I need."

```c
// ANTI-PATTERN: Application Layer Skipping the HAL Firewall
#include "app_thermostat.h"
#include "stm32f4xx_hal.h" // FATAL VIOLATION! Layer 3 including Layer 0 directly.

void App_ThermostatTask(void) {
    float temp = SensorService_GetTemp();
    
    if (temp > 30.0f) {
        // Direct vendor call in the application layer!
        // The business logic is now permanently infected by STMicroelectronics.
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); 
        
        // Even worse: Direct register manipulation bypassing even the vendor SDK
        TIM2->CCR1 = 500; // Hardcoded PWM duty cycle for the fan
    }
}
```

**Deep Technical Rationale for Failure:**
1. **The Obsolescence Trap:** When the STM32 chip is out of stock, and management demands a port to a Nordic nRF52, the engineering team must rewrite `App_ThermostatTask`. The business logic must be re-tested from scratch, incurring massive QA costs and risking life-threatening regressions in the core algorithm, all because of a blinking LED.
2. **Untestable Code:** You cannot unit test `App_ThermostatTask` on a PC. The x86 compiler does not know what `TIM2->CCR1` is, nor does it have the `stm32f4xx_hal.h` header. The code is untestable outside of the physical hardware, guaranteeing a slow, painful debug cycle.

### ✅ Good Pattern: Dependency Injection and The Facade

We prevent layer skipping by strictly injecting dependencies or utilizing the Facade pattern through the HAL.

```c
// GOOD: Application Layer completely isolated from Hardware
#include "app_thermostat.h"
#include "hal_gpio.h"    // Layer 1 Interface ONLY
#include "hal_pwm.h"     // Layer 1 Interface ONLY

// The application uses logical handles, completely ignorant of port/pin assignments
extern Gpio_Handle_t* g_alarm_led; 
extern Pwm_Handle_t*  g_fan_pwm;

void App_ThermostatTask(void) {
    float temp = SensorService_GetTemp();
    
    if (temp > 30.0f) {
        // Safe, abstract HAL calls. 
        // The implementation underneath can be STM32, ESP32, or a PC Mock.
        Hal_Gpio_SetHigh(g_alarm_led);
        Hal_Pwm_SetDutyCycle(g_fan_pwm, 50); // 50%
    }
}
```

### Company Standards & Rules: Layer Integrity

1. **Rule of Strict Upward Isolation:** Code in Layer `N` MUST NOT have any knowledge of Layer `N+1`. A lower layer MUST NOT `#include` a header from a higher layer. If data must move upwards, it MUST be passed via function return values, message queues, or explicitly registered callbacks (function pointers).
2. **Rule of the HAL Firewall (No Layer Skipping):** The Application Layer (Layer 3) and Services Layer (Layer 2) MUST NOT include vendor-specific headers or access memory-mapped hardware registers directly. All hardware interactions MUST go through the custom HAL (Layer 1).
3. **Rule of Application Portability:** The entirety of Layer 3 (Application) MUST compile warning-free using a standard desktop compiler (GCC/Clang on x86/ARM64) without requiring any embedded cross-compilation toolchains or vendor SDKs. It must link against Mock HAL implementations to prove its hardware independence.
4. **Rule of Build System Enforcement:** The build system (CMake) MUST mechanically enforce layer boundaries by explicitly restricting target include directories. The application target MUST fail to compile if it attempts to include a header from Layer 0.