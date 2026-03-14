/**
 * @file main.c
 * @brief RTOS-Based Connected Device - FreeRTOS Application Entry Point
 * 
 * @details This file demonstrates a production-grade RTOS-based embedded
 * application using FreeRTOS. It implements a multi-task architecture with:
 * 
 * Architecture Overview:
 * ┌────────────────────────────────────────────────────────────────────────┐
 * │                          APPLICATION LAYER                             │
 * │  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐    │
 * │  │  Controller Task │  │   Sensor Task    │  │  Network Task    │    │
 * │  │   (Coordinator)  │←→│   (Producer)     │←→│   (Consumer)     │    │
 * │  └──────────────────┘  └──────────────────┘  └──────────────────┘    │
 * │           ↓                    ↓                     ↓                 │
 * │  ┌─────────────────────────────────────────────────────────────┐     │
 * │  │              FreeRTOS Queues / Semaphores / Timers           │     │
 * │  └─────────────────────────────────────────────────────────────┘     │
 * │           ↓                    ↓                     ↓                 │
 * │  ┌─────────────────────────────────────────────────────────────┐     │
 * │  │                    Hardware Abstraction Layer                │     │
 * │  │  GPIO │ UART │ SPI │ I2C │ Timer │ ADC │ DMA │ Network      │     │
 * │  └─────────────────────────────────────────────────────────────┘     │
 * └────────────────────────────────────────────────────────────────────────┘
 * 
 * Key Design Patterns:
 * - Producer-Consumer: Sensor task produces, network task consumes
 * - Mediator: Controller task coordinates between tasks
 * - Observer: Callback-based event notification
 * - State Machine: Mode management in controller
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

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "task_sensor.h"
#include "task_network.h"
#include "task_controller.h"

/*============================================================================*/
/*                              DEFINES                                       */
/*============================================================================*/

/** @brief Main application task name */
#define APP_TASK_NAME               "AppMain"

/** @brief Main task stack size */
#define APP_TASK_STACK_SIZE         (512U)

/** @brief Main task priority */
#define APP_TASK_PRIORITY           (1U)

/** @brief System status report interval (ms) */
#define STATUS_REPORT_INTERVAL_MS   (10000U)

/** @brief Watchdog feed interval (ms) */
#define WATCHDOG_FEED_INTERVAL_MS   (1000U)

/** @brief Maximum startup timeout (ms) */
#define STARTUP_TIMEOUT_MS          (5000U)

/** @brief Application version */
#define APP_VERSION_MAJOR           (1U)
#define APP_VERSION_MINOR           (0U)
#define APP_VERSION_PATCH           (0U)

/*============================================================================*/
/*                              TYPE DEFINITIONS                              */
/*============================================================================*/

/**
 * @brief Application state enumeration
 */
typedef enum {
    APP_STATE_UNINITIALIZED = 0,    /**< Not initialized */
    APP_STATE_INITIALIZING,         /**< Initialization in progress */
    APP_STATE_RUNNING,              /**< Normal operation */
    APP_STATE_ERROR,                /**< Error state */
    APP_STATE_SHUTDOWN              /**< Shutdown in progress */
} AppState_e;

/**
 * @brief System configuration structure
 */
typedef struct {
    uint8_t     deviceId[16];       /**< Unique device identifier */
    uint32_t    sampleIntervalMs;   /**< Default sensor sample interval */
    uint32_t    reportIntervalMs;   /**< Status report interval */
    uint8_t     networkEnabled;     /**< Network enabled flag */
    uint8_t     debugLevel;         /**< Debug output level */
} SystemConfig_t;

/**
 * @brief Application context structure
 */
typedef struct {
    AppState_e      state;          /**< Current application state */
    SystemConfig_t  config;         /**< System configuration */
    TaskHandle_t    appTaskHandle;  /**< Main application task handle */
    TimerHandle_t   statusTimer;    /**< Status report timer */
    SemaphoreHandle_t initMutex;    /**< Initialization synchronization */
    uint32_t        initErrors;     /**< Initialization error flags */
    bool            allTasksRunning;/**< All tasks running flag */
} ApplicationContext_t;

/*============================================================================*/
/*                              GLOBAL VARIABLES                              */
/*============================================================================*/

/** @brief Application context instance */
static ApplicationContext_t g_app = {0};

/*============================================================================*/
/*                              PRIVATE FUNCTIONS                             */
/*============================================================================*/

/**
 * @brief Initialize hardware peripherals
 * 
 * @details Initializes all hardware peripherals required by the application.
 * This function is called before the RTOS scheduler starts.
 * 
 * @return true on success, false on failure
 */
static bool Hardware_Init(void)
{
    return true;
}

/**
 * @brief Initialize system clocks
 * 
 * @details Configures the system clock tree for optimal performance
 * and power consumption.
 */
static void Clock_Init(void)
{
}

/**
 * @brief Initialize GPIO pins
 * 
 * @details Configures all GPIO pins for their intended functions.
 */
static void GPIO_Init(void)
{
}

/**
 * @brief Watchdog initialization
 * 
 * @details Configures and starts the independent watchdog timer.
 */
static void Watchdog_Init(void)
{
}

/**
 * @brief Feed the watchdog timer
 * 
 * @details Must be called periodically to prevent system reset.
 */
static void Watchdog_Feed(void)
{
}

/**
 * @brief Get system status string
 * 
 * @return Status string for logging
 */
static const char* GetStateString(AppState_e state)
{
    static const char* stateStrings[] = {
        "Uninitialized",
        "Initializing",
        "Running",
        "Error",
        "Shutdown"
    };
    
    if (state < sizeof(stateStrings) / sizeof(stateStrings[0])) {
        return stateStrings[state];
    }
    return "Unknown";
}

/**
 * @brief Status timer callback
 * 
 * @details Called periodically to report system status.
 * 
 * @param timer Timer handle
 */
static void StatusTimer_Callback(TimerHandle_t timer)
{
    (void)timer;
    
    ControllerStats_t stats;
    if (ControllerTask_GetStats(&stats) == CTRL_OK) {
    }
}

/**
 * @brief Sensor event callback
 * 
 * @details Called when sensor events occur.
 * 
 * @param event Sensor event data
 * @param userData User context
 */
static void SensorEvent_Callback(const SensorEvent_t *event, void *userData)
{
    (void)userData;
    
    switch (event->type) {
        case SENSOR_EVENT_DATA_READY:
            ControllerTask_ProcessSensorData(
                (uint8_t)event->sensorType,
                event->sample.value,
                event->sample.timestamp
            );
            break;
            
        case SENSOR_EVENT_THRESHOLD:
            {
                ControllerEvent_t ctrlEvent = {
                    .type = CTRL_EVENT_THRESHOLD,
                    .source = (uint8_t)event->sensorType,
                    .dataLength = sizeof(float)
                };
                memcpy(ctrlEvent.data, &event->sample.value, sizeof(float));
                ControllerTask_SendEvent(&ctrlEvent);
            }
            break;
            
        case SENSOR_EVENT_ERROR:
            {
                ControllerEvent_t ctrlEvent = {
                    .type = CTRL_EVENT_SYSTEM_ERROR,
                    .source = (uint8_t)event->sensorType,
                    .errorCode = event->error
                };
                ControllerTask_SendEvent(&ctrlEvent);
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief Network event callback
 * 
 * @details Called when network events occur.
 * 
 * @param event Network event data
 * @param userData User context
 */
static void NetworkEvent_Callback(const NetworkEvent_t *event, void *userData)
{
    (void)userData;
    
    ControllerEvent_t ctrlEvent = {0};
    
    switch (event->type) {
        case NET_EVENT_CONNECTED:
            ctrlEvent.type = CTRL_EVENT_NETWORK_STATUS;
            ctrlEvent.source = event->connectionId;
            ctrlEvent.errorCode = 0;
            break;
            
        case NET_EVENT_DISCONNECTED:
            ctrlEvent.type = CTRL_EVENT_NETWORK_STATUS;
            ctrlEvent.source = event->connectionId;
            ctrlEvent.errorCode = -1;
            break;
            
        case NET_EVENT_DATA_RECEIVED:
            ctrlEvent.type = CTRL_EVENT_NETWORK_DATA;
            ctrlEvent.source = event->connectionId;
            /* BUGFIX: Bounds check to prevent buffer overflow.
             * NETWORK_MAX_PACKET_SIZE (1500) > CONTROLLER_CMD_BUFFER_SIZE (64).
             * Truncate if payload exceeds our buffer capacity. */
            ctrlEvent.dataLength = (event->dataLength > CONTROLLER_CMD_BUFFER_SIZE) 
                                   ? CONTROLLER_CMD_BUFFER_SIZE 
                                   : event->dataLength;
            memcpy(ctrlEvent.data, event->data, ctrlEvent.dataLength);
            break;
            
        case NET_EVENT_ERROR:
            ctrlEvent.type = CTRL_EVENT_NETWORK_STATUS;
            ctrlEvent.source = event->connectionId;
            ctrlEvent.errorCode = event->error;
            break;
            
        default:
            break;
    }
    
    if (ctrlEvent.type != CTRL_EVENT_NONE) {
        ControllerTask_SendEvent(&ctrlEvent);
    }
}

/**
 * @brief Controller event callback
 * 
 * @details Called when controller events occur.
 * 
 * @param event Controller event data
 * @param userData User context
 */
static void ControllerEvent_Callback(const ControllerEvent_t *event, void *userData)
{
    (void)userData;
    
    switch (event->type) {
        case CTRL_EVENT_ALARM:
            break;
            
        case CTRL_EVENT_SYSTEM_ERROR:
            g_app.initErrors |= (1U << event->source);
            break;
            
        default:
            break;
    }
}

/**
 * @brief Initialize all RTOS tasks
 * 
 * @details Creates and initializes all application tasks and their
 * associated resources (queues, semaphores, timers).
 * 
 * @return true if all tasks created successfully
 */
static bool Tasks_Init(void)
{
    if (!SensorTask_Init()) {
        return false;
    }
    
    if (NetworkTask_Init() != NET_OK) {
        return false;
    }
    
    if (!ControllerTask_Init()) {
        return false;
    }
    
    SensorTask_RegisterCallback(SensorEvent_Callback, NULL);
    NetworkTask_RegisterCallback(NetworkEvent_Callback, NULL);
    ControllerTask_RegisterCallback(ControllerEvent_Callback, NULL);
    
    if (!SensorTask_Create()) {
        return false;
    }
    
    if (!NetworkTask_Create()) {
        return false;
    }
    
    if (!ControllerTask_Create()) {
        return false;
    }
    
    return true;
}

/**
 * @brief Register default sensors
 * 
 * @details Registers the default set of sensors with the sensor task.
 */
static void RegisterDefaultSensors(void)
{
    SensorConfig_t tempConfig = {
        .type = SENSOR_TYPE_TEMPERATURE,
        .instance = 0,
        .sampleIntervalMs = 1000,
        .oversampling = 4,
        .enableFilter = true,
        .enableThreshold = true,
        .threshold = {
            .lowThreshold = -10.0f,
            .highThreshold = 50.0f,
            .hysteresis = 1.0f,
            .lowEnabled = true,
            .highEnabled = true
        }
    };
    
    uint8_t sensorId;
    SensorTask_RegisterSensor(&tempConfig, &sensorId);
    SensorTask_Start(sensorId);
    
    SensorConfig_t humidityConfig = {
        .type = SENSOR_TYPE_HUMIDITY,
        .instance = 0,
        .sampleIntervalMs = 2000,
        .oversampling = 2,
        .enableFilter = true,
        .enableThreshold = false
    };
    
    SensorTask_RegisterSensor(&humidityConfig, &sensorId);
    SensorTask_Start(sensorId);
}

/**
 * @brief Configure default rules
 * 
 * @details Sets up default automation rules in the controller.
 */
static void ConfigureDefaultRules(void)
{
    ControllerRule_t tempAlarmRule = {
        .id = 1,
        .name = "TempHighAlarm",
        .condition = {
            .sensorId = 0,
            .op = CTRL_OP_GREATER,
            .value = 45.0f
        },
        .actions = {
            { .type = CTRL_ACTION_TRIGGER_ALARM, .targetId = 1, .parameter = 100 }
        },
        .actionCount = 1,
        .enabled = true,
        .oneShot = false
    };
    ControllerTask_RegisterRule(&tempAlarmRule);
    
    ControllerRule_t sendDataRule = {
        .id = 2,
        .name = "SendData",
        .condition = {
            .sensorId = 0,
            .op = CTRL_OP_ALWAYS,
            .value = 0
        },
        .actions = {
            { .type = CTRL_ACTION_SEND_DATA, .targetId = 0 }
        },
        .actionCount = 1,
        .enabled = true,
        .oneShot = false
    };
    ControllerTask_RegisterRule(&sendDataRule);
}

/**
 * @brief Main application task function
 * 
 * @details This task runs after all other tasks have been created.
 * It handles application-level coordination and monitoring.
 * 
 * @param pvParameters Task parameters (unused)
 */
static void App_MainTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t lastWakeTime = xTaskGetTickCount();
    uint32_t watchdogCounter = 0;
    
    g_app.state = APP_STATE_INITIALIZING;
    
    RegisterDefaultSensors();
    ConfigureDefaultRules();
    
    g_app.statusTimer = xTimerCreate(
        "StatusTimer",
        pdMS_TO_TICKS(STATUS_REPORT_INTERVAL_MS),
        pdTRUE,
        NULL,
        StatusTimer_Callback
    );
    
    if (g_app.statusTimer != NULL) {
        xTimerStart(g_app.statusTimer, 0);
    }
    
    g_app.state = APP_STATE_RUNNING;
    g_app.allTasksRunning = true;
    
    while (g_app.state == APP_STATE_RUNNING) {
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(WATCHDOG_FEED_INTERVAL_MS));
        
        watchdogCounter++;
        if (watchdogCounter >= (WATCHDOG_FEED_INTERVAL_MS / 1000)) {
            Watchdog_Feed();
            watchdogCounter = 0;
        }
    }
    
    if (g_app.statusTimer != NULL) {
        xTimerStop(g_app.statusTimer, pdMS_TO_TICKS(100));
        xTimerDelete(g_app.statusTimer, pdMS_TO_TICKS(100));
    }
    
    vTaskDelete(NULL);
}

/**
 * @brief Create the main application task
 * 
 * @return true on success
 */
static bool App_CreateMainTask(void)
{
    BaseType_t result = xTaskCreate(
        App_MainTask,
        APP_TASK_NAME,
        APP_TASK_STACK_SIZE,
        NULL,
        APP_TASK_PRIORITY,
        &g_app.appTaskHandle
    );
    
    return (result == pdPASS);
}

/*============================================================================*/
/*                              PUBLIC FUNCTIONS                              */
/*============================================================================*/

/**
 * @brief Application entry point
 * 
 * @details This is the main entry point for the RTOS application.
 * It performs hardware initialization and starts the FreeRTOS scheduler.
 * 
 * Initialization Sequence:
 * 1. Hardware initialization (clocks, GPIO, peripherals)
 * 2. RTOS task creation
 * 3. Start FreeRTOS scheduler
 * 
 * @return Never returns in embedded systems
 */
int main(void)
{
    g_app.state = APP_STATE_UNINITIALIZED;
    
    Clock_Init();
    GPIO_Init();
    
    if (!Hardware_Init()) {
        while (1) {
        }
    }
    
    Watchdog_Init();
    
    g_app.state = APP_STATE_INITIALIZING;
    
    if (!Tasks_Init()) {
        g_app.state = APP_STATE_ERROR;
        while (1) {
        }
    }
    
    if (!App_CreateMainTask()) {
        g_app.state = APP_STATE_ERROR;
        while (1) {
        }
    }
    
    vTaskStartScheduler();
    
    while (1) {
    }
    
    return 0;
}

/*============================================================================*/
/*                              FREERTOS HOOKS                                */
/*============================================================================*/

/**
 * @brief Stack overflow hook
 * 
 * @details Called when a stack overflow is detected.
 * 
 * @param xTask Task handle
 * @param pcTaskName Task name
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    
    while (1) {
    }
}

/**
 * @brief Malloc failed hook
 * 
 * @details Called when pvPortMalloc() fails.
 */
void vApplicationMallocFailedHook(void)
{
    while (1) {
    }
}

/**
 * @brief Idle hook
 * 
 * @details Called repeatedly from the idle task.
 */
void vApplicationIdleHook(void)
{
}

/**
 * @brief Tick hook
 * 
 * @details Called from the tick ISR.
 */
void vApplicationTickHook(void)
{
}

/**
 * @brief Get idle task memory hook
 * 
 * @details Provides static memory for idle task when using static allocation.
 */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize)
{
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
    
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/**
 * @brief Get timer task memory hook
 * 
 * @details Provides static memory for timer task when using static allocation.
 */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize)
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
    
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
