/**
 * @file Haladc.h
 * @brief Hardware Abstraction Layer for ADC (Analog to Digital Converter).
 *
 * @details
 * ARCHITECTURE DECISION: Abstraction of Complex Modes
 * Modern ADCs have injected channels, regular channels, circular DMA buffers,
 * and complex trigger sources. This API abstracts these into logical, independent
 * modes configurable via strongly-typed structs to prevent invalid combinations.
 */

#ifndef HalADC_H
#define HalADC_H

#include <stdint.h>
#include <stdbool.h>

/** @brief Opaque handle for ADC instances. */
typedef struct HalAdc_s* HalAdcHandle;

/** @brief ADC resolution. */
typedef enum {
    HalADC_RES_8BIT,
    HalADC_RES_10BIT,
    HalADC_RES_12BIT,
    HalADC_RES_16BIT
} HalAdcResolution_t;

/** @brief ADC alignment. */
typedef enum {
    HalADC_ALIGN_RIGHT,
    HalADC_ALIGN_LEFT
} HalAdcAlign_t;

/** @brief ADC configuration structure. */
typedef struct {
    uint8_t adc_id;              /**< Physical ADC ID (e.g., 1 for ADC1) */
    HalAdcResolution_t res;      /**< Resolution of conversions */
    HalAdcAlign_t align;         /**< Data alignment */
    bool continuous_conv_mode;   /**< True for continuous, False for single-shot */
} HalAdcConfig_t;

/** @brief ADC Channel configuration structure. */
typedef struct {
    uint8_t channel_id;          /**< The physical analog channel */
    uint8_t rank;                /**< Sequence rank */
    uint32_t sample_time_cycles; /**< Sampling time in clock cycles */
    bool is_injected;            /**< True if this is an injected (high priority) channel */
} HalAdcChannelConfig_t;

/**
 * @brief Initialize an ADC instance.
 * @param config Pointer to configuration struct.
 * @return HalAdcHandle Valid handle or NULL.
 */
HalAdcHandle HalAdc_Init(const HalAdcConfig_t* config);

/**
 * @brief Configure a specific channel for the ADC.
 * @param handle The ADC handle.
 * @param ch_config The channel configuration.
 * @return true if successful, false otherwise.
 */
bool HalAdc_ConfigChannel(HalAdcHandle handle, const HalAdcChannelConfig_t* ch_config);

/**
 * @brief Start ADC conversion (Software Trigger).
 * @param handle The ADC handle.
 * @return true if successfully started.
 */
bool HalAdc_Start(HalAdcHandle handle);

/**
 * @brief Stop ADC conversion.
 * @param handle The ADC handle.
 */
void HalAdc_Stop(HalAdcHandle handle);

/**
 * @brief Poll for conversion complete.
 * @param handle The ADC handle.
 * @param timeout_ms Maximum time to wait.
 * @return true if conversion is complete and data is ready.
 */
bool HalAdc_PollForConversion(HalAdcHandle handle, uint32_t timeout_ms);

/**
 * @brief Get the latest converted value.
 * @param handle The ADC handle.
 * @return The digital value.
 */
uint32_t HalAdc_GetValue(HalAdcHandle handle);

/**
 * @brief Get the latest converted value for an injected channel.
 * @param handle The ADC handle.
 * @param injected_rank The rank of the injected channel.
 * @return The digital value.
 */
uint32_t HalAdc_GetInjectedValue(HalAdcHandle handle, uint8_t injected_rank);

/**
 * @brief Start ADC conversion with DMA circular buffering.
 * @param handle The ADC handle.
 * @param buffer Pointer to the destination memory buffer.
 * @param length Number of elements (not bytes) to transfer before repeating.
 * @return true on success.
 */
bool HalAdc_StartDma(HalAdcHandle handle, uint32_t* buffer, uint32_t length);

/**
 * @brief Deinitialize the ADC instance.
 * @param handle The ADC handle.
 */
void HalAdc_DeInit(HalAdcHandle handle);

#endif /* HalADC_H */
