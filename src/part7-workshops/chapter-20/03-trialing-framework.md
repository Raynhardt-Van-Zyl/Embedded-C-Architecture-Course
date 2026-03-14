# Trialing the Framework

With Framework V1 defined (Standard Errors, Logging, Core Types, and a basic HAL interface), it is time to deploy it. Because we established that a "Big Rewrite" is disastrous, we must integrate the framework incrementally.

The standard architectural pattern for this is the **Strangler Fig Pattern**.

## The Strangler Fig Pattern in Embedded C

Named after a tree that grows around a host tree, eventually replacing it, this pattern involves slowly wrapping legacy code with new framework code. New features are built entirely using the framework. Old modules are ported one by one. Over time, the legacy code is "strangled" out of existence.

### Step 1: The Build System Integration

First, the framework must be brought into the legacy project's build system as a distinct, isolated library or folder module. 

- The legacy code is allowed to `#include` framework headers.
- The framework is **STRICTLY FORBIDDEN** from `#including` legacy headers.

If the framework needs something from the legacy code, the legacy code must provide it via a function pointer (Dependency Injection).

### Step 2: Porting a Leaf Node

Do not attempt to port the main application logic or the most complex communication stack first. Find a **Leaf Node**—a module that has few dependencies on other parts of the system. A simple I2C temperature sensor driver is a perfect candidate.

#### Legacy Sensor Driver
```c
// legacy_temp_sensor.c
#include "stm32f4_i2c.h" // Tightly coupled hardware

int read_temp_sensor(void) {
    if (I2C_Start() != 0) return -1;
    // ... bit banging or direct register access ...
    return temp_value;
}
```

#### Framework-Compliant Sensor Driver
We rewrite this leaf node to use the Framework V1 standards.

```c
// fw_temp_sensor.c
#include "fw_errors.h"
#include "fw_i2c_interface.h"

// Injected dependency
static fw_i2c_interface_t *i2c_bus;

fw_result_t fw_temp_sensor_init(fw_i2c_interface_t *bus) {
    if (bus == NULL) return FW_ERR_INVALID_PARAM;
    i2c_bus = bus;
    return FW_OK;
}

fw_result_t fw_temp_sensor_read(int32_t *out_temp) {
    uint8_t data[2];
    FW_RETURN_IF_ERROR(i2c_bus->read(i2c_bus, TEMP_ADDR, data, 2));
    
    *out_temp = (data[0] << 8) | data[1];
    return FW_OK;
}
```

### Step 3: The Adapter / Shim Layer

Now, how does the legacy application use this new framework sensor without rewriting the legacy application? We create a **Shim**.

The shim matches the signature of the legacy function but calls the new framework code under the hood.

```c
// legacy_temp_sensor_shim.c (Replaces legacy_temp_sensor.c)
#include "fw_temp_sensor.h"
#include "legacy_globals.h" // Shim is allowed to touch legacy

// The legacy app still calls this exact function signature
int read_temp_sensor(void) {
    int32_t temp = 0;
    
    // Call the new framework
    fw_result_t res = fw_temp_sensor_read(&temp);
    
    if (res != FW_OK) {
        return -1; // Translate framework error back to legacy error
    }
    return (int)temp;
}
```

## Architectural Rules for Trialing

1. **Rule of Framework Isolation:** The framework folder/repository must compile cleanly entirely on its own. It cannot possess a single dependency on the legacy application it is being integrated into.
2. **Rule of New Code:** All *new* peripheral drivers or business logic modules commissioned after the release of Framework V1 MUST be written using the framework interfaces. No new legacy code is permitted.
3. **Rule of the Shim:** Legacy application code must not be heavily refactored just to support the framework. Instead, write shim layers to translate between the old application domain and the new framework domain.