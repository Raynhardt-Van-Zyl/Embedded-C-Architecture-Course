/**
 * @file Haldma.h
 * @brief Hardware Abstraction Layer for Direct Memory Access (DMA).
 *
 * @details
 * ARCHITECTURE DECISION: Cache Coherency
 * On modern microcontrollers with data caches (e.g., Cortex-M7), DMA transfers 
 * can lead to cache incoherency. This API explicitly provides stubs for cache 
 * invalidation and cleaning, enforcing safe memory operations at the HAL level.
 */

#ifndef HalDMA_H
#define HalDMA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** @brief Opaque handle for DMA instances. */
typedef struct HalDma_s* HalDmaHandle;

/** @brief DMA Data Transfer Direction. */
typedef enum {
    HalDMA_DIR_PERIPH_TO_MEM,
    HalDMA_DIR_MEM_TO_PERIPH,
    HalDMA_DIR_MEM_TO_MEM
} HalDmaDirection_t;

/** @brief DMA Data Size. */
typedef enum {
    HalDMA_SIZE_BYTE = 1,
    HalDMA_SIZE_HALF_WORD = 2,
    HalDMA_SIZE_WORD = 4
} HalDmaDataSize_t;

/** @brief DMA Callback Types. */
typedef void (*HalDmaCallback_t)(void* context);

/** @brief DMA configuration structure. */
typedef struct {
    uint8_t controller_id;        /**< DMA controller (e.g., 1 or 2) */
    uint8_t stream_id;            /**< DMA Stream or Channel ID */
    uint8_t request_id;           /**< Peripheral request ID / Slot */
    HalDmaDirection_t direction;  /**< Transfer direction */
    HalDmaDataSize_t src_size;    /**< Source data width */
    HalDmaDataSize_t dst_size;    /**< Destination data width */
    bool src_inc;                 /**< Increment source address */
    bool dst_inc;                 /**< Increment destination address */
    bool circular_mode;           /**< Enable circular buffering */
    HalDmaCallback_t cb_complete; /**< Transfer complete callback */
    HalDmaCallback_t cb_error;    /**< Transfer error callback */
    void* cb_context;             /**< Callback context */
} HalDmaConfig_t;

/**
 * @brief Initialize a DMA stream/channel.
 * @param config Pointer to the configuration struct.
 * @return HalDmaHandle Valid handle or NULL.
 */
HalDmaHandle HalDma_Init(const HalDmaConfig_t* config);

/**
 * @brief Start a DMA transfer.
 * @param handle The DMA handle.
 * @param src_addr Source memory/register address.
 * @param dst_addr Destination memory/register address.
 * @param length Number of items (based on peripheral data size) to transfer.
 * @return true on success.
 */
bool HalDma_Start(HalDmaHandle handle, uint32_t src_addr, uint32_t dst_addr, uint32_t length);

/**
 * @brief Stop a DMA transfer.
 * @param handle The DMA handle.
 */
void HalDma_Stop(HalDmaHandle handle);

/**
 * @brief Invalidate Data Cache for a specific memory region before a DMA Read (Peripheral to Memory).
 * @param addr Start address of the buffer.
 * @param size Size in bytes.
 */
void HalDma_InvalidateCache(void* addr, size_t size);

/**
 * @brief Clean Data Cache for a specific memory region before a DMA Write (Memory to Peripheral).
 * @param addr Start address of the buffer.
 * @param size Size in bytes.
 */
void HalDma_CleanCache(void* addr, size_t size);

/**
 * @brief Deinitialize the DMA stream/channel.
 * @param handle The DMA handle.
 */
void HalDma_DeInit(HalDmaHandle handle);

#endif /* HalDMA_H */
