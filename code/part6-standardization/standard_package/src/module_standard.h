/**
 * @file module_standard.h
 * @brief Template for a "Gold Standard" module.
 * 
 * DESIGN RULES APPLIED:
 * 1. Opaque handle for state encapsulation.
 * 2. Static allocation pool.
 * 3. Consistent PascalCase naming.
 * 4. Standardized status returns.
 */

#ifndef MODULE_STANDARD_H
#define MODULE_STANDARD_H

#include "hal_interface.h"

/** @brief Opaque handle to module instance. */
typedef struct HalModule_s* HalModuleHandle;

/** @brief Module configuration. */
typedef struct {
    uint32_t frequency;
    uint8_t  id;
} HalModuleConfig_t;

/**
 * @brief Initialize the module.
 * @param config Pointer to configuration struct.
 * @return HalModuleHandle or NULL on failure.
 */
HalModuleHandle HalModule_Init(const HalModuleConfig_t* config);

/**
 * @brief Perform a standard operation.
 * @param handle Module handle.
 * @return HAL_STATUS_OK on success.
 */
HalStatus_t HalModule_DoWork(HalModuleHandle handle);

/**
 * @brief Deinitialize the module.
 */
void HalModule_DeInit(HalModuleHandle handle);

#endif /* MODULE_STANDARD_H */
