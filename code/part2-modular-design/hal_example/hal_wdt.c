/**
 * @file Halwdt.c
 * @brief Implementation of the WDT Hardware Abstraction Layer.
 */

#include "Halwdt.h"
#include <stddef.h>

#define MAX_WDT_INSTANCES 2 /* Support for 1 IWDG, 1 WWDG */

struct HalWdt_s {
    bool is_allocated;
    HalWdtConfig_t config;
    bool is_windowed;
};

static struct HalWdt_s g_wdt_pool[MAX_WDT_INSTANCES] = {0};

HalWdtHandle HalWdt_Init(const HalWdtConfig_t* config) {
    if (!config || config->timeout_ms == 0) return NULL;

    uint8_t inst_idx = (config->window_ms > 0) ? 1 : 0; /* 1 for WWDG, 0 for IWDG */
    struct HalWdt_s* inst = &g_wdt_pool[inst_idx];
    
    if (inst->is_allocated) return NULL;

    inst->is_allocated = true;
    inst->config = *config;
    inst->is_windowed = (config->window_ms > 0);

    /* TODO: Hardware initialization.
       If IWDG:
       - Write Key to KR to enable access.
       - Set Prescaler (PR) and Reload (RLR) based on timeout_ms and LSI clock.
       - Start watchdog (write 0xCCCC to KR).
       
       If WWDG:
       - Enable WWDG Clock.
       - Calculate Window and Counter values based on timeout_ms, window_ms and APB clock.
       - Write CFR and CR.
       - Enable EWI interrupt if ewi_cb is provided.
    */

    return (HalWdtHandle)inst;
}

void HalWdt_Feed(HalWdtHandle handle) {
    if (!handle) return;

    if (handle->is_windowed) {
        /**
         * ARCHITECTURE DECISION: Safe Feeding
         * For Windowed Watchdogs, feeding the dog while the counter is above the window threshold 
         * will trigger an immediate reset. Application must ensure Feed is called in the safe zone.
         */
        /* TODO: Write reload value to CR register. Make sure not to set WDGA again. */
    } else {
        /* TODO: Write 0xAAAA to IWDG_KR to reload counter */
    }
}
