# 17.2 Module template standard

In C, the compiler enforces very few object-oriented or encapsulation rules. A developer can easily expose internal state globally, ignore initialization routines, and create a tangled web of `#include` dependencies.

To counteract this, the framework must provide and mandate strict **Module Templates** for all `.c` and `.h` files. These templates enforce architectural best practices—like opaque pointers, explicit initialization, and proper documentation—right from the creation of a new file.

## The Standard Header File (`.h`) Template

The header file is the public contract of the module. It must expose *only* what is necessary to use the module, hiding all internal implementation details.

```c
/**
 * @file    thermometer.h
 * @brief   Application module for managing temperature readings.
 * @details Implements filtering and alarm threshold logic. Hardware agnostic.
 */

#ifndef THERMOMETER_H
#define THERMOMETER_H

#include <stdint.h>
#include <stdbool.h>
#include "company_errors.h" // Standard framework error types
#include "i2c_interface.h"  // HAL dependency interface

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Opaque Types (Encapsulation)                                               */
/* -------------------------------------------------------------------------- */
/** @brief Opaque handle representing a thermometer instance */
typedef struct thermometer_instance* thermometer_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Configures the thermometer dependencies.
 * @param i2c_bus Pointer to the I2C interface to use for communication.
 */
typedef struct {
    const i2c_interface_t* i2c_bus;
    float high_alarm_threshold_c;
} thermometer_config_t;

/**
 * @brief Initializes a thermometer instance.
 * @param out_inst Pointer to return the initialized handle.
 * @param config Configuration parameters and dependency injection.
 * @return SYS_OK on success, standard error code otherwise.
 */
sys_err_t thermometer_init(thermometer_t* out_inst, const thermometer_config_t* config);

/**
 * @brief Reads the current filtered temperature.
 * @param inst The thermometer instance.
 * @param out_temp_c Pointer to return the temperature in Celsius.
 * @return SYS_OK on success.
 */
sys_err_t thermometer_read(thermometer_t inst, float* out_temp_c);

#ifdef __cplusplus
}
#endif

#endif // THERMOMETER_H
```

### Key Rules Enforced by the Header Template:
1.  **Doxygen Comments:** Mandatory header blocks and function documentation.
2.  **Opaque Pointers (`typedef struct name* name_t;`):** The internal state structure is *not* defined in the header. Callers cannot access internal variables directly; they must use the provided API. This enforces true encapsulation in C.
3.  **Standardized Error Returns:** Every function that can fail returns `sys_err_t`.
4.  **Configuration Structs for Dependency Injection:** Dependencies (like the `i2c_interface_t`) are passed in via a config struct during initialization, not hardcoded as `#include "stm32_i2c.h"`.

## The Standard Source File (`.c`) Template

The source file contains the implementation details. Because the header uses opaque pointers, the actual structure definition lives here, safely hidden from the rest of the application.

```c
/**
 * @file    thermometer.c
 * @brief   Implementation of the thermometer module.
 */

#include "thermometer.h"
#include "company_log.h"     // Framework logging
#include "company_assert.h"  // Framework assertions
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* Private Types & State                                                      */
/* -------------------------------------------------------------------------- */

// Actual definition of the opaque struct. Hidden from callers.
struct thermometer_instance {
    const i2c_interface_t* i2c;
    float threshold;
    bool is_initialized;
    float last_reading;
};

// Static allocation pool (if dynamic memory is banned)
#define MAX_THERMOMETERS 4
static struct thermometer_instance s_instances[MAX_THERMOMETERS];
static uint8_t s_instance_count = 0;

/* -------------------------------------------------------------------------- */
/* Private Functions                                                          */
/* -------------------------------------------------------------------------- */

/** @brief Internal helper, static to restrict scope. */
static float apply_kalman_filter(float raw_val) {
    // ... filtering logic ...
    return raw_val;
}

/* -------------------------------------------------------------------------- */
/* Public API Implementation                                                  */
/* -------------------------------------------------------------------------- */

sys_err_t thermometer_init(thermometer_t* out_inst, const thermometer_config_t* config) {
    SYS_ASSERT(out_inst != NULL);
    SYS_ASSERT(config != NULL);
    SYS_ASSERT(config->i2c_bus != NULL);

    if (s_instance_count >= MAX_THERMOMETERS) {
        LOG_ERROR("Max thermometer instances reached");
        return SYS_ERR_NO_MEMORY;
    }

    struct thermometer_instance* inst = &s_instances[s_instance_count++];
    
    inst->i2c = config->i2c_bus;
    inst->threshold = config->high_alarm_threshold_c;
    inst->last_reading = 0.0f;
    inst->is_initialized = true;

    *out_inst = inst;
    return SYS_OK;
}

sys_err_t thermometer_read(thermometer_t inst, float* out_temp_c) {
    SYS_ASSERT(inst != NULL);
    SYS_ASSERT(inst->is_initialized == true);
    SYS_ASSERT(out_temp_c != NULL);

    uint8_t raw_data[2];
    // Use the injected I2C dependency, NOT a hardcoded vendor call
    sys_err_t err = inst->i2c->read(0x4A, 0x00, raw_data, 2);
    
    if (err != SYS_OK) {
        LOG_ERROR("I2C read failed with code %d", err);
        return err;
    }

    // Process data...
    *out_temp_c = apply_kalman_filter((float)raw_data[0]); 
    return SYS_OK;
}
```

### Key Rules Enforced by the Source Template:
1.  **Private State:** `struct thermometer_instance` is defined here.
2.  **`static` Memory Allocation:** The module manages its own memory pool using statically allocated arrays, avoiding `malloc`.
3.  **`static` Private Functions:** All internal helper functions must be declared `static` to prevent namespace pollution in the linker.
4.  **Design by Contract / Assertions:** Every public API starts with `SYS_ASSERT()` to validate pointers and preconditions, ensuring fail-fast behavior.