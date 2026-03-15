/**
 * @file Halgpio.h
 * @brief Hardware Abstraction Layer for GPIO.
 *
 * @details
 * ARCHITECTURE DECISION: Decoupling ISRs from Application Logic
 * By registering a function pointer callback for external interrupts, 
 * the application layer avoids needing to know the specific IRQn 
 * or write architecture-specific ISR handlers.
 */

#ifndef HalGPIO_H
#define HalGPIO_H

#include <stdint.h>
#include <stdbool.h>

/** @brief Opaque handle for GPIO instances. */
typedef struct HalGpio_s* HalGpioHandle;

/** @brief GPIO pin modes. */
typedef enum {
    HalGPIO_MODE_INPUT,
    HalGPIO_MODE_OUTPUT_PP, /**< Push-Pull */
    HalGPIO_MODE_OUTPUT_OD, /**< Open-Drain */
    HalGPIO_MODE_AF_PP,     /**< Alternate Function Push-Pull */
    HalGPIO_MODE_AF_OD,     /**< Alternate Function Open-Drain */
    HalGPIO_MODE_ANALOG     /**< Analog (ADC/DAC) */
} HalGpioMode_t;

/** @brief GPIO pull-up/pull-down resistor settings. */
typedef enum {
    HalGPIO_PULL_NONE,
    HalGPIO_PULL_UP,
    HalGPIO_PULL_DOWN
} HalGpioPull_t;

/** @brief GPIO interrupt trigger definitions. */
typedef enum {
    HalGPIO_INTR_NONE,
    HalGPIO_INTR_RISING_EDGE,
    HalGPIO_INTR_FALLING_EDGE,
    HalGPIO_INTR_BOTH_EDGES
} HalGpioIntr_t;

/** @brief Callback function type for GPIO interrupts. */
typedef void (*HalGpioInterruptCallback_t)(void* context);

/** @brief GPIO configuration structure. */
typedef struct {
    uint8_t port_id;                 /**< e.g., 0 for Port A, 1 for Port B */
    uint8_t pin_number;              /**< Pin number 0-15 */
    HalGpioMode_t mode;              /**< Operating mode */
    HalGpioPull_t pull;              /**< Internal pull resistors */
    uint8_t alternate_function;      /**< AF number if mode is AF */
    HalGpioIntr_t interrupt_trigger; /**< Interrupt trigger condition */
    HalGpioInterruptCallback_t isr_cb; /**< Callback on interrupt */
    void* isr_context;               /**< User context passed to callback */
} HalGpioConfig_t;

/** @brief Logical state of a pin. */
typedef enum {
    HalGPIO_STATE_LOW = 0,
    HalGPIO_STATE_HIGH = 1
} HalGpioState_t;

/**
 * @brief Initialize a GPIO pin.
 * @param config Pointer to the configuration struct.
 * @return HalGpioHandle Valid handle or NULL.
 */
HalGpioHandle HalGpio_Init(const HalGpioConfig_t* config);

/**
 * @brief Write state to a GPIO output pin.
 * @param handle The GPIO handle.
 * @param state Low or High.
 */
void HalGpio_Write(HalGpioHandle handle, HalGpioState_t state);

/**
 * @brief Read state from a GPIO pin.
 * @param handle The GPIO handle.
 * @return HalGpioState_t Low or High.
 */
HalGpioState_t HalGpio_Read(HalGpioHandle handle);

/**
 * @brief Toggle the state of a GPIO output pin.
 * @param handle The GPIO handle.
 */
void HalGpio_Toggle(HalGpioHandle handle);

/**
 * @brief Deinitialize a GPIO pin.
 * @param handle The GPIO handle.
 */
void HalGpio_DeInit(HalGpioHandle handle);

#endif /* HalGPIO_H */
