/**
 * @file task_sensor.c
 * @brief Sensor Task Implementation - FreeRTOS-based sensor acquisition
 * 
 * @details Implements the sensor task that handles periodic sensor reading,
 * data filtering, calibration, and threshold monitoring.
 * 
 * Key Features:
 * - Multiple sensor management
 * - Configurable sampling intervals
 * - Moving average filtering
 * - Threshold detection with hysteresis
 * - Calibration support
 * - Event-driven notifications
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#include "task_sensor.h"
#include <string.h>
#include <stdlib.h>

/*============================================================================*/
/*                              FREERTOS INCLUDES                             */
/*============================================================================*/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/*============================================================================*/
/*                              PRIVATE DEFINES                               */
/*============================================================================*/

/** @brief Minimum sampling interval in milliseconds */
#define SENSOR_MIN_SAMPLE_MS        (10U)

/** @brief Task processing tick interval */
#define SENSOR_TICK_MS              (10U)

/*============================================================================*/
/*                              PRIVATE TYPES                                 */
/*============================================================================*/

/**
 * @brief Internal sensor instance structure
 */
typedef struct {
    SensorConfig_t      config;         /**< Configuration */
    SensorInfo_t        info;           /**< Sensor info */
    SensorStats_t       stats;          /**< Statistics */
    SensorSample_t      lastSample;     /**< Last sample */
    
    float               filterBuffer[SENSOR_FILTER_SIZE]; /**< Filter history */
    uint8_t             filterIndex;    /**< Filter buffer index */
    uint8_t             filterCount;    /**< Valid samples in buffer */
    
    uint32_t            nextSampleTime; /**< Next sample time (ticks) */
    bool                active;         /**< Sensor active flag */
    bool                thresholdHighActive; /**< High threshold active */
    bool                thresholdLowActive;  /**< Low threshold active */
} SensorInstance_t;

/**
 * @brief Sensor task context structure
 */
typedef struct {
    TaskHandle_t        taskHandle;     /**< FreeRTOS task handle */
    QueueHandle_t       dataQueue;      /**< Data output queue */
    QueueHandle_t       cmdQueue;       /**< Command input queue */
    SemaphoreHandle_t   mutex;          /**< Resource mutex */
    
    SensorInstance_t    sensors[SENSOR_MAX_COUNT]; /**< Sensor instances */
    uint8_t             sensorCount;    /**< Active sensor count */
    
    SensorEventCallback_t callback;     /**< Event callback */
    void*               callbackUserData; /**< Callback user data */
    
    bool                initialized;    /**< Initialization flag */
    bool                running;        /**< Task running flag */
} SensorContext_t;

/*============================================================================*/
/*                              PRIVATE VARIABLES                             */
/*============================================================================*/

/** @brief Sensor task context instance */
static SensorContext_t s_sensorCtx = {0};

/*============================================================================*/
/*                              PRIVATE FUNCTIONS                             */
/*============================================================================*/

/**
 * @brief Find a free sensor slot
 * 
 * @return Slot index or -1 if none available
 */
static int8_t Sensor_FindFreeSlot(void)
{
    for (uint8_t i = 0; i < SENSOR_MAX_COUNT; i++) {
        if (!s_sensorCtx.sensors[i].active) {
            return (int8_t)i;
        }
    }
    return -1;
}

/**
 * @brief Get sensor instance by ID
 * 
 * @param sensorId Sensor ID
 * @return Pointer to instance or NULL
 */
static SensorInstance_t* Sensor_GetInstance(uint8_t sensorId)
{
    if (sensorId < SENSOR_MAX_COUNT && s_sensorCtx.sensors[sensorId].active) {
        return &s_sensorCtx.sensors[sensorId];
    }
    return NULL;
}

/**
 * @brief Get sensor name by type
 * 
 * @param type Sensor type
 * @param name Output buffer for name
 */
static void Sensor_GetTypeName(SensorType_e type, char *name)
{
    const char *typeNames[] = {
        "None", "Temperature", "Humidity", "Pressure",
        "Light", "Accel", "Gyro", "Mag",
        "Prox", "Gas", "Current", "Voltage"
    };
    
    if (type < SENSOR_TYPE_COUNT) {
        strncpy(name, typeNames[type], SENSOR_NAME_LENGTH - 1);
    } else {
        strncpy(name, "Unknown", SENSOR_NAME_LENGTH - 1);
    }
}

/**
 * @brief Initialize a sensor instance
 * 
 * @param sensor   Sensor instance to initialize
 * @param config   Configuration
 * @param id       Sensor ID
 */
static void Sensor_InitInstance(SensorInstance_t *sensor,
                                  const SensorConfig_t *config,
                                  uint8_t id)
{
    memset(sensor, 0, sizeof(SensorInstance_t));
    sensor->config = *config;
    sensor->active = true;
    
    Sensor_GetTypeName(config->type, sensor->info.name);
    sensor->info.type = config->type;
    sensor->info.instance = config->instance;
    sensor->info.state = SENSOR_STATE_READY;
    sensor->info.available = true;
    
    sensor->config.offset = 0.0f;
    sensor->config.scale = 1.0f;
    
    sensor->nextSampleTime = xTaskGetTickCount();
}

/**
 * @brief Update moving average filter
 * 
 * @param sensor    Sensor instance
 * @param value     New value
 * @return Filtered value
 */
static float Sensor_UpdateFilter(SensorInstance_t *sensor, float value)
{
    if (!sensor->config.enableFilter) {
        return value;
    }
    
    sensor->filterBuffer[sensor->filterIndex] = value;
    sensor->filterIndex = (sensor->filterIndex + 1U) % SENSOR_FILTER_SIZE;
    
    if (sensor->filterCount < SENSOR_FILTER_SIZE) {
        sensor->filterCount++;
    }
    
    float sum = 0.0f;
    for (uint8_t i = 0; i < sensor->filterCount; i++) {
        sum += sensor->filterBuffer[i];
    }
    
    return sum / (float)sensor->filterCount;
}

/**
 * @brief Check threshold conditions
 * 
 * @param sensor Sensor instance
 * @param value  Current value
 * @return true if threshold event occurred
 */
static bool Sensor_CheckThreshold(SensorInstance_t *sensor, float value)
{
    if (!sensor->config.enableThreshold) {
        return false;
    }
    
    bool thresholdEvent = false;
    const SensorThreshold_t *thresh = &sensor->config.threshold;
    
    if (thresh->highEnabled) {
        if (value > thresh->highThreshold + thresh->hysteresis) {
            if (!sensor->thresholdHighActive) {
                sensor->thresholdHighActive = true;
                thresholdEvent = true;
            }
        } else if (value < thresh->highThreshold - thresh->hysteresis) {
            sensor->thresholdHighActive = false;
        }
    }
    
    if (thresh->lowEnabled) {
        if (value < thresh->lowThreshold - thresh->hysteresis) {
            if (!sensor->thresholdLowActive) {
                sensor->thresholdLowActive = true;
                thresholdEvent = true;
            }
        } else if (value > thresh->lowThreshold + thresh->hysteresis) {
            sensor->thresholdLowActive = false;
        }
    }
    
    return thresholdEvent;
}

/**
 * @brief Read sensor value (simulated)
 * 
 * @param sensor    Sensor instance
 * @param pValue    Output pointer for value
 * @return SENSOR_OK on success
 */
static SensorError_e Sensor_ReadValue(SensorInstance_t *sensor, float *pValue)
{
    switch (sensor->config.type) {
        case SENSOR_TYPE_TEMPERATURE:
            *pValue = 25.0f + (float)(rand() % 100 - 50) / 10.0f;
            break;
        case SENSOR_TYPE_HUMIDITY:
            *pValue = 50.0f + (float)(rand() % 100 - 50) / 5.0f;
            break;
        case SENSOR_TYPE_PRESSURE:
            *pValue = 101325.0f + (float)(rand() % 200 - 100);
            break;
        case SENSOR_TYPE_LIGHT:
            *pValue = 500.0f + (float)(rand() % 500);
            break;
        default:
            *pValue = 0.0f;
            break;
    }
    
    return SENSOR_OK;
}

/**
 * @brief Sample a single sensor
 * 
 * @param sensor Sensor instance to sample
 */
static void Sensor_SampleSensor(SensorInstance_t *sensor)
{
    float rawValue, calibratedValue;
    SensorError_e error;
    
    sensor->info.state = SENSOR_STATE_SAMPLING;
    
    error = Sensor_ReadValue(sensor, &rawValue);
    
    if (error != SENSOR_OK) {
        sensor->stats.errorCount++;
        sensor->stats.lastError = error;
        sensor->stats.lastErrorTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
        sensor->info.state = SENSOR_STATE_ERROR;
        
        if (s_sensorCtx.callback != NULL) {
            SensorEvent_t event = {
                .type = SENSOR_EVENT_ERROR,
                .error = error,
                .sensorType = sensor->config.type,
                .instance = sensor->config.instance
            };
            s_sensorCtx.callback(&event, s_sensorCtx.callbackUserData);
        }
        return;
    }
    
    calibratedValue = (rawValue + sensor->config.offset) * sensor->config.scale;
    
    sensor->lastSample.rawValue = rawValue;
    sensor->lastSample.value = calibratedValue;
    sensor->lastSample.filteredValue = Sensor_UpdateFilter(sensor, calibratedValue);
    sensor->lastSample.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    sensor->lastSample.type = sensor->config.type;
    sensor->lastSample.instance = sensor->config.instance;
    sensor->lastSample.quality = 100;
    sensor->lastSample.flags = 0;
    
    sensor->stats.sampleCount++;
    sensor->stats.lastSampleTime = sensor->lastSample.timestamp;
    
    if (sensor->stats.sampleCount == 1) {
        sensor->stats.minValue = calibratedValue;
        sensor->stats.maxValue = calibratedValue;
        sensor->stats.avgValue = calibratedValue;
    } else {
        if (calibratedValue < sensor->stats.minValue) {
            sensor->stats.minValue = calibratedValue;
        }
        if (calibratedValue > sensor->stats.maxValue) {
            sensor->stats.maxValue = calibratedValue;
        }
        sensor->stats.avgValue = sensor->stats.avgValue * 0.9f + calibratedValue * 0.1f;
    }
    
    bool thresholdEvent = Sensor_CheckThreshold(sensor, calibratedValue);
    if (thresholdEvent) {
        sensor->stats.thresholdCrossings++;
        
        if (s_sensorCtx.callback != NULL) {
            SensorEvent_t event = {
                .type = SENSOR_EVENT_THRESHOLD,
                .sensorType = sensor->config.type,
                .instance = sensor->config.instance,
                .sample = sensor->lastSample
            };
            s_sensorCtx.callback(&event, s_sensorCtx.callbackUserData);
        }
    }
    
    if (s_sensorCtx.dataQueue != NULL) {
        xQueueSend(s_sensorCtx.dataQueue, &sensor->lastSample, 0);
    }
    
    if (s_sensorCtx.callback != NULL) {
        SensorEvent_t event = {
            .type = SENSOR_EVENT_DATA_READY,
            .sensorType = sensor->config.type,
            .instance = sensor->config.instance,
            .sample = sensor->lastSample
        };
        s_sensorCtx.callback(&event, s_sensorCtx.callbackUserData);
    }
    
    sensor->info.state = SENSOR_STATE_READY;
}

/**
 * @brief Process all sensors due for sampling
 */
static void Sensor_ProcessSampling(void)
{
    TickType_t now = xTaskGetTickCount();
    
    for (uint8_t i = 0; i < SENSOR_MAX_COUNT; i++) {
        SensorInstance_t *sensor = &s_sensorCtx.sensors[i];
        
        if (sensor->active && sensor->info.state != SENSOR_STATE_DISABLED) {
            if ((int32_t)(now - sensor->nextSampleTime) >= 0) {
                Sensor_SampleSensor(sensor);
                sensor->nextSampleTime = now + pdMS_TO_TICKS(sensor->config.sampleIntervalMs);
            }
        }
    }
}

/**
 * @brief Handle command events
 * 
 * @param event Command event
 */
static void Sensor_HandleCommand(const SensorEvent_t *event)
{
    switch (event->type) {
        case SENSOR_EVENT_START:
            {
                SensorInstance_t *sensor = Sensor_GetInstance(event->instance);
                if (sensor != NULL) {
                    sensor->info.state = SENSOR_STATE_READY;
                    sensor->nextSampleTime = xTaskGetTickCount();
                }
            }
            break;
            
        case SENSOR_EVENT_STOP:
            {
                if (event->instance == 0xFF) {
                    for (uint8_t i = 0; i < SENSOR_MAX_COUNT; i++) {
                        if (s_sensorCtx.sensors[i].active) {
                            s_sensorCtx.sensors[i].info.state = SENSOR_STATE_DISABLED;
                        }
                    }
                } else {
                    SensorInstance_t *sensor = Sensor_GetInstance(event->instance);
                    if (sensor != NULL) {
                        sensor->info.state = SENSOR_STATE_DISABLED;
                    }
                }
            }
            break;
            
        case SENSOR_EVENT_CALIBRATE:
            {
                SensorInstance_t *sensor = Sensor_GetInstance(event->instance);
                if (sensor != NULL) {
                    float sum = 0.0f;
                    float value;
                    for (uint8_t i = 0; i < 10; i++) {
                        Sensor_ReadValue(sensor, &value);
                        sum += value;
                    }
                    sensor->config.offset = -(sum / 10.0f);
                }
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief Main sensor task function
 * 
 * @param pvParameters Task parameters
 */
static void Sensor_TaskFunction(void *pvParameters)
{
    (void)pvParameters;
    SensorEvent_t cmdEvent;
    
    s_sensorCtx.running = true;
    
    while (s_sensorCtx.running) {
        if (xQueueReceive(s_sensorCtx.cmdQueue, &cmdEvent, 
                          pdMS_TO_TICKS(SENSOR_TICK_MS)) == pdPASS) {
            Sensor_HandleCommand(&cmdEvent);
        }
        
        if (xSemaphoreTake(s_sensorCtx.mutex, pdMS_TO_TICKS(10)) == pdPASS) {
            Sensor_ProcessSampling();
            xSemaphoreGive(s_sensorCtx.mutex);
        }
    }
    
    vTaskDelete(NULL);
}

/*============================================================================*/
/*                              PUBLIC API IMPLEMENTATION                     */
/*============================================================================*/

bool SensorTask_Init(void)
{
    if (s_sensorCtx.initialized) {
        return true;
    }
    
    memset(&s_sensorCtx, 0, sizeof(SensorContext_t));
    
    s_sensorCtx.dataQueue = xQueueCreate(SENSOR_DATA_QUEUE_LENGTH, sizeof(SensorSample_t));
    if (s_sensorCtx.dataQueue == NULL) {
        return false;
    }
    
    s_sensorCtx.cmdQueue = xQueueCreate(SENSOR_DATA_QUEUE_LENGTH, sizeof(SensorEvent_t));
    if (s_sensorCtx.cmdQueue == NULL) {
        vQueueDelete(s_sensorCtx.dataQueue);
        return false;
    }
    
    s_sensorCtx.mutex = xSemaphoreCreateMutex();
    if (s_sensorCtx.mutex == NULL) {
        vQueueDelete(s_sensorCtx.dataQueue);
        vQueueDelete(s_sensorCtx.cmdQueue);
        return false;
    }
    
    s_sensorCtx.initialized = true;
    return true;
}

bool SensorTask_Create(void)
{
    if (!s_sensorCtx.initialized) {
        if (!SensorTask_Init()) {
            return false;
        }
    }
    
    BaseType_t result = xTaskCreate(
        Sensor_TaskFunction,
        SENSOR_TASK_NAME,
        SENSOR_TASK_STACK_SIZE,
        NULL,
        SENSOR_TASK_PRIORITY,
        &s_sensorCtx.taskHandle
    );
    
    return (result == pdPASS);
}

void* SensorTask_GetHandle(void)
{
    return s_sensorCtx.taskHandle;
}

SensorError_e SensorTask_RegisterSensor(const SensorConfig_t *config, uint8_t *pSensorId)
{
    if (config == NULL || pSensorId == NULL) {
        return SENSOR_ERR_CONFIG;
    }
    
    if (xSemaphoreTake(s_sensorCtx.mutex, pdMS_TO_TICKS(100)) != pdPASS) {
        return SENSOR_ERR_BUSY;
    }
    
    int8_t slot = Sensor_FindFreeSlot();
    if (slot < 0) {
        xSemaphoreGive(s_sensorCtx.mutex);
        return SENSOR_ERR_NOT_FOUND;
    }
    
    Sensor_InitInstance(&s_sensorCtx.sensors[slot], config, (uint8_t)slot);
    s_sensorCtx.sensorCount++;
    
    *pSensorId = (uint8_t)slot;
    
    xSemaphoreGive(s_sensorCtx.mutex);
    return SENSOR_OK;
}

SensorError_e SensorTask_UnregisterSensor(uint8_t sensorId)
{
    SensorInstance_t *sensor = Sensor_GetInstance(sensorId);
    if (sensor == NULL) {
        return SENSOR_ERR_NOT_FOUND;
    }
    
    sensor->active = false;
    s_sensorCtx.sensorCount--;
    
    return SENSOR_OK;
}

SensorError_e SensorTask_GetSample(uint8_t sensorId, SensorSample_t *pSample)
{
    if (pSample == NULL) {
        return SENSOR_ERR_CONFIG;
    }
    
    SensorInstance_t *sensor = Sensor_GetInstance(sensorId);
    if (sensor == NULL) {
        return SENSOR_ERR_NOT_FOUND;
    }
    
    *pSample = sensor->lastSample;
    return SENSOR_OK;
}

SensorError_e SensorTask_GetStats(uint8_t sensorId, SensorStats_t *pStats)
{
    if (pStats == NULL) {
        return SENSOR_ERR_CONFIG;
    }
    
    SensorInstance_t *sensor = Sensor_GetInstance(sensorId);
    if (sensor == NULL) {
        return SENSOR_ERR_NOT_FOUND;
    }
    
    *pStats = sensor->stats;
    return SENSOR_OK;
}

SensorError_e SensorTask_GetInfo(uint8_t sensorId, SensorInfo_t *pInfo)
{
    if (pInfo == NULL) {
        return SENSOR_ERR_CONFIG;
    }
    
    SensorInstance_t *sensor = Sensor_GetInstance(sensorId);
    if (sensor == NULL) {
        return SENSOR_ERR_NOT_FOUND;
    }
    
    *pInfo = sensor->info;
    return SENSOR_OK;
}

SensorError_e SensorTask_SetInterval(uint8_t sensorId, uint32_t intervalMs)
{
    if (intervalMs < SENSOR_MIN_SAMPLE_MS) {
        return SENSOR_ERR_CONFIG;
    }
    
    SensorInstance_t *sensor = Sensor_GetInstance(sensorId);
    if (sensor == NULL) {
        return SENSOR_ERR_NOT_FOUND;
    }
    
    sensor->config.sampleIntervalMs = intervalMs;
    return SENSOR_OK;
}

SensorError_e SensorTask_SetThreshold(uint8_t sensorId, const SensorThreshold_t *threshold)
{
    if (threshold == NULL) {
        return SENSOR_ERR_CONFIG;
    }
    
    SensorInstance_t *sensor = Sensor_GetInstance(sensorId);
    if (sensor == NULL) {
        return SENSOR_ERR_NOT_FOUND;
    }
    
    sensor->config.threshold = *threshold;
    sensor->config.enableThreshold = true;
    return SENSOR_OK;
}

SensorError_e SensorTask_Start(uint8_t sensorId)
{
    SensorEvent_t event = {
        .type = SENSOR_EVENT_START,
        .instance = sensorId
    };
    
    if (xQueueSend(s_sensorCtx.cmdQueue, &event, pdMS_TO_TICKS(100)) != pdPASS) {
        return SENSOR_ERR_BUSY;
    }
    
    return SENSOR_OK;
}

SensorError_e SensorTask_Stop(uint8_t sensorId)
{
    SensorEvent_t event = {
        .type = SENSOR_EVENT_STOP,
        .instance = sensorId
    };
    
    if (xQueueSend(s_sensorCtx.cmdQueue, &event, pdMS_TO_TICKS(100)) != pdPASS) {
        return SENSOR_ERR_BUSY;
    }
    
    return SENSOR_OK;
}

SensorError_e SensorTask_Calibrate(uint8_t sensorId)
{
    SensorEvent_t event = {
        .type = SENSOR_EVENT_CALIBRATE,
        .instance = sensorId
    };
    
    if (xQueueSend(s_sensorCtx.cmdQueue, &event, pdMS_TO_TICKS(100)) != pdPASS) {
        return SENSOR_ERR_BUSY;
    }
    
    return SENSOR_OK;
}

SensorError_e SensorTask_SelfTest(uint8_t sensorId, bool *pResult)
{
    SensorInstance_t *sensor = Sensor_GetInstance(sensorId);
    if (sensor == NULL) {
        return SENSOR_ERR_NOT_FOUND;
    }
    
    *pResult = sensor->active;
    return SENSOR_OK;
}

bool SensorTask_RegisterCallback(SensorEventCallback_t callback, void *userData)
{
    if (xSemaphoreTake(s_sensorCtx.mutex, pdMS_TO_TICKS(100)) == pdPASS) {
        s_sensorCtx.callback = callback;
        s_sensorCtx.callbackUserData = userData;
        xSemaphoreGive(s_sensorCtx.mutex);
        return true;
    }
    return false;
}

void* SensorTask_GetDataQueue(void)
{
    return s_sensorCtx.dataQueue;
}
