/**
 * @file hal_spi.c
 * @brief Example implementation of the architectural SPI HAL.
 */

#include "hal_spi.h"

#define HAL_SPI_MAX_INSTANCES 4U

struct HalSpi_s {
    bool allocated;
    HalSpiConfig_t config;
};

static struct HalSpi_s g_spi_pool[HAL_SPI_MAX_INSTANCES] = {0};

HalSpiHandle HalSpi_Init(const HalSpiConfig_t* config)
{
    uint8_t i;

    if ((config == NULL) || (config->bus_id >= HAL_SPI_MAX_INSTANCES) || (config->frequency_hz == 0U)) {
        return NULL;
    }

    for (i = 0U; i < HAL_SPI_MAX_INSTANCES; i++) {
        if (!g_spi_pool[i].allocated) {
            g_spi_pool[i].allocated = true;
            g_spi_pool[i].config = *config;

            /* Target-specific setup belongs here:
             * - enable peripheral clocks
             * - configure GPIO alternate functions
             * - apply clock polarity/phase and frequency dividers
             * - configure DMA descriptors if requested
             */
            return &g_spi_pool[i];
        }
    }

    return NULL;
}

HalSpiStatus_t HalSpi_Transmit(HalSpiHandle handle, const uint8_t* tx_data, size_t length, uint32_t timeout_ms)
{
    (void)timeout_ms;

    if ((handle == NULL) || (tx_data == NULL) || (length == 0U)) {
        return HAL_SPI_ERR_INVALID_ARGS;
    }

    return HAL_SPI_OK;
}

HalSpiStatus_t HalSpi_Receive(HalSpiHandle handle, uint8_t* rx_data, size_t length, uint32_t timeout_ms)
{
    (void)timeout_ms;

    if ((handle == NULL) || (rx_data == NULL) || (length == 0U)) {
        return HAL_SPI_ERR_INVALID_ARGS;
    }

    return HAL_SPI_OK;
}

HalSpiStatus_t HalSpi_Transfer(HalSpiHandle handle,
                               const uint8_t* tx_data,
                               uint8_t* rx_data,
                               size_t length,
                               uint32_t timeout_ms)
{
    (void)timeout_ms;

    if ((handle == NULL) || (length == 0U)) {
        return HAL_SPI_ERR_INVALID_ARGS;
    }

    if ((tx_data == NULL) && (rx_data == NULL)) {
        return HAL_SPI_ERR_INVALID_ARGS;
    }

    return HAL_SPI_OK;
}

void HalSpi_DeInit(HalSpiHandle handle)
{
    if (handle != NULL) {
        handle->allocated = false;
    }
}
