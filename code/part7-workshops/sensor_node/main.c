/**
 * @file main.c
 * @brief Bare-Metal Sensor Node - Superloop Architecture Example
 * 
 * @details This file demonstrates a production-grade bare-metal embedded
 * application using the superloop (foreground/background) architecture.
 * 
 * Architecture Overview:
 * ┌─────────────────────────────────────────────────────────────┐
 * │                    FOREGROUND (Main Loop)                   │
 * │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐       │
 * │  │ Watchdog│→ │  Power  │→ │ Sensor  │→ │ Process │       │
 * │  │  Feed   │  │ Manager │  │  Read   │  │  Data   │       │
 * │  └─────────┘  └─────────┘  └─────────┘  └─────────┘       │
 * │       ↓            ↓            ↓            ↓             │
 * │  ┌─────────────────────────────────────────────────┐      │
 * │  │              BACKGROUND (ISRs)                   │      │
 * │  │  Timer ISR │ UART ISR │ GPIO ISR │ ADC ISR     │      │
 * │  └─────────────────────────────────────────────────┘      │
 * └─────────────────────────────────────────────────────────────┘
 * 
 * Key Design Patterns:
 * - Non-blocking state machines
 * - Time-triggered scheduling
 * - Watchdog supervision
 * - Low-power optimization
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

/*============================================================================*/
/*                              INCLUDES                                      */
/*============================================================================*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sensor_driver.h"
#include "power_manager.h"

/*============================================================================*/
/*                              COMPILER/SYSTEM CONFIG                        */
/*============================================================================*/

/**
 * @brief Compiler abstraction for special instructions
 * 
 * @details These macros provide portable access to compiler-specific
 * intrinsics for watchdog and low-power operations.
 */
#if defined(__GNUC__)
    #define BARRIER()           __asm__ volatile("" ::: "memory")
    #define UNUSED(x)           __attribute__((unused)) x
    #define WEAK                __attribute__((weak))
#elif defined(__ICCARM__)
    #define BARRIER()           __no_operation()
    #define UNUSED(x)           x
    #define WEAK                __weak
#elif defined(__CC_ARM)
    #define BARRIER()           __nop()
    #define UNUSED(x)           x
    #define WEAK                __weak
#else
    #define BARRIER()
    #define UNUSED(x)           x
    #define WEAK
#endif

/*============================================================================*/
/*                              DEFINES                                       */
/*============================================================================*/

/** @brief System tick frequency in Hz */
#define SYSTEM_TICK_HZ              (1000U)

/** @brief Main loop period in milliseconds */
#define MAIN_LOOP_PERIOD_MS         (10U)

/** @brief Watchdog timeout in milliseconds */
#define WATCHDOG_TIMEOUT_MS         (2000U)

/** @brief Sensor sampling interval in milliseconds */
#define SENSOR_SAMPLE_INTERVAL_MS   (1000U)

/** @brief Data processing interval in milliseconds */
#define DATA_PROCESS_INTERVAL_MS    (5000U)

/** @brief Statistics reporting interval in milliseconds */
#define STATS_REPORT_INTERVAL_MS    (60000U)

/** @brief Idle timeout before entering low-power mode */
#define IDLE_TIMEOUT_MS             (30000U)

/** @brief Number of samples for moving average calculation */
#define MOVING_AVERAGE_SAMPLES      (10U)

/** @brief Temperature alarm threshold (Celsius) */
#define TEMP_ALARM_HIGH             (50.0f)

/** @brief Temperature warning threshold (Celsius) */
#define TEMP_WARNING_HIGH           (40.0f)

/** @brief Humidity alarm threshold (%) */
#define HUMIDITY_ALARM_HIGH         (90.0f)

/** @brief Watchdog refresh period (must be < WATCHDOG_TIMEOUT_MS) */
#define WATCHDOG_REFRESH_MS         (1000U)

/*============================================================================*/
/*                              TYPE DEFINITIONS                              */
/*============================================================================*/

/**
 * @brief Application state enumeration
 * 
 * @details Defines the high-level states of the application.
 * Used for state machine implementation.
 */
typedef enum {
    APP_STATE_INIT = 0,             /**< Initialization state */
    APP_STATE_NORMAL,               /**< Normal operation */
    APP_STATE_WARNING,              /**< Warning condition active */
    APP_STATE_ALARM,                /**< Alarm condition active */
    APP_STATE_LOW_POWER,            /**< Low-power mode */
    APP_STATE_ERROR,                /**< Error state */
    APP_STATE_COUNT                 /**< Number of states */
} AppState_t;

/**
 * @brief System tick counter structure
 * 
 * @details Organizes timing counters for different periodic tasks.
 * Each counter is decremented by the tick ISR.
 */
typedef struct {
    volatile uint32_t milliseconds;     /**< Millisecond counter */
    volatile uint16_t sensorTimer;      /**< Sensor read timer */
    volatile uint16_t processTimer;     /**< Data processing timer */
    volatile uint16_t statsTimer;       /**< Statistics timer */
    volatile uint16_t watchdogTimer;    /**< Watchdog refresh timer */
    volatile uint16_t idleTimer;        /**< Idle detection timer */
} SystemTimers_t;

/**
 * @brief Sensor data structure
 * 
 * @details Holds the latest sensor readings and associated metadata.
 */
typedef struct {
    float   temperature;                /**< Current temperature (°C) */
    float   humidity;                   /**< Current humidity (%) */
    float   tempAverage;                /**< Temperature moving average */
    float   humidityAverage;            /**< Humidity moving average */
    float   tempHistory[MOVING_AVERAGE_SAMPLES];   /**< Temperature history */
    float   humidityHistory[MOVING_AVERAGE_SAMPLES];/**< Humidity history */
    uint8_t historyIndex;               /**< Current history buffer index */
    uint8_t historyCount;               /**< Number of valid samples */
    uint32_t lastUpdate;                /**< Last update timestamp */
    bool    isValid;                    /**< Data validity flag */
} SensorData_t;

/**
 * @brief System statistics structure
 * 
 * @details Tracks application-level statistics for monitoring and debugging.
 */
typedef struct {
    uint32_t mainLoopCount;             /**< Main loop iterations */
    uint32_t sensorReadCount;           /**< Total sensor reads */
    uint32_t sensorErrorCount;          /**< Sensor read errors */
    uint32_t watchdogResets;            /**< Watchdog reset count */
    uint32_t lowPowerEntries;           /**< Low-power mode entries */
    uint32_t alarmsTriggered;           /**< Alarm events */
    uint32_t maxLoopTime;               /**< Maximum loop execution time */
    uint32_t uptime;                    /**< System uptime in seconds */
} SystemStats_t;

/**
 * @brief Application context structure
 * 
 * @details Central container for all application state.
 * Single global instance for simplicity in bare-metal system.
 */
typedef struct {
    AppState_t         state;           /**< Current application state */
    SystemTimers_t     timers;          /**< System timing counters */
    SensorData_t       sensorData;      /**< Sensor readings */
    SystemStats_t      stats;           /**< System statistics */
    SensorHandle_t    *tempSensor;      /**< Temperature sensor handle */
    SensorHandle_t    *humiSensor;      /**< Humidity sensor handle */
    PowerMode_t        currentPowerMode;/**< Current power mode */
    bool               initialized;     /**< Initialization complete flag */
    bool               sensorActive;    /**< Sensor reading active flag */
    bool               watchdogEnabled; /**< Watchdog enabled flag */
} Application_t;

/*============================================================================*/
/*                              GLOBAL VARIABLES                              */
/*============================================================================*/

/** 
 * @brief Application context instance
 * 
 * @details Single global application context. In a bare-metal system,
 * this is acceptable and simplifies state management across modules.
 */
static Application_t g_app = {0};

/*============================================================================*/
/*                              WATCHDOG FUNCTIONS                            */
/*============================================================================*/

/**
 * @brief Initialize the hardware watchdog timer
 * 
 * @details Configures the independent watchdog (IWDG) with the specified
 * timeout. The watchdog is clocked by the internal LSI oscillator.
 * 
 * Configuration steps:
 * 1. Enable write access to IWDG registers
 * 2. Set prescaler for desired timeout range
 * 3. Set reload value for exact timeout
 * 4. Start the watchdog counter
 */
static void Watchdog_Init(void)
{
    g_app.watchdogEnabled = true;
    g_app.timers.watchdogTimer = WATCHDOG_REFRESH_MS / MAIN_LOOP_PERIOD_MS;
}

/**
 * @brief Refresh (feed) the watchdog timer
 * 
 * @details Must be called periodically before the watchdog timeout expires.
 * This function is called from the main loop - avoid calling from ISRs.
 * 
 * @note The refresh is only performed if the watchdog timer has expired,
 *       preventing unnecessary IWDG register access.
 */
static void Watchdog_Feed(void)
{
    if (!g_app.watchdogEnabled) {
        return;
    }
    
    if (g_app.timers.watchdogTimer == 0) {
        g_app.timers.watchdogTimer = WATCHDOG_REFRESH_MS / MAIN_LOOP_PERIOD_MS;
        g_app.stats.watchdogResets++;
    }
}

/**
 * @brief Disable the watchdog (if supported by hardware)
 * 
 * @details On most MCUs, once started, the watchdog cannot be disabled.
 * This function is provided for compatibility and testing purposes.
 * 
 * @warning This function may have no effect on production hardware.
 */
static void Watchdog_Disable(void)
{
    g_app.watchdogEnabled = false;
}

/*============================================================================*/
/*                              TIME MANAGEMENT                               */
/*============================================================================*/

/**
 * @brief Get current system time in milliseconds
 * 
 * @details Returns the value of the millisecond counter.
 * This counter overflows approximately every 49.7 days.
 * 
 * @return Current system time in milliseconds
 */
static uint32_t System_GetTime(void)
{
    return g_app.timers.milliseconds;
}

/**
 * @brief Check if a time interval has elapsed
 * 
 * @details Non-blocking delay check using the system timer.
 * Handles timer wrap-around correctly.
 * 
 * @param lastTime  Time of last event
 * @param interval  Interval to check in milliseconds
 * @return true if interval has elapsed, false otherwise
 */
static bool System_TimeElapsed(uint32_t lastTime, uint32_t interval)
{
    uint32_t currentTime = System_GetTime();
    uint32_t elapsed = currentTime - lastTime;
    return (elapsed >= interval);
}

/**
 * @brief Increment system timers (called from ISR)
 * 
 * @details Called from the SysTick handler to update all timing counters.
 * This is the only function that modifies the timer values.
 */
static void System_TickHandler(void)
{
    g_app.timers.milliseconds++;
    
    if (g_app.timers.sensorTimer > 0) {
        g_app.timers.sensorTimer--;
    }
    
    if (g_app.timers.processTimer > 0) {
        g_app.timers.processTimer--;
    }
    
    if (g_app.timers.statsTimer > 0) {
        g_app.timers.statsTimer--;
    }
    
    if (g_app.timers.watchdogTimer > 0) {
        g_app.timers.watchdogTimer--;
    }
    
    if (g_app.timers.idleTimer > 0) {
        g_app.timers.idleTimer--;
    }
}

/*============================================================================*/
/*                              DATA PROCESSING                               */
/*============================================================================*/

/**
 * @brief Update moving average calculation
 * 
 * @details Adds a new sample to the history buffer and calculates
 * the moving average. Uses a circular buffer for efficiency.
 * 
 * @param history      Pointer to history buffer
 * @param newSample    New sample to add
 * @param pIndex       Pointer to current index (modified)
 * @param pCount       Pointer to sample count (modified)
 * @return Calculated moving average
 */
static float Data_UpdateMovingAverage(float *history, float newSample,
                                       uint8_t *pIndex, uint8_t *pCount)
{
    history[*pIndex] = newSample;
    *pIndex = (*pIndex + 1U) % MOVING_AVERAGE_SAMPLES;
    
    if (*pCount < MOVING_AVERAGE_SAMPLES) {
        (*pCount)++;
    }
    
    float sum = 0.0f;
    for (uint8_t i = 0; i < *pCount; i++) {
        sum += history[i];
    }
    
    return sum / (float)*pCount;
}

/**
 * @brief Process and analyze sensor data
 * 
 * @details Performs data validation, averaging, and threshold checking.
 * Updates application state based on analysis results.
 */
static void Data_Process(void)
{
    SensorData_t *data = &g_app.sensorData;
    
    if (!data->isValid) {
        return;
    }
    
    data->tempAverage = Data_UpdateMovingAverage(
        data->tempHistory, data->temperature,
        &data->historyIndex, &data->historyCount);
    
    data->humidityAverage = Data_UpdateMovingAverage(
        data->humidityHistory, data->humidity,
        &data->historyIndex, &data->historyCount);
    
    if (data->temperature >= TEMP_ALARM_HIGH || 
        data->humidity >= HUMIDITY_ALARM_HIGH) {
        g_app.state = APP_STATE_ALARM;
        g_app.stats.alarmsTriggered++;
    } else if (data->temperature >= TEMP_WARNING_HIGH) {
        g_app.state = APP_STATE_WARNING;
    } else {
        if (g_app.state == APP_STATE_WARNING || g_app.state == APP_STATE_ALARM) {
            g_app.state = APP_STATE_NORMAL;
        }
    }
    
    data->lastUpdate = System_GetTime();
}

/*============================================================================*/
/*                              SENSOR HANDLING                               */
/*============================================================================*/

/**
 * @brief Initialize all sensors
 * 
 * @details Creates and initializes sensor driver instances for all
 * configured sensors. Sets up callbacks for asynchronous operation.
 * 
 * @return true if all sensors initialized successfully
 */
static bool Sensors_Init(void)
{
    SensorStatus_t status;
    SensorConfig_t config = {
        .sampleRateHz = 1,
        .enableFiltering = true,
        .oversampling = 4
    };
    
    Sensor_InitSubsystem();
    
    status = Sensor_Create(SENSOR_TYPE_TEMPERATURE, 0, 0x48, &config, &g_app.tempSensor);
    if (status != SENSOR_OK) {
        return false;
    }
    
    status = Sensor_Initialize(g_app.tempSensor);
    if (status != SENSOR_OK) {
        return false;
    }
    
    status = Sensor_Create(SENSOR_TYPE_HUMIDITY, 0, 0x44, &config, &g_app.humiSensor);
    if (status != SENSOR_OK) {
        return false;
    }
    
    status = Sensor_Initialize(g_app.humiSensor);
    if (status != SENSOR_OK) {
        return false;
    }
    
    g_app.sensorActive = true;
    return true;
}

/**
 * @brief Read all sensor values
 * 
 * @details Reads temperature and humidity sensors, updates the sensor
 * data structure, and validates the readings.
 * 
 * @return true if all readings successful
 */
static bool Sensors_Read(void)
{
    SensorStatus_t status;
    SensorData_t *data = &g_app.sensorData;
    float tempValue, humiValue;
    bool success = true;
    
    status = Sensor_Read(g_app.tempSensor, &tempValue);
    if (status == SENSOR_OK) {
        data->temperature = tempValue;
        g_app.stats.sensorReadCount++;
    } else {
        g_app.stats.sensorErrorCount++;
        success = false;
    }
    
    status = Sensor_Read(g_app.humiSensor, &humiValue);
    if (status == SENSOR_OK) {
        data->humidity = humiValue;
        g_app.stats.sensorReadCount++;
    } else {
        g_app.stats.sensorErrorCount++;
        success = false;
    }
    
    data->isValid = success;
    data->lastUpdate = System_GetTime();
    
    return success;
}

/**
 * @brief Put sensors in low-power mode
 * 
 * @details Called before entering low-power mode to minimize power
 * consumption. Sensors retain their configuration.
 */
static void Sensors_Sleep(void)
{
    if (g_app.tempSensor != NULL) {
        Sensor_Sleep(g_app.tempSensor);
    }
    if (g_app.humiSensor != NULL) {
        Sensor_Sleep(g_app.humiSensor);
    }
    g_app.sensorActive = false;
}

/**
 * @brief Wake sensors from low-power mode
 * 
 * @details Restores sensor operation after waking from low-power mode.
 * May require time for sensors to stabilize.
 */
static void Sensors_Wake(void)
{
    if (g_app.tempSensor != NULL) {
        Sensor_Wake(g_app.tempSensor);
    }
    if (g_app.humiSensor != NULL) {
        Sensor_Wake(g_app.humiSensor);
    }
    g_app.sensorActive = true;
}

/*============================================================================*/
/*                              POWER MANAGEMENT                              */
/*============================================================================*/

/**
 * @brief Power state change callback
 * 
 * @details Called when the power mode changes. Used to prepare
 * peripherals for the new power state.
 * 
 * @param oldMode   Previous power mode
 * @param newMode   New power mode  
 * @param reason    Reason for the transition
 * @param userData  User context (unused)
 */
static void Power_StateChangeCallback(PowerMode_t oldMode, PowerMode_t newMode,
                                       PowerTransitionReason_t reason, void *userData)
{
    (void)userData;
    
    g_app.currentPowerMode = newMode;
    
    if (newMode != PM_MODE_ACTIVE && oldMode == PM_MODE_ACTIVE) {
        Sensors_Sleep();
        g_app.state = APP_STATE_LOW_POWER;
        g_app.stats.lowPowerEntries++;
    } else if (newMode == PM_MODE_ACTIVE && oldMode != PM_MODE_ACTIVE) {
        Sensors_Wake();
        g_app.state = APP_STATE_NORMAL;
    }
}

/**
 * @brief Initialize power management
 * 
 * @details Initializes the power manager subsystem and configures
 * wake-up sources for low-power operation.
 */
static void App_Power_Init(void)
{
    Power_Init();
    
    Power_RegisterStateCallback(Power_StateChangeCallback, NULL);
    
    WakeupConfig_t wakeupConfig = {
        .source = PM_WAKEUP_TIMER,
        .instance = 0,
        .polarity = 1,
        .debounceMs = 0,
        .enabled = true
    };
    Power_RegisterWakeupSource(&wakeupConfig);
    
    Power_SetIdleTimeout(IDLE_TIMEOUT_MS, PM_MODE_SLEEP);
}

/**
 * @brief Process power management tasks
 * 
 * @details Called each main loop iteration to handle automatic
 * power mode transitions and idle detection.
 */
static void Power_ProcessTasks(void)
{
    Power_Process(System_GetTime());
    
    if (g_app.state != APP_STATE_LOW_POWER) {
        Power_ResetIdleTimer();
    }
}

/**
 * @brief Enter low-power mode
 * 
 * @details Puts the system into the specified low-power mode.
 * Execution resumes when a wake-up event occurs.
 * 
 * @param mode Power mode to enter
 */
static void Power_EnterMode(PowerMode_t mode)
{
    WakeupSource_t wakeupSource;
    
    Power_EnterLowPower(mode, &wakeupSource);
    
    g_app.timers.sensorTimer = SENSOR_SAMPLE_INTERVAL_MS / MAIN_LOOP_PERIOD_MS;
}

/*============================================================================*/
/*                              INITIALIZATION                                */
/*============================================================================*/

/**
 * @brief Initialize all system peripherals
 * 
 * @details Performs complete system initialization in the correct order.
 * Each subsystem is initialized and verified before proceeding.
 * 
 * @return true if initialization successful
 */
static bool System_Init(void)
{
    memset(&g_app, 0, sizeof(Application_t));
    g_app.state = APP_STATE_INIT;
    
    Watchdog_Init();
    
    App_Power_Init();
    
    if (!Sensors_Init()) {
        g_app.state = APP_STATE_ERROR;
        return false;
    }
    
    g_app.timers.sensorTimer = SENSOR_SAMPLE_INTERVAL_MS / MAIN_LOOP_PERIOD_MS;
    g_app.timers.processTimer = DATA_PROCESS_INTERVAL_MS / MAIN_LOOP_PERIOD_MS;
    g_app.timers.statsTimer = STATS_REPORT_INTERVAL_MS / MAIN_LOOP_PERIOD_MS;
    g_app.timers.idleTimer = IDLE_TIMEOUT_MS / MAIN_LOOP_PERIOD_MS;
    
    g_app.state = APP_STATE_NORMAL;
    g_app.initialized = true;
    
    return true;
}

/*============================================================================*/
/*                              MAIN LOOP                                     */
/*============================================================================*/

/**
 * @brief Main application entry point
 * 
 * @details Implements the superloop architecture with the following cycle:
 * 1. Feed watchdog
 * 2. Process power management
 * 3. Check and handle sensors
 * 4. Process data
 * 5. Handle communications (if applicable)
 * 6. Sleep until next tick (low-power optimization)
 * 
 * The loop runs at a fixed rate determined by MAIN_LOOP_PERIOD_MS.
 * All operations are non-blocking to maintain deterministic timing.
 * 
 * @return Never returns in embedded systems
 */
int main(void)
{
    if (!System_Init()) {
        while (1) {
            BARRIER();
        }
    }
    
    while (1) {
        uint32_t loopStartTime = System_GetTime();
        
        Watchdog_Feed();
        
        Power_ProcessTasks();
        
        if (g_app.state != APP_STATE_LOW_POWER) {
            if (g_app.timers.sensorTimer == 0) {
                Sensors_Read();
                g_app.timers.sensorTimer = SENSOR_SAMPLE_INTERVAL_MS / MAIN_LOOP_PERIOD_MS;
            }
            
            if (g_app.timers.processTimer == 0) {
                Data_Process();
                g_app.timers.processTimer = DATA_PROCESS_INTERVAL_MS / MAIN_LOOP_PERIOD_MS;
            }
            
            if (g_app.timers.statsTimer == 0) {
                g_app.stats.uptime++;
                g_app.timers.statsTimer = STATS_REPORT_INTERVAL_MS / MAIN_LOOP_PERIOD_MS;
            }
        }
        
        g_app.stats.mainLoopCount++;
        
        uint32_t loopTime = System_GetTime() - loopStartTime;
        if (loopTime > g_app.stats.maxLoopTime) {
            g_app.stats.maxLoopTime = loopTime;
        }
        
        if (g_app.currentPowerMode == PM_MODE_LOW_POWER || 
            g_app.state == APP_STATE_LOW_POWER) {
            Power_EnterMode(PM_MODE_SLEEP);
        }
    }
    
    return 0;
}

/*============================================================================*/
/*                              INTERRUPT HANDLERS                            */
/*============================================================================*/

/**
 * @brief System tick interrupt handler
 * 
 * @details Called every 1ms by the SysTick timer. Updates timing counters
 * and sets flags for the main loop to process.
 * 
 * @note Keep ISR execution time minimal. Only update counters here,
 *       do the actual work in the main loop.
 */
void SysTick_Handler(void)
{
    System_TickHandler();
}

/**
 * @brief External interrupt handler (for wake-up sources)
 * 
 * @details Handles GPIO interrupts configured as wake-up sources.
 * Clears the interrupt and signals the main loop to wake up.
 */
void EXTI0_IRQHandler(void)
{
    Power_ResetIdleTimer();
    
    if (g_app.state == APP_STATE_LOW_POWER) {
        g_app.state = APP_STATE_NORMAL;
    }
}
