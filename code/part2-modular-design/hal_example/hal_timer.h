/**
 * @file Haltimer.h
 * @brief Hardware Abstraction Layer for General Purpose Timers.
 *
 * @details
 * ARCHITECTURE DECISION: Time Base API
 * General purpose timers are abstracted to deal strictly in microseconds 
 * rather than raw clock ticks. The HAL calculates Prescalers and Auto-Reload 
 * values behind the scenes, ensuring portability across varying core clock speeds.
 */

#ifndef HalTIMER_H
#define HalTIMER_H

#include <stdint.h>
#include <stdbool.h>

/** @brief Opaque handle for Timer instances. */
typedef struct HalTimer_s* HalTimerHandle;

/** @brief Timer Operating Mode. */
typedef enum {
    HalTIMER_MODE_PERIODIC,   /**< Automatically reloads and repeats */
    HalTIMER_MODE_ONE_SHOT,   /**< Runs once then stops */
    HalTIMER_MODE_INPUT_CAPTURE /**< Captures timestamp on external pin event */
} HalTimerMode_t;

/** @brief Timer interrupt callback. */
typedef void (*HalTimerCallback_t)(void* context);

/** @brief Timer configuration structure. */
typedef struct {
    uint8_t timer_id;        /**< Physical timer ID (e.g., 2 for TIM2) */
    HalTimerMode_t mode;     /**< Operating mode */
    uint32_t period_us;      /**< Period in microseconds */
    HalTimerCallback_t cb;   /**< Callback when period elapses or capture occurs */
    void* cb_context;        /**< Callback context */
} HalTimerConfig_t;

/**
 * @brief Initialize the Timer.
 * @param config Pointer to the configuration struct.
 * @return HalTimerHandle Valid handle or NULL.
 */
HalTimerHandle HalTimer_Init(const HalTimerConfig_t* config);

/**
 * @brief Start the Timer.
 * @param handle The Timer handle.
 */
void HalTimer_Start(HalTimerHandle handle);

/**
 * @brief Stop the Timer.
 * @param handle The Timer handle.
 */
void HalTimer_Stop(HalTimerHandle handle);

/**
 * @brief Get current raw count of the Timer.
 * @param handle The Timer handle.
 * @return Current counter value.
 */
uint32_t HalTimer_GetCount(HalTimerHandle handle);

/**
 * @brief Configure Input Capture mode for a specific channel.
 * @param handle The Timer handle.
 * @param channel The capture channel (1-4).
 * @param rising_edge True to capture on rising edge, false for falling edge.
 */
void HalTimer_ConfigInputCapture(HalTimerHandle handle, uint8_t channel, bool rising_edge);

/**
 * @brief Get the captured value after an Input Capture event.
 * @param handle The Timer handle.
 * @param channel The capture channel.
 * @return The captured counter value.
 */
uint32_t HalTimer_GetCaptureValue(HalTimerHandle handle, uint8_t channel);

/**
 * @brief Deinitialize the Timer.
 * @param handle The Timer handle.
 */
void HalTimer_DeInit(HalTimerHandle handle);

#endif /* HalTIMER_H */
