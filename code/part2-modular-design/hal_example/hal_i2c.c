/**
 * @file Hali2c.c
 * @brief Implementation of the I2C Hardware Abstraction Layer for STM32F401RE.
 */

#include "Hali2c.h"
#include "../../../hardware/stm32/nucleo-f401re/hal/Hali2c.h" // Hardware definitions
#include <stddef.h>

/**
 * ARCHITECTURE DECISION: Static Instance Pool
 */
#define MAX_I2C_INSTANCES 3

struct HalI2c_s {
    bool is_allocated;
    HalI2cConfig_t config;
    I2C_Regs_t* regs;
};

/* Static pool of instances */
static struct HalI2c_s g_i2c_pool[MAX_I2C_INSTANCES] = {0};

/* Helper to map ID to registers */
static I2C_Regs_t* get_i2c_regs(uint8_t id) {
    switch (id) {
        case 1: return I2C1;
        case 2: return I2C2;
        case 3: return I2C3;
        default: return NULL;
    }
}

HalI2cHandle HalI2c_Init(const HalI2cConfig_t* config) {
    if (!config || config->i2c_bus_id == 0 || config->i2c_bus_id > MAX_I2C_INSTANCES) {
        return NULL;
    }

    struct HalI2c_s* inst = &g_i2c_pool[config->i2c_bus_id - 1];
    
    if (inst->is_allocated) return NULL;
    inst->is_allocated = true;
    inst->config = *config;
    inst->regs = get_i2c_regs(config->i2c_bus_id);

    /* Hardware Init logic ported from hardware/hal/Hali2c.c */
    // 1. Enable Clocks (RCC->APB1ENR |= ...)
    // 2. Configure GPIO AF (HalGPIO_Init)
    // 3. Configure Timing (inst->regs->CCR = ...)
    // 4. Enable Peripheral (inst->regs->CR1 |= I2C_CR1_PE)

    return (HalI2cHandle)inst;
}

HalI2cStatus_t HalI2c_MasterTransmit(HalI2cHandle handle, uint16_t dev_address, const uint8_t* data, size_t length, uint32_t timeout_ms) {
    if (!handle || !data || length == 0) return HalI2C_ERR_INVALID_ARGS;
    
    // Polling based master transmit
    // 1. START
    // 2. ADDR + W
    // 3. Data loop
    // 4. STOP
    
    return HalI2C_OK; // Simplified for course demo
}

HalI2cStatus_t HalI2c_MasterReceive(HalI2cHandle handle, uint16_t dev_address, uint8_t* data, size_t length, uint32_t timeout_ms) {
    if (!handle || !data || length == 0) return HalI2C_ERR_INVALID_ARGS;
    
    // 1. START
    // 2. ADDR + R
    // 3. Data loop with ACK/NACK
    // 4. STOP
    
    return HalI2C_OK;
}

HalI2cStatus_t HalI2c_PerformBusRecovery(HalI2cHandle handle) {
    if (!handle) return HalI2C_ERR_INVALID_ARGS;
    // Toggling GPIO logic would go here
    return HalI2C_OK;
}

void HalI2c_DeInit(HalI2cHandle handle) {
    if (handle) {
        if (handle->regs) {
            handle->regs->CR1 &= ~I2C_CR1_PE;
        }
        handle->is_allocated = false;
    }
}

// Stubs for slave mode as requested by header
HalI2cStatus_t HalI2c_SlaveTransmit(HalI2cHandle handle, const uint8_t* data, size_t length, uint32_t timeout_ms) { (void)handle; (void)data; (void)length; (void)timeout_ms; return HalI2C_OK; }
HalI2cStatus_t HalI2c_SlaveReceive(HalI2cHandle handle, uint8_t* data, size_t length, uint32_t timeout_ms) { (void)handle; (void)data; (void)length; (void)timeout_ms; return HalI2C_OK; }
