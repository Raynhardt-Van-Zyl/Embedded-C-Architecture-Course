/**
 * @file Haldma.c
 * @brief Implementation of the DMA Hardware Abstraction Layer.
 */

#include "Haldma.h"

#define MAX_DMA_STREAMS 16

struct HalDma_s {
    bool is_allocated;
    HalDmaConfig_t config;
};

static struct HalDma_s g_dma_pool[MAX_DMA_STREAMS] = {0};

HalDmaHandle HalDma_Init(const HalDmaConfig_t* config) {
    if (!config || config->controller_id > 2 || config->stream_id > 7) return NULL;

    uint8_t pool_idx = (config->controller_id * 8) + config->stream_id;
    if (pool_idx >= MAX_DMA_STREAMS) return NULL;

    struct HalDma_s* inst = &g_dma_pool[pool_idx];
    if (inst->is_allocated) return NULL;

    inst->is_allocated = true;
    inst->config = *config;

    /* TODO: Hardware initialization.
       - Enable DMA Controller Clock.
       - Configure stream: Direction, Size, Increment, Priority, Circular.
       - Configure Request selection (Multiplexer).
       - Enable Interrupts in NVIC if callbacks are provided.
    */

    return (HalDmaHandle)inst;
}

bool HalDma_Start(HalDmaHandle handle, uint32_t src_addr, uint32_t dst_addr, uint32_t length) {
    if (!handle) return false;

    /* TODO: 
       - Clear interrupt flags.
       - Write source and destination addresses to registers.
       - Write length (NDTR).
       - Enable stream.
    */
    (void)src_addr;
    (void)dst_addr;
    (void)length;
    return true;
}

void HalDma_Stop(HalDmaHandle handle) {
    if (!handle) return;
    /* TODO: Clear EN bit to stop stream. Wait for EN to clear. */
}

void HalDma_InvalidateCache(void* addr, size_t size) {
    if (!addr || size == 0) return;
    /**
     * ARCHITECTURE DECISION: CMSIS Cache Operations
     * If the core has an L1 D-Cache (Cortex-M7), calling SCB_InvalidateDCache_by_Addr
     * ensures the CPU sees the fresh data DMA just wrote to RAM.
     */
    /* SCB_InvalidateDCache_by_Addr((uint32_t*)addr, size); */
}

void HalDma_CleanCache(void* addr, size_t size) {
    if (!addr || size == 0) return;
    /**
     * ARCHITECTURE DECISION: CMSIS Cache Operations
     * If the core has an L1 D-Cache, calling SCB_CleanDCache_by_Addr
     * flushes CPU writes to RAM before DMA reads it to send to peripheral.
     */
    /* SCB_CleanDCache_by_Addr((uint32_t*)addr, size); */
}

void HalDma_DeInit(HalDmaHandle handle) {
    if (handle) {
        /* TODO: Stop stream, clear registers, disable interrupts */
        handle->is_allocated = false;
    }
}
