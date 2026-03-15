/**
 * @file Hali2c.h
 * @brief Hardware Abstraction Layer for I2C (Inter-Integrated Circuit).
 * 
 * @details
 * ARCHITECTURE DECISION: PIMPL (Opaque Pointer) Pattern
 * To ensure this HAL remains stable across 20 years of microcontroller changes,
 * the application layer must never see the internal representation of an I2C peripheral.
 * The `HalI2cHandle` hides all hardware-specific register pointers or vendor HAL structs.
 *
 * ARCHITECTURE DECISION: Configuration Structs
 * Passing a single configuration struct to `HalI2c_Init` allows us to add new configuration
 * options in the future (e.g., SMBus support) without breaking the API signature.
 */

#ifndef HalI2C_H
#define HalI2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** @brief Opaque handle for I2C instances. */
typedef struct HalI2c_s* HalI2cHandle;

/** @brief I2C speed modes. */
typedef enum {
    HalI2C_SPEED_STANDARD = 100000, /**< 100 kHz */
    HalI2C_SPEED_FAST     = 400000, /**< 400 kHz */
    HalI2C_SPEED_FAST_PLUS= 1000000 /**< 1 MHz */
} HalI2cSpeed_t;

/** @brief I2C addressing modes. */
typedef enum {
    HalI2C_ADDRESSING_7BIT,
    HalI2C_ADDRESSING_10BIT
} HalI2cAddressing_t;

/** @brief I2C configuration structure. */
typedef struct {
    uint8_t i2c_bus_id;           /**< Physical bus ID (e.g., 1 for I2C1) */
    HalI2cSpeed_t speed;          /**< Bus speed */
    HalI2cAddressing_t addr_mode; /**< 7-bit or 10-bit addressing */
    bool is_slave;                /**< True if operating as slave */
    uint16_t slave_address;       /**< Own address if operating as slave */
    bool enable_clock_stretch;    /**< Enable clock stretching (slave mode) */
} HalI2cConfig_t;

/** @brief I2C transaction status codes. */
typedef enum {
    HalI2C_OK = 0,
    HalI2C_ERR_NACK,
    HalI2C_ERR_TIMEOUT,
    HalI2C_ERR_BUS_BUSY,
    HalI2C_ERR_ARBITRATION_LOST,
    HalI2C_ERR_INVALID_ARGS
} HalI2cStatus_t;

/**
 * @brief Initialize an I2C instance.
 * @param config Pointer to the configuration structure.
 * @return HalI2cHandle Valid handle on success, NULL on failure (e.g., no free instances).
 */
HalI2cHandle HalI2c_Init(const HalI2cConfig_t* config);

/**
 * @brief Transmit data as a master.
 * @param handle The I2C handle.
 * @param dev_address The slave device address.
 * @param data Pointer to data to transmit.
 * @param length Number of bytes to transmit.
 * @param timeout_ms Timeout in milliseconds.
 * @return HalI2cStatus_t Transaction status.
 */
HalI2cStatus_t HalI2c_MasterTransmit(HalI2cHandle handle, uint16_t dev_address, const uint8_t* data, size_t length, uint32_t timeout_ms);

/**
 * @brief Receive data as a master.
 * @param handle The I2C handle.
 * @param dev_address The slave device address.
 * @param data Pointer to buffer for received data.
 * @param length Number of bytes to receive.
 * @param timeout_ms Timeout in milliseconds.
 * @return HalI2cStatus_t Transaction status.
 */
HalI2cStatus_t HalI2c_MasterReceive(HalI2cHandle handle, uint16_t dev_address, uint8_t* data, size_t length, uint32_t timeout_ms);

/**
 * @brief Transmit data as a slave.
 * @param handle The I2C handle.
 * @param data Pointer to data to transmit.
 * @param length Number of bytes to transmit.
 * @param timeout_ms Timeout in milliseconds.
 * @return HalI2cStatus_t Transaction status.
 */
HalI2cStatus_t HalI2c_SlaveTransmit(HalI2cHandle handle, const uint8_t* data, size_t length, uint32_t timeout_ms);

/**
 * @brief Receive data as a slave.
 * @param handle The I2C handle.
 * @param data Pointer to buffer for received data.
 * @param length Number of bytes to receive.
 * @param timeout_ms Timeout in milliseconds.
 * @return HalI2cStatus_t Transaction status.
 */
HalI2cStatus_t HalI2c_SlaveReceive(HalI2cHandle handle, uint8_t* data, size_t length, uint32_t timeout_ms);

/**
 * @brief Perform a bus recovery sequence.
 * @details Toggles SCL 9 times and generates a STOP condition to free a stuck bus.
 * @param handle The I2C handle.
 * @return HalI2cStatus_t HalI2C_OK if bus is recovered, error otherwise.
 */
HalI2cStatus_t HalI2c_PerformBusRecovery(HalI2cHandle handle);

/**
 * @brief Deinitialize the I2C instance and return it to the pool.
 * @param handle The I2C handle.
 */
void HalI2c_DeInit(HalI2cHandle handle);

#endif /* HalI2C_H */
