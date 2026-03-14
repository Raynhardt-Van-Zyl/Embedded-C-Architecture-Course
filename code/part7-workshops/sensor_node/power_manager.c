/**
 * @file power_manager.c
 * @brief Power Management Implementation - Low-power mode control
 * 
 * @details Implements the power management subsystem with support for multiple
 * low-power modes, wake-up source management, and power consumption monitoring.
 * 
 * Key Features:
 * - State machine for mode transitions
 * - Configurable wake-up sources
 * - Automatic idle timeout management
 * - Battery monitoring and warnings
 * - Power consumption statistics
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#include "power_manager.h"
#include <string.h>

#ifndef __WFI
#define __WFI()  ((void)0)
#endif

/*============================================================================*/
/*                              PRIVATE DEFINES                               */
/*============================================================================*/

/** @brief Default idle timeout before sleep (5 seconds) */
#define PM_DEFAULT_IDLE_TIMEOUT_MS      (5000U)

/** @brief Low battery warning threshold (%) */
#define PM_DEFAULT_WARNING_THRESHOLD    (20U)

/** @brief Low battery critical threshold (%) */
#define PM_DEFAULT_CRITICAL_THRESHOLD   (5U)

/** @brief Tick interval for power management processing */
#define PM_TICK_INTERVAL_MS             (100U)

/** @brief Number of mode transition attempts before error */
#define PM_MAX_TRANSITION_ATTEMPTS      (3U)

/*============================================================================*/
/*                              PRIVATE TYPES                                 */
/*============================================================================*/

/**
 * @brief Internal power manager state
 */
typedef struct {
    PowerMode_t         currentMode;            /**< Current power mode */
    PowerMode_t         pendingMode;            /**< Pending mode transition target */
    PowerMode_t         idleTargetMode;         /**< Target mode for idle timeout */
    PowerTransitionReason_t lastReason;         /**< Last transition reason */
    
    uint32_t            idleTimeoutMs;          /**< Configured idle timeout */
    uint32_t            lastActivityTime;       /**< Last activity timestamp */
    uint32_t            modeEntryTime;          /**< Time current mode was entered */
    
    WakeupConfig_t      wakeupSources[PM_MAX_WAKEUP_SOURCES]; /**< Registered sources */
    uint32_t            enabledWakeupMask;      /**< Enabled wake-up sources bitmask */
    
    PowerStateCallback_t    stateCallbacks[PM_MAX_LISTENERS]; /**< State change callbacks */
    WakeupCallback_t        wakeupCallbacks[PM_MAX_LISTENERS]; /**< Wake-up callbacks */
    LowBatteryCallback_t    batteryCallbacks[PM_MAX_LISTENERS]; /**< Battery callbacks */
    void*                   callbackUserData[PM_MAX_LISTENERS][3]; /**< User data per callback type */
    uint8_t                 stateCallbackCount; /**< Number of registered state callbacks */
    uint8_t                 wakeupCallbackCount;/**< Number of registered wake-up callbacks */
    uint8_t                 batteryCallbackCount;/**< Number of registered battery callbacks */
    
    PowerStats_t        stats;                  /**< Power statistics */
    PowerProfile_t      currentProfile;         /**< Active power profile */
    PowerModeConfig_t   modeConfigs[PM_MODE_COUNT]; /**< Mode configurations */
    
    BatteryInfo_t       battery;                /**< Battery information */
    uint8_t             warningThreshold;       /**< Battery warning threshold */
    uint8_t             criticalThreshold;      /**< Battery critical threshold */
    bool                batteryWarningIssued;   /**< Warning already issued flag */
    bool                initialized;            /**< Subsystem initialized flag */
    bool                transitionInProgress;   /**< Mode transition in progress */
} PowerManager_t;

/*============================================================================*/
/*                              PRIVATE VARIABLES                             */
/*============================================================================*/

/** @brief Power manager instance */
static PowerManager_t s_pm = {0};

/** @brief Default power mode configurations */
static const PowerModeConfig_t s_defaultModeConfigs[PM_MODE_COUNT] = {
    [PM_MODE_ACTIVE] = {
        .mode = PM_MODE_ACTIVE,
        .entryTimeoutMs = 0,
        .exitLatencyMs = 0,
        .typicalCurrent = 10000,
        .wakeupSources = PM_WAKEUP_ALL,
        .retainRam = true,
        .retainRegisters = true,
        .clockScaling = false,
        .clockDivider = 1
    },
    [PM_MODE_LOW_POWER] = {
        .mode = PM_MODE_LOW_POWER,
        .entryTimeoutMs = 0,
        .exitLatencyMs = 1,
        .typicalCurrent = 2000,
        .wakeupSources = PM_WAKEUP_ALL,
        .retainRam = true,
        .retainRegisters = true,
        .clockScaling = true,
        .clockDivider = 2
    },
    [PM_MODE_SLEEP] = {
        .mode = PM_MODE_SLEEP,
        .entryTimeoutMs = 0,
        .exitLatencyMs = 10,
        .typicalCurrent = 500,
        .wakeupSources = PM_WAKEUP_GPIO | PM_WAKEUP_RTC_ALARM | PM_WAKEUP_UART | PM_WAKEUP_TIMER,
        .retainRam = true,
        .retainRegisters = true,
        .clockScaling = true,
        .clockDivider = 4
    },
    [PM_MODE_DEEP_SLEEP] = {
        .mode = PM_MODE_DEEP_SLEEP,
        .entryTimeoutMs = 0,
        .exitLatencyMs = 100,
        .typicalCurrent = 50,
        .wakeupSources = PM_WAKEUP_GPIO | PM_WAKEUP_RTC_ALARM | PM_WAKEUP_TIMER,
        .retainRam = true,
        .retainRegisters = false,
        .clockScaling = false,
        .clockDivider = 1
    },
    [PM_MODE_STANDBY] = {
        .mode = PM_MODE_STANDBY,
        .entryTimeoutMs = 0,
        .exitLatencyMs = 500,
        .typicalCurrent = 5,
        .wakeupSources = PM_WAKEUP_GPIO | PM_WAKEUP_RTC_ALARM,
        .retainRam = false,
        .retainRegisters = false,
        .clockScaling = false,
        .clockDivider = 1
    },
    [PM_MODE_SHUTDOWN] = {
        .mode = PM_MODE_SHUTDOWN,
        .entryTimeoutMs = 0,
        .exitLatencyMs = 5000,
        .typicalCurrent = 1,
        .wakeupSources = PM_WAKEUP_GPIO,
        .retainRam = false,
        .retainRegisters = false,
        .clockScaling = false,
        .clockDivider = 1
    }
};

/** @brief Default power profile */
static const PowerProfile_t s_defaultProfile = {
    .name = "default",
    .defaultMode = PM_MODE_ACTIVE,
    .idleTimeoutMs = PM_DEFAULT_IDLE_TIMEOUT_MS,
    .idleTargetMode = PM_MODE_SLEEP,
    .adaptiveBrightness = false,
    .cpuScalingEnabled = true,
    .peripheralPowerGating = true,
    .wakeupSourceMask = 0xFF
};

/*============================================================================*/
/*                              PRIVATE FUNCTIONS                             */
/*============================================================================*/

/**
 * @brief Execute power mode transition
 * 
 * @details Performs the hardware-specific operations to enter a new power mode.
 * This is a simulation - real implementation would configure MCU registers.
 * 
 * @param newMode Target power mode
 * @return PM_OK on success, error code otherwise
 */
static PowerStatus_t Power_ExecuteTransition(PowerMode_t newMode)
{
    PowerModeConfig_t *config = &s_pm.modeConfigs[newMode];
    
    switch (newMode) {
        case PM_MODE_ACTIVE:
            break;
            
        case PM_MODE_LOW_POWER:
            break;
            
        case PM_MODE_SLEEP:
            __WFI();
            break;
            
        case PM_MODE_DEEP_SLEEP:
            __WFI();
            break;
            
        case PM_MODE_STANDBY:
            __WFI();
            break;
            
        case PM_MODE_SHUTDOWN:
            break;
            
        default:
            return PM_ERROR_INVALID_MODE;
    }
    
    return PM_OK;
}

/**
 * @brief Notify state change callbacks
 * 
 * @param oldMode   Previous power mode
 * @param newMode   New power mode
 * @param reason    Transition reason
 */
static void Power_NotifyStateChange(PowerMode_t oldMode, PowerMode_t newMode, 
                                     PowerTransitionReason_t reason)
{
    for (uint8_t i = 0; i < s_pm.stateCallbackCount; i++) {
        if (s_pm.stateCallbacks[i] != NULL) {
            s_pm.stateCallbacks[i](oldMode, newMode, reason, 
                                   s_pm.callbackUserData[i][0]);
        }
    }
}

/**
 * @brief Notify wake-up callbacks
 * 
 * @param source Wake-up source that triggered the event
 */
static void Power_NotifyWakeup(WakeupSource_t source)
{
    for (uint8_t i = 0; i < s_pm.wakeupCallbackCount; i++) {
        if (s_pm.wakeupCallbacks[i] != NULL) {
            s_pm.wakeupCallbacks[i](source, s_pm.callbackUserData[i][1]);
        }
    }
}

/**
 * @brief Notify battery warning callbacks
 * 
 * @param level     Current battery level
 * @param status    Battery status
 */
static void Power_NotifyBatteryWarning(uint8_t level, BatteryStatus_t status)
{
    for (uint8_t i = 0; i < s_pm.batteryCallbackCount; i++) {
        if (s_pm.batteryCallbacks[i] != NULL) {
            s_pm.batteryCallbacks[i](level, status, s_pm.callbackUserData[i][2]);
        }
    }
}

/**
 * @brief Update power statistics for mode change
 * 
 * @param oldMode       Previous mode
 * @param newMode       New mode
 * @param currentTimeMs Current timestamp
 */
static void Power_UpdateStatsForTransition(PowerMode_t oldMode, PowerMode_t newMode,
                                            uint32_t currentTimeMs)
{
    if (oldMode < PM_MODE_COUNT && s_pm.modeEntryTime > 0) {
        uint32_t timeInMode = currentTimeMs - s_pm.modeEntryTime;
        s_pm.stats.timeInMode[oldMode] += timeInMode;
    }
    
    if (newMode < PM_MODE_COUNT) {
        s_pm.stats.transitionCount[newMode]++;
    }
    
    s_pm.stats.lastSleepTime = currentTimeMs;
    s_pm.modeEntryTime = currentTimeMs;
}

/**
 * @brief Check and update battery status
 * 
 * @param currentTimeMs Current timestamp
 */
static void Power_CheckBattery(uint32_t currentTimeMs)
{
    (void)currentTimeMs;
    
    if (!s_pm.battery.present) {
        return;
    }
    
    BatteryStatus_t oldStatus = s_pm.battery.status;
    
    if (s_pm.battery.currentMa > 0) {
        s_pm.battery.status = PM_BATTERY_CHARGING;
    } else if (s_pm.battery.currentMa < 0) {
        s_pm.battery.status = PM_BATTERY_DISCHARGING;
    }
    
    if (s_pm.battery.levelPercent <= s_pm.criticalThreshold) {
        s_pm.battery.status = PM_BATTERY_CRITICAL;
    } else if (s_pm.battery.levelPercent <= s_pm.warningThreshold) {
        s_pm.battery.status = PM_BATTERY_LOW;
    }
    
    if (oldStatus != s_pm.battery.status && 
        (s_pm.battery.status == PM_BATTERY_LOW || 
         s_pm.battery.status == PM_BATTERY_CRITICAL)) {
        if (!s_pm.batteryWarningIssued) {
            Power_NotifyBatteryWarning(s_pm.battery.levelPercent, s_pm.battery.status);
            s_pm.batteryWarningIssued = true;
        }
    }
    
    if (s_pm.battery.levelPercent > s_pm.warningThreshold + 5) {
        s_pm.batteryWarningIssued = false;
    }
}

/**
 * @brief Process idle timeout logic
 * 
 * @param currentTimeMs Current timestamp
 */
static void Power_ProcessIdleTimeout(uint32_t currentTimeMs)
{
    if (s_pm.idleTimeoutMs == 0 || s_pm.idleTargetMode == s_pm.currentMode) {
        return;
    }
    
    uint32_t idleTime = currentTimeMs - s_pm.lastActivityTime;
    
    if (idleTime >= s_pm.idleTimeoutMs && 
        s_pm.currentMode == PM_MODE_ACTIVE &&
        !s_pm.transitionInProgress) {
        Power_RequestMode(s_pm.idleTargetMode, PM_REASON_TIMEOUT);
    }
}

/*============================================================================*/
/*                              PUBLIC API IMPLEMENTATION                     */
/*============================================================================*/

PowerStatus_t Power_Init(void)
{
    if (s_pm.initialized) {
        return PM_OK;
    }
    
    memset(&s_pm, 0, sizeof(PowerManager_t));
    
    s_pm.currentMode = PM_MODE_ACTIVE;
    s_pm.pendingMode = PM_MODE_ACTIVE;
    s_pm.idleTargetMode = PM_MODE_SLEEP;
    s_pm.idleTimeoutMs = PM_DEFAULT_IDLE_TIMEOUT_MS;
    s_pm.warningThreshold = PM_DEFAULT_WARNING_THRESHOLD;
    s_pm.criticalThreshold = PM_DEFAULT_CRITICAL_THRESHOLD;
    
    memcpy(s_pm.modeConfigs, s_defaultModeConfigs, sizeof(s_pm.modeConfigs));
    memcpy(&s_pm.currentProfile, &s_defaultProfile, sizeof(PowerProfile_t));
    
    s_pm.battery.present = true;
    s_pm.battery.levelPercent = 100;
    s_pm.battery.status = PM_BATTERY_HIGH;
    s_pm.battery.voltageMv = 4200;
    
    s_pm.initialized = true;
    s_pm.stats.lastSleepTime = 0;
    s_pm.modeEntryTime = 0;
    
    return PM_OK;
}

void Power_Deinit(void)
{
    if (!s_pm.initialized) {
        return;
    }
    
    memset(&s_pm, 0, sizeof(PowerManager_t));
}

PowerStatus_t Power_GetMode(PowerMode_t *pMode)
{
    if (pMode == NULL) {
        return PM_ERROR;
    }
    
    *pMode = s_pm.currentMode;
    return PM_OK;
}

PowerStatus_t Power_RequestMode(PowerMode_t newMode, PowerTransitionReason_t reason)
{
    if (!s_pm.initialized) {
        return PM_ERROR_NOT_INITIALIZED;
    }
    
    if (newMode >= PM_MODE_COUNT) {
        return PM_ERROR_INVALID_MODE;
    }
    
    if (s_pm.transitionInProgress) {
        return PM_ERROR_BUSY;
    }
    
    if (newMode == s_pm.currentMode) {
        return PM_OK;
    }
    
    PowerMode_t oldMode = s_pm.currentMode;
    Power_NotifyStateChange(oldMode, newMode, reason);
    
    s_pm.transitionInProgress = true;
    PowerStatus_t status = Power_ExecuteTransition(newMode);
    s_pm.transitionInProgress = false;
    
    if (status == PM_OK) {
        uint32_t currentTime = 0;
        Power_UpdateStatsForTransition(oldMode, newMode, currentTime);
        
        s_pm.currentMode = newMode;
        s_pm.lastReason = reason;
        
        if (newMode == PM_MODE_ACTIVE) {
            s_pm.stats.wakeupCount++;
            s_pm.stats.lastWakeupTime = currentTime;
        }
    }
    
    return status;
}

PowerStatus_t Power_ForceMode(PowerMode_t newMode, PowerTransitionReason_t reason)
{
    s_pm.transitionInProgress = false;
    return Power_RequestMode(newMode, reason);
}

PowerStatus_t Power_RegisterWakeupSource(const WakeupConfig_t *config)
{
    if (config == NULL) {
        return PM_ERROR;
    }
    
    for (uint8_t i = 0; i < PM_MAX_WAKEUP_SOURCES; i++) {
        if (s_pm.wakeupSources[i].source == PM_WAKEUP_NONE) {
            s_pm.wakeupSources[i] = *config;
            if (config->enabled) {
                s_pm.enabledWakeupMask |= config->source;
            }
            return PM_OK;
        }
    }
    
    return PM_ERROR;
}

PowerStatus_t Power_UnregisterWakeupSource(WakeupSource_t source, uint8_t instance)
{
    for (uint8_t i = 0; i < PM_MAX_WAKEUP_SOURCES; i++) {
        if (s_pm.wakeupSources[i].source == source && 
            s_pm.wakeupSources[i].instance == instance) {
            s_pm.enabledWakeupMask &= ~source;
            s_pm.wakeupSources[i].source = PM_WAKEUP_NONE;
            return PM_OK;
        }
    }
    return PM_ERROR_WAKEUP_SOURCE;
}

PowerStatus_t Power_SetWakeupEnabled(WakeupSource_t source, uint8_t instance, bool enable)
{
    for (uint8_t i = 0; i < PM_MAX_WAKEUP_SOURCES; i++) {
        if (s_pm.wakeupSources[i].source == source && 
            s_pm.wakeupSources[i].instance == instance) {
            s_pm.wakeupSources[i].enabled = enable;
            if (enable) {
                s_pm.enabledWakeupMask |= source;
            } else {
                s_pm.enabledWakeupMask &= ~source;
            }
            return PM_OK;
        }
    }
    return PM_ERROR_WAKEUP_SOURCE;
}

PowerStatus_t Power_GetWakeupSources(uint32_t *pSources)
{
    if (pSources == NULL) {
        return PM_ERROR;
    }
    *pSources = s_pm.enabledWakeupMask;
    return PM_OK;
}

PowerStatus_t Power_EnterLowPower(PowerMode_t mode, WakeupSource_t *pSource)
{
    PowerStatus_t status = Power_RequestMode(mode, PM_REASON_USER_REQUEST);
    
    if (status == PM_OK && pSource != NULL) {
        *pSource = PM_WAKEUP_RTC_ALARM;
        Power_NotifyWakeup(*pSource);
    }
    
    return status;
}

PowerStatus_t Power_SetIdleTimeout(uint32_t timeoutMs, PowerMode_t targetMode)
{
    if (targetMode >= PM_MODE_COUNT) {
        return PM_ERROR_INVALID_MODE;
    }
    
    s_pm.idleTimeoutMs = timeoutMs;
    s_pm.idleTargetMode = targetMode;
    
    return PM_OK;
}

void Power_ResetIdleTimer(void)
{
    s_pm.lastActivityTime = 0;
}

PowerStatus_t Power_RegisterStateCallback(PowerStateCallback_t callback, void *userData)
{
    if (callback == NULL || s_pm.stateCallbackCount >= PM_MAX_LISTENERS) {
        return PM_ERROR;
    }
    
    s_pm.stateCallbacks[s_pm.stateCallbackCount] = callback;
    s_pm.callbackUserData[s_pm.stateCallbackCount][0] = userData;
    s_pm.stateCallbackCount++;
    
    return PM_OK;
}

PowerStatus_t Power_RegisterWakeupCallback(WakeupCallback_t callback, void *userData)
{
    if (callback == NULL || s_pm.wakeupCallbackCount >= PM_MAX_LISTENERS) {
        return PM_ERROR;
    }
    
    s_pm.wakeupCallbacks[s_pm.wakeupCallbackCount] = callback;
    s_pm.callbackUserData[s_pm.wakeupCallbackCount][1] = userData;
    s_pm.wakeupCallbackCount++;
    
    return PM_OK;
}

PowerStatus_t Power_RegisterLowBatteryCallback(LowBatteryCallback_t callback, void *userData)
{
    if (callback == NULL || s_pm.batteryCallbackCount >= PM_MAX_LISTENERS) {
        return PM_ERROR;
    }
    
    s_pm.batteryCallbacks[s_pm.batteryCallbackCount] = callback;
    s_pm.callbackUserData[s_pm.batteryCallbackCount][2] = userData;
    s_pm.batteryCallbackCount++;
    
    return PM_OK;
}

PowerStatus_t Power_GetStats(PowerStats_t *pStats)
{
    if (pStats == NULL) {
        return PM_ERROR;
    }
    *pStats = s_pm.stats;
    return PM_OK;
}

PowerStatus_t Power_ClearStats(void)
{
    memset(&s_pm.stats, 0, sizeof(PowerStats_t));
    return PM_OK;
}

PowerStatus_t Power_GetBatteryInfo(BatteryInfo_t *pInfo)
{
    if (pInfo == NULL) {
        return PM_ERROR;
    }
    *pInfo = s_pm.battery;
    return PM_OK;
}

PowerStatus_t Power_SetBatteryThresholds(uint8_t warningLevel, uint8_t criticalLevel)
{
    if (warningLevel > 100 || criticalLevel > 100 || warningLevel <= criticalLevel) {
        return PM_ERROR;
    }
    
    s_pm.warningThreshold = warningLevel;
    s_pm.criticalThreshold = criticalLevel;
    
    return PM_OK;
}

PowerStatus_t Power_SetProfile(const PowerProfile_t *profile)
{
    if (profile == NULL) {
        return PM_ERROR;
    }
    
    s_pm.currentProfile = *profile;
    s_pm.idleTimeoutMs = profile->idleTimeoutMs;
    s_pm.idleTargetMode = profile->idleTargetMode;
    
    return PM_OK;
}

PowerStatus_t Power_GetProfile(PowerProfile_t *pProfile)
{
    if (pProfile == NULL) {
        return PM_ERROR;
    }
    *pProfile = s_pm.currentProfile;
    return PM_OK;
}

PowerStatus_t Power_ConfigureMode(const PowerModeConfig_t *config)
{
    if (config == NULL || config->mode >= PM_MODE_COUNT) {
        return PM_ERROR;
    }
    
    s_pm.modeConfigs[config->mode] = *config;
    return PM_OK;
}

PowerStatus_t Power_IsModeAvailable(PowerMode_t mode, bool *pAvailable)
{
    if (pAvailable == NULL || mode >= PM_MODE_COUNT) {
        return PM_ERROR;
    }
    
    *pAvailable = (s_pm.modeConfigs[mode].typicalCurrent > 0);
    return PM_OK;
}

PowerStatus_t Power_GetModeCurrent(PowerMode_t mode, uint32_t *pCurrent)
{
    if (pCurrent == NULL || mode >= PM_MODE_COUNT) {
        return PM_ERROR;
    }
    
    *pCurrent = s_pm.modeConfigs[mode].typicalCurrent;
    return PM_OK;
}

void Power_Process(uint32_t currentTimeMs)
{
    if (!s_pm.initialized) {
        return;
    }
    
    s_pm.stats.totalUptime += PM_TICK_INTERVAL_MS;
    
    Power_ProcessIdleTimeout(currentTimeMs);
    Power_CheckBattery(currentTimeMs);
}
