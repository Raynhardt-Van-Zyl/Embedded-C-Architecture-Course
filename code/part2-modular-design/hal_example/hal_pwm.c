/**
 * @file Halpwm.c
 * @brief Implementation of the PWM Hardware Abstraction Layer.
 */

#include "Halpwm.h"
#include <stddef.h>

#define MAX_PWM_CHANNELS 16

struct HalPwm_s {
    bool is_allocated;
    HalPwmConfig_t config;
    uint32_t auto_reload_value; /* Cached ARR for duty cycle math */
};

static struct HalPwm_s g_pwm_pool[MAX_PWM_CHANNELS] = {0};

HalPwmHandle HalPwm_Init(const HalPwmConfig_t* config) {
    if (!config || config->timer_id > 15 || config->channel > 4) return NULL;

    /* Find free slot */
    struct HalPwm_s* inst = NULL;
    for (int i = 0; i < MAX_PWM_CHANNELS; i++) {
        if (!g_pwm_pool[i].is_allocated) {
            inst = &g_pwm_pool[i];
            inst->is_allocated = true;
            break;
        }
    }

    if (!inst) return NULL;

    inst->config = *config;

    /**
     * ARCHITECTURE DECISION: Frequency Calculation
     * Timer base clock is divided by Prescaler (PSC) and Auto-Reload Register (ARR).
     * HAL calculates optimal PSC and ARR to maximize duty cycle resolution at the requested freq.
     */
    /* Pseudo calculation */
    /* uint32_t timer_clk = SystemCoreClock / APB_Divider; */
    /* inst->auto_reload_value = (timer_clk / config->frequency_hz) - 1; */
    inst->auto_reload_value = 1000; /* Mock value */

    /* TODO: Hardware initialization.
       - Enable Timer Clock.
       - Set PSC and ARR.
       - Set CCMR mode to PWM Mode 1.
       - If dead_time_ns > 0, configure BDTR dead-time generator.
    */

    return (HalPwmHandle)inst;
}

void HalPwm_SetDutyCycle(HalPwmHandle handle, float duty_cycle_percent) {
    if (!handle) return;
    
    if (duty_cycle_percent < 0.0f) duty_cycle_percent = 0.0f;
    if (duty_cycle_percent > 100.0f) duty_cycle_percent = 100.0f;

    uint32_t ccr_value = (uint32_t)((duty_cycle_percent / 100.0f) * (float)handle->auto_reload_value);

    /* TODO: Write ccr_value to the specific CCRx register */
    (void)ccr_value;
}

void HalPwm_Start(HalPwmHandle handle) {
    if (!handle) return;
    /* TODO: Enable Capture/Compare output (CCxE) and Main Output Enable (MOE), enable Timer counter (CEN) */
}

void HalPwm_Stop(HalPwmHandle handle) {
    if (!handle) return;
    /* TODO: Clear CCxE, MOE, and CEN */
}

void HalPwm_DeInit(HalPwmHandle handle) {
    if (handle) {
        /* TODO: Reset Timer registers */
        handle->is_allocated = false;
    }
}
