/**
 * @file hal_gpio.c
 * @brief Implementation of the GPIO Hardware Abstraction Layer.
 */

#include "hal_gpio.h"
#include <stddef.h>

/**
 * ARCHITECTURE DECISION: Memory footprint vs Performance
 * Allocating a struct for every single pin in the system can consume significant RAM.
 * We dimension MAX_GPIO_PINS based on realistic usage of dynamic pin handles,
 * or we use a flat array if memory permits. For 100 pins, an array of structs is small enough.
 */
#define MAX_GPIO_PINS 64

struct HalGpio_s {
    bool is_allocated;
    HalGpioConfig_t config;
};

static struct HalGpio_s g_gpio_pool[MAX_GPIO_PINS] = {0};

HalGpioHandle HalGpio_Init(const HalGpioConfig_t* config) {
    if (!config || config->port_id > 10 || config->pin_number > 15) return NULL;

    /* Simple allocation strategy: find first free */
    struct HalGpio_s* inst = NULL;
    for (int i = 0; i < MAX_GPIO_PINS; i++) {
        if (!g_gpio_pool[i].is_allocated) {
            inst = &g_gpio_pool[i];
            inst->is_allocated = true;
            break;
        }
    }

    if (!inst) return NULL; /* Pool exhausted */

    inst->config = *config;

    /* TODO: Hardware initialization.
       - Enable Port Clock.
       - Set MODER, PUPDR, OTYPER, OSPEEDR.
       - If alternate_function is set, configure AFR.
       - If interrupt_trigger != NONE, configure EXTI/SYSCFG and NVIC.
    */

    return (HalGpioHandle)inst;
}

void HalGpio_Write(HalGpioHandle handle, HalGpioState_t state) {
    if (!handle) return;
    /* TODO: Write to BSRR or ODR registers */
    (void)state;
}

HalGpioState_t HalGpio_Read(HalGpioHandle handle) {
    if (!handle) return HAL_GPIO_STATE_LOW;
    /* TODO: Read IDR register */
    return HAL_GPIO_STATE_LOW; /* Stub */
}

void HalGpio_Toggle(HalGpioHandle handle) {
    if (!handle) return;
    /* TODO: Read ODR, invert, Write ODR, or use specific toggle register */
}

void HalGpio_DeInit(HalGpioHandle handle) {
    if (handle) {
        /* TODO: Reset pin registers to default analog state, disable EXTI */
        handle->is_allocated = false;
    }
}

/**
 * ARCHITECTURE DECISION: Interrupt Dispatching
 * This function would be called from the vendor-specific IRQ handler (e.g., EXTI15_10_IRQHandler).
 * It scans the pool to find the configured pin and fires the callback.
 */
void HalGpio_InternalInterruptDispatcher(uint8_t pin_number) {
    for (int i = 0; i < MAX_GPIO_PINS; i++) {
        if (g_gpio_pool[i].is_allocated && 
            g_gpio_pool[i].config.pin_number == pin_number &&
            g_gpio_pool[i].config.isr_cb != NULL) {
            
            g_gpio_pool[i].config.isr_cb(g_gpio_pool[i].config.isr_context);
        }
    }
}
