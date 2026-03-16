# Chapter 4.4: Interface Design Patterns (Run-Time Polymorphism)

In modern C++ or Java, "Polymorphism"—the ability for a single interface to represent multiple different underlying implementations—is a native language feature. You define a pure virtual `class`, and the compiler automatically handles the routing of function calls to the correct object.

In C, we have no native objects, no inheritance, and no virtual keywords. Yet, to achieve true hardware independence and runtime flexibility, we absolutely require polymorphism. If a system has three different types of temperature sensors (I2C, SPI, and Analog), the high-level application shouldn't care which one it's talking to. It just wants to call `Sensor->Read()`.

We achieve this in C using a profound architectural pattern: **The V-Table (Virtual Method Table)**. This is exactly how the C++ compiler implements virtual functions under the hood, but in C, we build it manually.

---

## 1. The Anatomy of a V-Table in C

A V-Table is simply a `struct` consisting entirely of Function Pointers. It defines the "Contract" or the "Interface."

### 1.1 Step 1: Defining the Interface (`.h`)
First, we define the V-Table struct. Crucially, we also define an Opaque Pointer (the "Instance") that will hold the state of the specific implementation.

```c
// PRODUCTION STANDARD: Polymorphic Interface
// i_temperature_sensor.h

// 1. Opaque pointer for the instance data (State)
typedef struct ITempSensor_Context_t ITempSensor_t;

// 2. The V-Table (The Interface Contract)
typedef struct {
    // Every function in the V-Table MUST take the opaque instance pointer as its first argument!
    // This is the equivalent of the implicit 'this' pointer in C++.
    float (*ReadCelsius)(ITempSensor_t* self);
    bool  (*IsHealthy)(ITempSensor_t* self);
} ITempSensor_VTable_t;

// 3. The Interface Wrapper (Optional, but highly recommended for clean syntax)
// Instead of forcing the caller to dereference the function pointer manually,
// we provide a clean inline wrapper.
static inline float ITempSensor_Read(ITempSensor_t* self, const ITempSensor_VTable_t* vtable) {
    return vtable->ReadCelsius(self);
}
```

### 1.2 Step 2: The Concrete Implementation (`.c`)
Now we implement a specific sensor, for example, a Bosch BME280 on I2C.

```c
// driver_bme280.c
#include "i_temperature_sensor.h"
#include "hal_i2c.h"

// 1. The Concrete State (Hidden in the .c file)
struct ITempSensor_Context_t {
    HAL_I2C_t* i2c_bus; // Hardware dependency
    uint8_t i2c_address;
    float last_reading;
};

// 2. The Concrete Functions (Must match the V-Table signatures)
// Note: These are marked static! They are hidden from the linker.
static float BME280_Read(ITempSensor_t* self) {
    // Cast the generic 'self' to our known concrete struct type.
    // (In a highly rigorous system, you might add a magic number check here to verify the cast).
    struct ITempSensor_Context_t* ctx = (struct ITempSensor_Context_t*)self;
    
    // ... perform I2C read using ctx->i2c_bus ...
    ctx->last_reading = 25.5f; // Dummy data
    return ctx->last_reading;
}

static bool BME280_Health(ITempSensor_t* self) {
    return true;
}

// 3. The Concrete V-Table Instance
// This is instantiated ONCE in flash (const) to save RAM.
const ITempSensor_VTable_t BME280_VTable = {
    .ReadCelsius = BME280_Read,
    .IsHealthy   = BME280_Health
};

// 4. The Factory/Init Function
void BME280_Init(ITempSensor_t* instance, HAL_I2C_t* i2c, uint8_t addr) {
    struct ITempSensor_Context_t* ctx = (struct ITempSensor_Context_t*)instance;
    ctx->i2c_bus = i2c;
    ctx->i2c_address = addr;
}
```

### 1.3 Step 3: Polymorphism in Action (The Application)
The high-level application logic now receives both the Instance data AND the V-Table. It has no idea it is talking to a BME280. It just executes the contract.

```c
// thermostat_app.c
#include "i_temperature_sensor.h"

void Thermostat_ControlLoop(ITempSensor_t* sensor_instance, const ITempSensor_VTable_t* sensor_vtable) {
    
    // The Double Dereference: 
    // The CPU looks up the vtable pointer, finds the 'ReadCelsius' address, and jumps to it,
    // passing the 'sensor_instance' state data as the argument.
    float current_temp = ITempSensor_Read(sensor_instance, sensor_vtable);
    
    if (current_temp > 30.0f) {
        // Turn on AC
    }
}
```

---

## 2. The Memory and Silicon Cost

```mermaid
graph LR
    subgraph SG_1["RAM"]
        App[App Logic] -->|Holds Pointer to| State[ITempSensor_Context_t\ni2c_bus\ni2c_address]
        App -->|Holds Pointer to| VT_Flash
    end
    
    subgraph SG_2["FLASH (ROM)"]
        VT_Flash[BME280_VTable] -->|ReadCelsius Ptr| Func1[BME280_Read() Instructions]
        VT_Flash -->|IsHealthy Ptr| Func2[BME280_Health() Instructions]
    end
    
    State -.->|Passed as 'self' to| Func1
```

**The Cost:** Calling a function via a V-Table requires a "Double Dereference." 
1. The CPU loads the V-Table address.
2. The CPU calculates the offset to the function pointer.
3. The CPU loads the function pointer address.
4. The CPU branches to that address.

On a Cortex-M4, this branch is indirect, meaning the CPU's branch predictor often fails, causing a minor pipeline flush (typically 3-5 extra clock cycles compared to a direct call). Furthermore, because the compiler doesn't know which function will be called until runtime, it **cannot inline the function**.

**The Verdict:** Do not use V-Tables for toggling a GPIO pin at 1MHz. However, for a Temperature Sensor read that happens every 100ms, or a network stack transmitting a packet, the 5-cycle overhead is mathematically irrelevant, while the architectural flexibility gained is monumental.

---

## 3. Company Standard Rules for V-Tables

1. **The 'self' Pointer:** Every function signature within a V-Table `struct` MUST accept an opaque pointer representing the instance state (`self` or `context`) as its first argument. This is non-negotiable for thread-safety and multi-instance support.
2. **Const V-Tables:** Concrete V-Table instances MUST be declared `const` so they are placed in Flash memory (`.rodata`) rather than consuming precious RAM (`.data`), preventing accidental or malicious corruption of function pointers.
3. **Restricted Hot-Paths:** V-Tables shall NOT be used for ultra-high-frequency hardware access (e.g., inside a high-speed PWM or ADC interrupt service routine) due to the pipeline stall overhead of indirect branching. Use Link-Time substitution (direct function calls resolved by the linker) for these specific paths.
4. **Explicit Interfaces:** Do not use massive `switch(sensor_type)` statements to handle polymorphism. If a module needs to support multiple different underlying implementations at runtime, it MUST utilize a formally defined V-Table struct.
