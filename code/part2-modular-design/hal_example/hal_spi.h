/**
 * @file hal_spi.h
 * @brief Hardware Abstraction Layer for SPI.
 *
 * @details
 * This example demonstrates a silicon-agnostic SPI interface that can be
 * implemented by a vendor wrapper on target hardware and replaced by a mock
 * during host-based tests.
 */

#ifndef HAL_SPI_H
#define HAL_SPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct HalSpi_s* HalSpiHandle;

typedef enum {
    HAL_SPI_MODE_0 = 0,
    HAL_SPI_MODE_1,
    HAL_SPI_MODE_2,
    HAL_SPI_MODE_3
} HalSpiMode_t;

typedef enum {
    HAL_SPI_BIT_ORDER_MSB_FIRST = 0,
    HAL_SPI_BIT_ORDER_LSB_FIRST
} HalSpiBitOrder_t;

typedef enum {
    HAL_SPI_OK = 0,
    HAL_SPI_ERR_INVALID_ARGS,
    HAL_SPI_ERR_BUSY,
    HAL_SPI_ERR_TIMEOUT,
    HAL_SPI_ERR_HW_FAULT
} HalSpiStatus_t;

typedef struct {
    uint8_t bus_id;
    uint32_t frequency_hz;
    HalSpiMode_t mode;
    HalSpiBitOrder_t bit_order;
    bool use_dma;
} HalSpiConfig_t;

HalSpiHandle HalSpi_Init(const HalSpiConfig_t* config);
HalSpiStatus_t HalSpi_Transmit(HalSpiHandle handle, const uint8_t* tx_data, size_t length, uint32_t timeout_ms);
HalSpiStatus_t HalSpi_Receive(HalSpiHandle handle, uint8_t* rx_data, size_t length, uint32_t timeout_ms);
HalSpiStatus_t HalSpi_Transfer(HalSpiHandle handle, const uint8_t* tx_data, uint8_t* rx_data, size_t length, uint32_t timeout_ms);
void HalSpi_DeInit(HalSpiHandle handle);

#endif /* HAL_SPI_H */
