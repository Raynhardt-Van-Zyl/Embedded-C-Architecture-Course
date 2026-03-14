/**
 * @file hal_wdt.h
 * @brief Hardware Abstraction Layer for Watchdog Timer (WDT).
 *
 * @details
 * ARCHITECTURE DECISION: Unified Windowed and Independent Watchdog API
 * Microcontrollers often have both an Independent Watchdog (IWDG - runs on LSI) 
 * and a Windowed Watchdog (WWDG - runs on APB clock, triggers early wakeup). 
 * This API unifies them. Setting window_ms > 0 engages the windowed behavior.
 */

#ifndef HAL_WDT_H
#define HAL_WDT_H

#include <stdint.h>
#include <stdbool.h>

/** @brief Opaque handle for WDT instances. */
typedef struct HalWdt_s* HalWdtHandle;

/** @brief WDT Early Wakeup Callback. */
typedef void (*HalWdtEarlyWakeupCallback_t)(void* context);

/** @brief WDT configuration structure. */
typedef struct {
    uint32_t timeout_ms; /**< Reset timeout in milliseconds */
    uint32_t window_ms;  /**< Window size. If 0, operates as standard independent WDT. If > 0, feeding too fast causes reset. */
    HalWdtEarlyWakeupCallback_t ewi_cb; /**< Callback executed right before WWDG reset occurs */
    void* ewi_context;   /**< Callback context */
} HalWdtConfig_t;

/**
 * @brief Initialize and start the Watchdog Timer.
 * @note Once started, the WDT typically cannot be stopped except by hardware reset.
 * @param config Pointer to the configuration struct.
 * @return HalWdtHandle Valid handle or NULL.
 */
HalWdtHandle HalWdt_Init(const HalWdtConfig_t* config);

/**
 * @brief Feed (reload) the Watchdog to prevent a system reset.
 * @param handle The WDT handle.
 */
void HalWdt_Feed(HalWdtHandle handle);

#endif /* HAL_WDT_H */
