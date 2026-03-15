/**
 * @file Halgpio.c
 * @brief Implementation of the GPIO Hardware Abstraction Layer for STM32F401RE.
 */

#include "Halgpio.h"
#include "../../../hardware/stm32/nucleo-f401re/hal/Halgpio.h" // Use the hardware definitions
#include <stddef.h>

/**
 * ARCHITECTURE DECISION: Memory footprint vs Performance
 * Allocating a struct for every single pin in the system can consume significant RAM.
 * We dimension MAX_GPIO_PINS based on realistic usage of dynamic pin handles.
 */
#define MAX_GPIO_PINS 64

struct HalGpio_s {
    bool is_allocated;
    HalGpioConfig_t config;
    GPIO_Regs_t* port;
    uint16_t pin_mask;
};

static struct HalGpio_s g_gpio_pool[MAX_GPIO_PINS] = {0};

/* Helper to map port_id to registers */
static GPIO_Regs_t* get_port_regs(uint8_t port_id) {
    switch (port_id) {
        case 0: return GPIOA;
        case 1: return GPIOB;
        case 2: return GPIOC;
        case 3: return GPIOD;
        case 4: return GPIOE;
        case 7: return GPIOH;
        default: return NULL;
    }
}

HalGpioHandle HalGpio_Init(const HalGpioConfig_t* config) {
    if (!config || config->pin_number > 15) return NULL;

    GPIO_Regs_t* port = get_port_regs(config->port_id);
    if (!port) return NULL;

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
    inst->port = port;
    inst->pin_mask = (uint16_t)(1U << config->pin_number);

    /* 1. Enable Clock */
    HalGPIO_ClockEnable(port);

    /* 2. Map Course Mode to Hardware Mode */
    GPIO_Mode_t mode;
    GPIO_OType_t otype = GPIO_OTYPE_PUSHPULL;
    switch (config->mode) {
        case HalGPIO_MODE_INPUT:     mode = GPIO_MODE_INPUT; break;
        case HalGPIO_MODE_OUTPUT_PP: mode = GPIO_MODE_OUTPUT; otype = GPIO_OTYPE_PUSHPULL; break;
        case HalGPIO_MODE_OUTPUT_OD: mode = GPIO_MODE_OUTPUT; otype = GPIO_OTYPE_OPENDRAIN; break;
        case HalGPIO_MODE_AF_PP:     mode = GPIO_MODE_ALTERNATE; otype = GPIO_OTYPE_PUSHPULL; break;
        case HalGPIO_MODE_AF_OD:     mode = GPIO_MODE_ALTERNATE; otype = GPIO_OTYPE_OPENDRAIN; break;
        case HalGPIO_MODE_ANALOG:    mode = GPIO_MODE_ANALOG; break;
        default: mode = GPIO_MODE_INPUT; break;
    }

    /* 3. Map Course Pull to Hardware Pull */
    GPIO_Pull_t pull;
    switch (config->pull) {
        case HalGPIO_PULL_NONE: pull = GPIO_PULL_NONE; break;
        case HalGPIO_PULL_UP:   pull = GPIO_PULL_UP; break;
        case HalGPIO_PULL_DOWN: pull = GPIO_PULL_DOWN; break;
        default: pull = GPIO_PULL_NONE; break;
    }

    /* 4. Execute Hardware Init */
    HalGPIO_Init(port, inst->pin_mask, mode, otype, GPIO_SPEED_HIGH, pull, (GPIO_AF_t)config->alternate_function);

    /* TODO: If interrupt_trigger != NONE, configure EXTI/SYSCFG and NVIC. 
       This requires more complex mapping of pin to EXTI line. */

    return (HalGpioHandle)inst;
}

void HalGpio_Write(HalGpioHandle handle, HalGpioState_t state) {
    if (!handle) return;
    if (state == HalGPIO_STATE_HIGH) {
        HalGPIO_Set(handle->port, handle->pin_mask);
    } else {
        HalGPIO_Reset(handle->port, handle->pin_mask);
    }
}

HalGpioState_t HalGpio_Read(HalGpioHandle handle) {
    if (!handle) return HalGPIO_STATE_LOW;
    return (HalGPIO_Read(handle->port, handle->pin_mask) == GPIO_PIN_SET) ? 
            HalGPIO_STATE_HIGH : HalGPIO_STATE_LOW;
}

void HalGpio_Toggle(HalGpioHandle handle) {
    if (!handle) return;
    HalGPIO_Toggle(handle->port, handle->pin_mask);
}

void HalGpio_DeInit(HalGpioHandle handle) {
    if (handle) {
        /* Set pin to Analog mode (lowest power) */
        HalGPIO_Init(handle->port, handle->pin_mask, GPIO_MODE_ANALOG, 
                      GPIO_OTYPE_PUSHPULL, GPIO_SPEED_LOW, GPIO_PULL_NONE, GPIO_AF0);
        handle->is_allocated = false;
    }
}

void HalGpio_InternalInterruptDispatcher(uint8_t pin_number) {
    for (int i = 0; i < MAX_GPIO_PINS; i++) {
        if (g_gpio_pool[i].is_allocated && 
            g_gpio_pool[i].config.pin_number == pin_number &&
            g_gpio_pool[i].config.isr_cb != NULL) {
            
            g_gpio_pool[i].config.isr_cb(g_gpio_pool[i].config.isr_context);
        }
    }
}
