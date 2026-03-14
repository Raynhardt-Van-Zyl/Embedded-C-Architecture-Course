/**
 * @file hal_pwm.h
 * @brief Hardware Abstraction Layer for Pulse Width Modulation (PWM).
 *
 * @details
 * ARCHITECTURE DECISION: Floating Point Duty Cycle vs Fixed Point
 * Using a float (0.0f to 100.0f) for duty cycle makes the API extremely intuitive for
 * application developers. The HAL handles the internal integer math required to set
 * the Capture/Compare registers based on the current Auto-Reload value.
 */

#ifndef HAL_PWM_H
#define HAL_PWM_H

#include <stdint.h>
#include <stdbool.h>

/** @brief Opaque handle for PWM instances. */
typedef struct HalPwm_s* HalPwmHandle;

/** @brief PWM Alignment Mode. */
typedef enum {
    HAL_PWM_ALIGN_EDGE,   /**< Edge-aligned mode (standard) */
    HAL_PWM_ALIGN_CENTER  /**< Center-aligned mode (good for motor control) */
} HalPwmAlign_t;

/** @brief PWM configuration structure. */
typedef struct {
    uint8_t timer_id;        /**< Underlying hardware timer ID */
    uint8_t channel;         /**< Timer channel (e.g., 1 to 4) */
    uint32_t frequency_hz;   /**< Desired PWM frequency in Hz */
    HalPwmAlign_t alignment; /**< Edge or Center aligned */
    uint32_t dead_time_ns;   /**< Dead-time insertion in nanoseconds (for complementary outputs) */
} HalPwmConfig_t;

/**
 * @brief Initialize a PWM channel.
 * @param config Pointer to the configuration struct.
 * @return HalPwmHandle Valid handle or NULL.
 */
HalPwmHandle HalPwm_Init(const HalPwmConfig_t* config);

/**
 * @brief Set the PWM duty cycle.
 * @param handle The PWM handle.
 * @param duty_cycle_percent Float from 0.0f to 100.0f.
 */
void HalPwm_SetDutyCycle(HalPwmHandle handle, float duty_cycle_percent);

/**
 * @brief Start PWM output.
 * @param handle The PWM handle.
 */
void HalPwm_Start(HalPwmHandle handle);

/**
 * @brief Stop PWM output.
 * @param handle The PWM handle.
 */
void HalPwm_Stop(HalPwmHandle handle);

/**
 * @brief Deinitialize the PWM channel.
 * @param handle The PWM handle.
 */
void HalPwm_DeInit(HalPwmHandle handle);

#endif /* HAL_PWM_H */
