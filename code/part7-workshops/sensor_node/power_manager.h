/**
 * @file power_manager.h
 * @brief Power Management Interface - Low-power mode control and optimization
 * 
 * @details This header defines the power management subsystem for embedded
 * systems requiring sophisticated power control. It supports multiple low-power
 * modes, wake-up source configuration, and power consumption monitoring.
 * 
 * Architecture Principles:
 * - Single Responsibility: Focused solely on power management
 * - Strategy Pattern: Different power strategies for different modes
 * - Observer Pattern: Notifications for power state changes
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*                              INCLUDES                                      */
/*============================================================================*/

#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/*                              DEFINES                                       */
/*============================================================================*/

/** @brief Maximum number of registered wake-up sources */
#define PM_MAX_WAKEUP_SOURCES       (8U)

/** @brief Maximum number of power state change listeners */
#define PM_MAX_LISTENERS            (4U)

/** @brief Default debounce time for wake-up sources (ms) */
#define PM_WAKEUP_DEBOUNCE_MS       (50U)

/** @brief Power profile name maximum length */
#define PM_PROFILE_NAME_LENGTH      (16U)

/*============================================================================*/
/*                              ENUMERATIONS                                  */
/*============================================================================*/

/**
 * @brief Power mode enumeration
 * 
 * @details Defines the available power modes in order of decreasing power
 * consumption. Transition between modes follows strict rules to ensure
 * system stability.
 */
typedef enum {
    PM_MODE_ACTIVE = 0,             /**< Full power, all peripherals active */
    PM_MODE_LOW_POWER,              /**< Reduced clock, some peripherals idle */
    PM_MODE_SLEEP,                  /**< CPU halted, peripherals running */
    PM_MODE_DEEP_SLEEP,             /**< CPU and most peripherals off, RAM retained */
    PM_MODE_STANDBY,                /**< Minimal power, only RTC and wake-up logic */
    PM_MODE_SHUTDOWN,               /**< Complete power down, cold start required */
    PM_MODE_COUNT                   /**< Number of power modes */
} PowerMode_t;

/**
 * @brief Power manager status codes
 */
typedef enum {
    PM_OK = 0,                      /**< Operation successful */
    PM_ERROR = -1,                  /**< Generic error */
    PM_ERROR_INVALID_MODE = -2,     /**< Invalid power mode requested */
    PM_ERROR_TRANSITION = -3,       /**< Mode transition failed */
    PM_ERROR_WAKEUP_SOURCE = -4,    /**< Invalid wake-up source */
    PM_ERROR_BUSY = -5,             /**< Operation in progress */
    PM_ERROR_NOT_INITIALIZED = -6,  /**< Subsystem not initialized */
    PM_ERROR_TIMEOUT = -7,          /**< Operation timed out */
    PM_ERROR_NOT_SUPPORTED = -8     /**< Feature not supported on this hardware */
} PowerStatus_t;

/**
 * @brief Wake-up source types
 * 
 * @details Defines the hardware sources that can wake the system from
 * low-power modes. Not all sources are available in all power modes.
 */
typedef enum {
    PM_WAKEUP_NONE = 0,             /**< No wake-up source */
    PM_WAKEUP_GPIO = 0x0001U,       /**< GPIO pin state change */
    PM_WAKEUP_RTC_ALARM = 0x0002U,  /**< Real-time clock alarm */
    PM_WAKEUP_UART = 0x0004U,       /**< UART activity detected */
    PM_WAKEUP_TIMER = 0x0008U,      /**< General purpose timer expiration */
    PM_WAKEUP_COMPARATOR = 0x0010U, /**< Analog comparator trigger */
    PM_WAKEUP_EXTI = 0x0020U,       /**< External interrupt */
    PM_WAKEUP_I2C = 0x0040U,        /**< I2C address match */
    PM_WAKEUP_SPI = 0x0080U,        /**< SPI activity */
    PM_WAKEUP_USB = 0x0100U,        /**< USB activity/resume */
    PM_WAKEUP_TOUCH = 0x0200U,      /**< Touch sensor detection */
    PM_WAKEUP_ALL = 0xFFFFU         /**< All wake-up sources */
} WakeupSource_t;

/**
 * @brief Power state change reason
 */
typedef enum {
    PM_REASON_NONE = 0,             /**< No specific reason */
    PM_REASON_USER_REQUEST,         /**< User/application requested change */
    PM_REASON_TIMEOUT,              /**< Idle timeout expired */
    PM_REASON_WAKEUP,               /**< Wake-up event occurred */
    PM_REASON_LOW_BATTERY,          /**< Low battery condition */
    PM_REASON_CRITICAL,             /**< Critical power level */
    PM_REASON_THERMAL,              /**< Thermal protection triggered */
    PM_REASON_STARTUP               /**< System startup */
} PowerTransitionReason_t;

/**
 * @brief Battery status enumeration
 */
typedef enum {
    PM_BATTERY_UNKNOWN = 0,         /**< Battery status unknown */
    PM_BATTERY_CRITICAL,            /**< Critical level (<5%) */
    PM_BATTERY_LOW,                 /**< Low level (<20%) */
    PM_BATTERY_MEDIUM,              /**< Medium level (20-80%) */
    PM_BATTERY_HIGH,                /**< High level (>80%) */
    PM_BATTERY_FULL,                /**< Fully charged */
    PM_BATTERY_CHARGING,            /**< Currently charging */
    PM_BATTERY_DISCHARGING,         /**< Currently discharging */
    PM_BATTERY_NOT_PRESENT          /**< No battery detected */
} BatteryStatus_t;

/*============================================================================*/
/*                              STRUCTURES                                    */
/*============================================================================*/

/**
 * @brief Wake-up source configuration
 * 
 * @details Configuration parameters for a single wake-up source.
 * Used when registering wake-up sources with the power manager.
 */
typedef struct {
    WakeupSource_t source;          /**< Wake-up source type */
    uint8_t         instance;       /**< Instance number (e.g., GPIO pin, UART port) */
    uint8_t         polarity;       /**< Trigger polarity (0=low/falling, 1=high/rising) */
    uint16_t        debounceMs;     /**< Debounce time in milliseconds */
    uint32_t        parameter;      /**< Source-specific parameter */
    bool            enabled;        /**< Enable/disable flag */
} WakeupConfig_t;

/**
 * @brief Power mode configuration
 * 
 * @details Defines the behavior and settings for each power mode.
 * Used for customizing power profiles.
 */
typedef struct {
    PowerMode_t     mode;           /**< Power mode identifier */
    uint32_t        entryTimeoutMs; /**< Maximum time allowed in this mode */
    uint32_t        exitLatencyMs;  /**< Time to exit this mode */
    uint32_t        typicalCurrent; /**< Typical current consumption (µA) */
    uint32_t        wakeupSources;  /**< Allowed wake-up sources bitmask */
    bool            retainRam;      /**< Retain RAM contents */
    bool            retainRegisters;/**< Retain peripheral registers */
    bool            clockScaling;   /**< Enable clock frequency scaling */
    uint8_t         clockDivider;   /**< Clock divider when scaling enabled */
} PowerModeConfig_t;

/**
 * @brief Power statistics structure
 * 
 * @details Tracks power consumption and time spent in each mode.
 * Used for power optimization analysis and battery life estimation.
 */
typedef struct {
    uint32_t timeInMode[PM_MODE_COUNT];     /**< Cumulative time in each mode (ms) */
    uint32_t transitionCount[PM_MODE_COUNT];/**< Number of transitions to each mode */
    uint32_t totalUptime;                   /**< Total system uptime (ms) */
    uint32_t wakeupCount;                   /**< Total wake-up events */
    uint32_t lastWakeupTime;                /**< Timestamp of last wake-up */
    uint32_t lastSleepTime;                 /**< Timestamp of last sleep entry */
    uint32_t averageCurrent;                /**< Running average current (µA) */
    uint32_t peakCurrent;                   /**< Peak current observed (µA) */
    uint32_t energyConsumed;                /**< Total energy consumed (mWh) */
} PowerStats_t;

/**
 * @brief Battery information structure
 */
typedef struct {
    uint16_t        voltageMv;      /**< Battery voltage in millivolts */
    int16_t         currentMa;      /**< Instantaneous current (mA, signed) */
    uint8_t         levelPercent;   /**< Charge level (0-100%) */
    int16_t         temperature;    /**< Battery temperature (0.1°C units) */
    BatteryStatus_t status;         /**< Current battery status */
    uint16_t        timeToEmpty;    /**< Estimated time to empty (minutes) */
    uint16_t        timeToFull;     /**< Estimated time to full (minutes) */
    uint32_t        cycleCount;     /**< Number of charge cycles */
    bool            present;        /**< Battery present flag */
} BatteryInfo_t;

/**
 * @brief Power profile structure
 * 
 * @details Named configuration preset for common use cases.
 * Profiles can be switched at runtime based on application state.
 */
typedef struct {
    char            name[PM_PROFILE_NAME_LENGTH];   /**< Profile name */
    PowerMode_t     defaultMode;                    /**< Default power mode */
    uint32_t        idleTimeoutMs;                  /**< Idle time before mode change */
    PowerMode_t     idleTargetMode;                 /**< Target mode after idle */
    bool            adaptiveBrightness;             /**< Auto-adjust display */
    bool            cpuScalingEnabled;              /**< Enable CPU frequency scaling */
    bool            peripheralPowerGating;          /**< Gate unused peripherals */
    uint8_t         wakeupSourceMask;               /**< Enabled wake-up sources */
} PowerProfile_t;

/*============================================================================*/
/*                              CALLBACK TYPES                                */
/*============================================================================*/

/**
 * @brief Power state change callback
 * 
 * @details Called before and after power mode transitions.
 * Allows application to prepare for or respond to mode changes.
 * 
 * @param oldMode   Previous power mode
 * @param newMode   New power mode
 * @param reason    Reason for the transition
 * @param userData  User-provided context
 */
typedef void (*PowerStateCallback_t)(PowerMode_t oldMode,
                                      PowerMode_t newMode,
                                      PowerTransitionReason_t reason,
                                      void *userData);

/**
 * @brief Wake-up event callback
 * 
 * @details Called when a wake-up event occurs.
 * 
 * @param source    Wake-up source that triggered the event
 * @param userData  User-provided context
 */
typedef void (*WakeupCallback_t)(WakeupSource_t source, void *userData);

/**
 * @brief Low battery warning callback
 * 
 * @details Called when battery level crosses warning thresholds.
 * 
 * @param level     Current battery level percentage
 * @param status    Current battery status
 * @param userData  User-provided context
 */
typedef void (*LowBatteryCallback_t)(uint8_t level, BatteryStatus_t status, void *userData);

/*============================================================================*/
/*                              PUBLIC API                                    */
/*============================================================================*/

/**
 * @brief Initialize the power management subsystem
 * 
 * @details Must be called before any other power management functions.
 * Configures default power profiles and initializes hardware.
 * 
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_Init(void);

/**
 * @brief Deinitialize the power management subsystem
 * 
 * @details Restores hardware to default state and releases resources.
 */
void Power_Deinit(void);

/**
 * @brief Get current power mode
 * 
 * @param pMode Output pointer for current mode
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_GetMode(PowerMode_t *pMode);

/**
 * @brief Request transition to a new power mode
 * 
 * @details Asynchronously requests a mode transition. The actual transition
 * may be delayed if the system is busy or the transition is not allowed.
 * 
 * @param newMode   Target power mode
 * @param reason    Reason for the transition
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_RequestMode(PowerMode_t newMode, PowerTransitionReason_t reason);

/**
 * @brief Force immediate power mode transition
 * 
 * @details Forces an immediate transition, bypassing normal checks.
 * Use with caution as this may disrupt ongoing operations.
 * 
 * @param newMode   Target power mode
 * @param reason    Reason for the transition
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_ForceMode(PowerMode_t newMode, PowerTransitionReason_t reason);

/**
 * @brief Register a wake-up source
 * 
 * @details Configures and enables a wake-up source for low-power mode exit.
 * 
 * @param config    Wake-up source configuration
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_RegisterWakeupSource(const WakeupConfig_t *config);

/**
 * @brief Unregister a wake-up source
 * 
 * @param source    Wake-up source type
 * @param instance  Source instance number
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_UnregisterWakeupSource(WakeupSource_t source, uint8_t instance);

/**
 * @brief Enable/disable a wake-up source
 * 
 * @param source    Wake-up source type
 * @param instance  Source instance number
 * @param enable    True to enable, false to disable
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_SetWakeupEnabled(WakeupSource_t source, uint8_t instance, bool enable);

/**
 * @brief Get currently enabled wake-up sources
 * 
 * @param pSources  Output pointer for wake-up source bitmask
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_GetWakeupSources(uint32_t *pSources);

/**
 * @brief Enter low-power mode until wake-up event
 * 
 * @details Blocks until a wake-up event occurs. The system enters the
 * lowest power mode compatible with configured wake-up sources.
 * 
 * @param mode      Power mode to enter
 * @param pSource   Output pointer for wake-up source (can be NULL)
 * @return PM_OK on wake-up, error code otherwise
 */
PowerStatus_t Power_EnterLowPower(PowerMode_t mode, WakeupSource_t *pSource);

/**
 * @brief Set idle timeout for automatic power mode transition
 * 
 * @details Configures automatic transition to a lower power mode after
 * a period of inactivity.
 * 
 * @param timeoutMs     Idle timeout in milliseconds (0 to disable)
 * @param targetMode    Power mode to enter after timeout
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_SetIdleTimeout(uint32_t timeoutMs, PowerMode_t targetMode);

/**
 * @brief Reset the idle timer
 * 
 * @details Call this to indicate activity and prevent automatic sleep.
 * Should be called from user input handlers or communication activity.
 */
void Power_ResetIdleTimer(void);

/**
 * @brief Register power state change callback
 * 
 * @param callback  Callback function pointer
 * @param userData  User context pointer
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_RegisterStateCallback(PowerStateCallback_t callback, void *userData);

/**
 * @brief Register wake-up event callback
 * 
 * @param callback  Callback function pointer
 * @param userData  User context pointer
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_RegisterWakeupCallback(WakeupCallback_t callback, void *userData);

/**
 * @brief Register low battery callback
 * 
 * @param callback  Callback function pointer
 * @param userData  User context pointer
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_RegisterLowBatteryCallback(LowBatteryCallback_t callback, void *userData);

/**
 * @brief Get power consumption statistics
 * 
 * @param pStats Output pointer for statistics structure
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_GetStats(PowerStats_t *pStats);

/**
 * @brief Clear power statistics
 * 
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_ClearStats(void);

/**
 * @brief Get battery information
 * 
 * @param pInfo Output pointer for battery information
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_GetBatteryInfo(BatteryInfo_t *pInfo);

/**
 * @brief Set battery warning thresholds
 * 
 * @param warningLevel   Warning threshold percentage (e.g., 20)
 * @param criticalLevel  Critical threshold percentage (e.g., 5)
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_SetBatteryThresholds(uint8_t warningLevel, uint8_t criticalLevel);

/**
 * @brief Set active power profile
 * 
 * @param profile   Pointer to power profile configuration
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_SetProfile(const PowerProfile_t *profile);

/**
 * @brief Get current power profile
 * 
 * @param pProfile Output pointer for current profile
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_GetProfile(PowerProfile_t *pProfile);

/**
 * @brief Configure a power mode
 * 
 * @param config Power mode configuration
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_ConfigureMode(const PowerModeConfig_t *config);

/**
 * @brief Check if a power mode is available
 * 
 * @param mode Power mode to check
 * @param pAvailable Output pointer for availability flag
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_IsModeAvailable(PowerMode_t mode, bool *pAvailable);

/**
 * @brief Get estimated current consumption for a mode
 * 
 * @param mode      Power mode to query
 * @param pCurrent  Output pointer for current in microamps
 * @return PM_OK on success, error code otherwise
 */
PowerStatus_t Power_GetModeCurrent(PowerMode_t mode, uint32_t *pCurrent);

/**
 * @brief Process power management tasks
 * 
 * @details Must be called regularly from the main loop to handle
 * automatic mode transitions and power management housekeeping.
 * 
 * @param currentTimeMs Current system time in milliseconds
 */
void Power_Process(uint32_t currentTimeMs);

#ifdef __cplusplus
}
#endif

#endif /* POWER_MANAGER_H */
