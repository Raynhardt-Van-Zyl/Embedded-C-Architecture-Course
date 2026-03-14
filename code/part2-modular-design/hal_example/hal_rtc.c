/**
 * @file hal_rtc.c
 * @brief Implementation of the RTC Hardware Abstraction Layer.
 */

#include "hal_rtc.h"
#include <stddef.h>

#define MAX_RTC_INSTANCES 1

struct HalRtc_s {
    bool is_allocated;
    HalRtcConfig_t config;
    HalRtcAlarmCallback_t alarm_cb;
    void* alarm_context;
};

static struct HalRtc_s g_rtc_pool[MAX_RTC_INSTANCES] = {0};

HalRtcHandle HalRtc_Init(const HalRtcConfig_t* config) {
    if (!config) return NULL;
    
    struct HalRtc_s* inst = &g_rtc_pool[0];
    if (inst->is_allocated) return (HalRtcHandle)inst; /* Return existing if already init */

    inst->is_allocated = true;
    inst->config = *config;

    /**
     * ARCHITECTURE DECISION: Backup Domain Access
     * On many architectures, the RTC is in a separate power domain to retain time on Vbat.
     * Write access requires unlocking the Backup Domain (e.g., DBP bit in PWR_CR).
     */
    /* TODO: Unlock Backup Domain, Enable LSI/LSE, wait for ready, select RTC clock source */

    return (HalRtcHandle)inst;
}

bool HalRtc_SetTime(HalRtcHandle handle, uint32_t unix_timestamp) {
    if (!handle) return false;

    /**
     * TODO: Convert Unix timestamp to Year, Month, Day, Hour, Min, Sec.
     * Convert those values to BCD format.
     * Unlock RTC registers (e.g., write 0xCA, 0x53 to WPR).
     * Enter Init mode.
     * Write TR (Time Register) and DR (Date Register).
     * Exit Init mode.
     */
    (void)unix_timestamp;
    return true;
}

uint32_t HalRtc_GetTime(HalRtcHandle handle, uint32_t* out_subseconds) {
    if (!handle) return 0;
    
    /* TODO: Read SSR (Sub-second), TR, and DR registers. 
       Note: Read SSR then TR then DR to lock values and prevent rollover mid-read. 
       Convert BCD back to Unix timestamp. */
       
    if (out_subseconds) {
        *out_subseconds = 0; /* Stub */
    }
    
    return 0; /* Stub */
}

bool HalRtc_SetAlarm(HalRtcHandle handle, uint32_t unix_timestamp) {
    if (!handle) return false;
    /* TODO: Convert timestamp, set Alarm Registers (ALRMAR), enable Alarm Interrupt (ALRAIE) */
    (void)unix_timestamp;
    return true;
}

void HalRtc_RegisterAlarmCallback(HalRtcHandle handle, HalRtcAlarmCallback_t cb, void* context) {
    if (handle) {
        handle->alarm_cb = cb;
        handle->alarm_context = context;
    }
}

uint32_t HalRtc_ReadBackupRegister(HalRtcHandle handle, uint8_t reg_index) {
    if (!handle) return 0;
    /* TODO: Return value from RTC_BKPxR */
    (void)reg_index;
    return 0;
}

void HalRtc_WriteBackupRegister(HalRtcHandle handle, uint8_t reg_index, uint32_t value) {
    if (!handle) return;
    /* TODO: Write value to RTC_BKPxR */
    (void)reg_index;
    (void)value;
}

void HalRtc_DeInit(HalRtcHandle handle) {
    if (handle) {
        /* TODO: Reset RTC backup domain if desired, or leave alone to keep time */
        handle->is_allocated = false;
    }
}
