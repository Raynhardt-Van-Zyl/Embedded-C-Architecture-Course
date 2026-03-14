/**
 * @file sensor_driver.c
 * @brief Sensor Driver Implementation - Concrete driver implementations
 * 
 * @details This file implements the sensor driver interface with support for
 * multiple sensor types. It demonstrates the factory pattern for driver
 * instantiation and the strategy pattern for polymorphic behavior.
 * 
 * Key Implementation Features:
 * - Virtual function table for polymorphism
 * - Circular buffer for data filtering
 * - CRC validation for data integrity
 * - Non-blocking state machine
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#include "sensor_driver.h"
#include <string.h>
#include <stdlib.h>

/*============================================================================*/
/*                              PRIVATE DEFINES                               */
/*============================================================================*/

/** @brief Maximum number of concurrent sensor instances */
#define SENSOR_MAX_INSTANCES        (8U)

/** @brief CRC-8 polynomial for data validation */
#define SENSOR_CRC8_POLYNOMIAL      (0x07U)

/** @brief Default sample rate if not specified */
#define SENSOR_DEFAULT_SAMPLE_RATE  (10U)

/** @brief Filter coefficient for exponential moving average */
#define SENSOR_FILTER_ALPHA         (0.125f)

/*============================================================================*/
/*                              PRIVATE TYPES                                 */
/*============================================================================*/

/**
 * @brief Virtual function table for sensor operations
 * 
 * @details Function pointer table enabling polymorphic behavior.
 * Each sensor type provides its own implementation.
 */
typedef struct {
    SensorStatus_t (*init)(SensorHandle_t *handle);
    SensorStatus_t (*deinit)(SensorHandle_t *handle);
    SensorStatus_t (*read)(SensorHandle_t *handle, float *value);
    SensorStatus_t (*readVector)(SensorHandle_t *handle, Vector3D_t *vector);
    SensorStatus_t (*readRaw)(SensorHandle_t *handle, int32_t *raw);
    SensorStatus_t (*trigger)(SensorHandle_t *handle);
    SensorStatus_t (*isDataReady)(SensorHandle_t *handle, bool *ready);
    SensorStatus_t (*sleep)(SensorHandle_t *handle);
    SensorStatus_t (*wake)(SensorHandle_t *handle);
    SensorStatus_t (*selfTest)(SensorHandle_t *handle, bool *result);
    SensorStatus_t (*reset)(SensorHandle_t *handle);
} SensorVTable_t;

/**
 * @brief Internal sensor handle structure
 * 
 * @details Complete sensor state including configuration, calibration,
 * and runtime data. Hidden from application code for encapsulation.
 */
struct SensorHandle {
    SensorInfo_t        info;                   /**< Sensor metadata */
    SensorConfig_t      config;                 /**< Current configuration */
    SensorCalibration_t calibration;            /**< Calibration data */
    SensorStats_t       stats;                  /**< Runtime statistics */
    SensorState_t       state;                  /**< Current state */
    const SensorVTable_t *vtable;               /**< Virtual function table */
    
    SensorDataCallback_t  dataCallback;         /**< Data ready callback */
    SensorErrorCallback_t errorCallback;        /**< Error callback */
    void                 *callbackUserData;     /**< Callback user data */
    
    float               filterBuffer[SENSOR_FILTER_SAMPLES]; /**< Filter history */
    uint8_t             filterIndex;            /**< Current filter position */
    uint8_t             filterCount;            /**< Samples in buffer */
    bool                filterInitialized;      /**< Filter initialized flag */
    
    uint32_t            measurementStartTime;   /**< Measurement start timestamp */
    bool                measurementPending;     /**< Measurement in progress flag */
    bool                inUse;                  /**< Instance in use flag */
};

/*============================================================================*/
/*                              PRIVATE VARIABLES                             */
/*============================================================================*/

/** @brief Sensor instance pool (static allocation) */
static SensorHandle_t s_sensorPool[SENSOR_MAX_INSTANCES];

/** @brief Subsystem initialization flag */
static bool s_subsystemInitialized = false;

/** @brief CRC-8 lookup table */
static uint8_t s_crc8Table[256];

/*============================================================================*/
/*                              PRIVATE FUNCTIONS                             */
/*============================================================================*/

/**
 * @brief Initialize CRC-8 lookup table
 * 
 * @details Pre-computes CRC-8 values for fast runtime calculation.
 * Uses polynomial 0x07 (CRC-8-CCITT).
 */
static void Sensor_InitCRC8(void)
{
    for (uint16_t i = 0; i < 256; i++) {
        uint8_t crc = (uint8_t)i;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80U) {
                crc = (crc << 1U) ^ SENSOR_CRC8_POLYNOMIAL;
            } else {
                crc <<= 1U;
            }
        }
        s_crc8Table[i] = crc;
    }
}

/**
 * @brief Calculate CRC-8 checksum
 * 
 * @param data Pointer to data buffer
 * @param len  Length of data in bytes
 * @return CRC-8 checksum
 */
static uint8_t Sensor_CalculateCRC8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00U;
    for (size_t i = 0; i < len; i++) {
        crc = s_crc8Table[crc ^ data[i]];
    }
    return crc;
}

/**
 * @brief Find free sensor instance in pool
 * 
 * @return Pointer to free instance, or NULL if pool exhausted
 */
static SensorHandle_t* Sensor_FindFreeInstance(void)
{
    for (uint8_t i = 0; i < SENSOR_MAX_INSTANCES; i++) {
        if (!s_sensorPool[i].inUse) {
            return &s_sensorPool[i];
        }
    }
    return NULL;
}

/**
 * @brief Apply calibration to raw value
 * 
 * @param handle   Sensor handle with calibration data
 * @param rawValue Raw sensor reading
 * @return Calibrated value
 */
static float Sensor_ApplyCalibrationValue(const SensorHandle_t *handle, float rawValue)
{
    if (handle->calibration.isValid) {
        return (rawValue - handle->calibration.offset) * handle->calibration.scale;
    }
    return rawValue;
}

/**
 * @brief Update moving average filter
 * 
 * @param handle Sensor handle with filter state
 * @param value  New value to add to filter
 * @return Filtered value
 */
static float Sensor_UpdateFilter(SensorHandle_t *handle, float value)
{
    if (!handle->config.enableFiltering) {
        return value;
    }
    
    handle->filterBuffer[handle->filterIndex] = value;
    handle->filterIndex = (handle->filterIndex + 1U) % SENSOR_FILTER_SAMPLES;
    
    if (handle->filterCount < SENSOR_FILTER_SAMPLES) {
        handle->filterCount++;
    }
    
    float sum = 0.0f;
    for (uint8_t i = 0; i < handle->filterCount; i++) {
        sum += handle->filterBuffer[i];
    }
    
    return sum / (float)handle->filterCount;
}

/**
 * @brief Update sensor statistics
 * 
 * @param handle    Sensor handle to update
 * @param value     New value to incorporate
 * @param timestamp Timestamp of the reading
 */
static void Sensor_UpdateStats(SensorHandle_t *handle, float value, uint32_t timestamp)
{
    handle->stats.readCount++;
    handle->stats.lastValue = value;
    handle->stats.lastReadTime = timestamp;
    
    if (handle->stats.readCount == 1U) {
        handle->stats.minValue = value;
        handle->stats.maxValue = value;
        handle->stats.avgValue = value;
    } else {
        if (value < handle->stats.minValue) {
            handle->stats.minValue = value;
        }
        if (value > handle->stats.maxValue) {
            handle->stats.maxValue = value;
        }
        handle->stats.avgValue = (handle->stats.avgValue * (1.0f - SENSOR_FILTER_ALPHA)) 
                                + (value * SENSOR_FILTER_ALPHA);
    }
}

/*============================================================================*/
/*                      FORWARD DECLARATIONS                                  */
/*============================================================================*/

static SensorStatus_t TempSensor_ReadRaw(SensorHandle_t *handle, int32_t *raw);

/*============================================================================*/
/*                      SIMULATED SENSOR IMPLEMENTATIONS                      */
/*============================================================================*/

/**
 * @brief Simulated temperature sensor initialization
 * 
 * @details Simulates hardware initialization with configurable delay.
 * In production, this would configure I2C/SPI registers.
 */
static SensorStatus_t TempSensor_Init(SensorHandle_t *handle)
{
    handle->info.type = SENSOR_TYPE_TEMPERATURE;
    strncpy(handle->info.name, "TMP36", SENSOR_MAX_NAME_LENGTH - 1U);
    strncpy(handle->info.manufacturer, "Analog Devices", 15);
    handle->info.minValue = -40.0f;
    handle->info.maxValue = 125.0f;
    handle->info.resolution = 0.1f;
    handle->info.accuracy = 1.0f;
    handle->info.capabilities = SENSOR_CAP_SLEEP_MODE | SENSOR_CAP_FILTERING;
    
    handle->state = SENSOR_STATE_READY;
    return SENSOR_OK;
}

static SensorStatus_t TempSensor_Deinit(SensorHandle_t *handle)
{
    handle->state = SENSOR_STATE_UNINITIALIZED;
    return SENSOR_OK;
}

static SensorStatus_t TempSensor_Read(SensorHandle_t *handle, float *value)
{
    int32_t rawValue;
    SensorStatus_t status = TempSensor_ReadRaw(handle, &rawValue);
    if (status != SENSOR_OK) {
        return status;
    }
    
    float voltage = (float)rawValue / 1000.0f;
    float tempC = (voltage - 0.5f) * 100.0f;
    
    tempC = Sensor_ApplyCalibrationValue(handle, tempC);
    *value = Sensor_UpdateFilter(handle, tempC);
    
    return SENSOR_OK;
}

static SensorStatus_t TempSensor_ReadRaw(SensorHandle_t *handle, int32_t *raw)
{
    (void)handle;
    *raw = 750;  /* Simulated 750mV = 25°C */
    return SENSOR_OK;
}

static SensorStatus_t TempSensor_ReadVector(SensorHandle_t *handle, Vector3D_t *vector)
{
    (void)handle;
    (void)vector;
    return SENSOR_ERROR_CONFIG;
}

static SensorStatus_t TempSensor_Trigger(SensorHandle_t *handle)
{
    handle->measurementPending = true;
    return SENSOR_OK;
}

static SensorStatus_t TempSensor_IsDataReady(SensorHandle_t *handle, bool *ready)
{
    *ready = !handle->measurementPending;
    return SENSOR_OK;
}

static SensorStatus_t TempSensor_Sleep(SensorHandle_t *handle)
{
    handle->state = SENSOR_STATE_SLEEP;
    return SENSOR_OK;
}

static SensorStatus_t TempSensor_Wake(SensorHandle_t *handle)
{
    handle->state = SENSOR_STATE_READY;
    return SENSOR_OK;
}

static SensorStatus_t TempSensor_SelfTest(SensorHandle_t *handle, bool *result)
{
    (void)handle;
    *result = true;
    return SENSOR_OK;
}

static SensorStatus_t TempSensor_Reset(SensorHandle_t *handle)
{
    handle->state = SENSOR_STATE_READY;
    return SENSOR_OK;
}

/** @brief Temperature sensor virtual function table */
static const SensorVTable_t s_tempVTable = {
    .init = TempSensor_Init,
    .deinit = TempSensor_Deinit,
    .read = TempSensor_Read,
    .readVector = TempSensor_ReadVector,
    .readRaw = TempSensor_ReadRaw,
    .trigger = TempSensor_Trigger,
    .isDataReady = TempSensor_IsDataReady,
    .sleep = TempSensor_Sleep,
    .wake = TempSensor_Wake,
    .selfTest = TempSensor_SelfTest,
    .reset = TempSensor_Reset
};

/**
 * @brief Simulated humidity sensor implementation
 */
static SensorStatus_t HumiditySensor_Init(SensorHandle_t *handle)
{
    handle->info.type = SENSOR_TYPE_HUMIDITY;
    strncpy(handle->info.name, "SHT31", SENSOR_MAX_NAME_LENGTH - 1U);
    strncpy(handle->info.manufacturer, "Sensirion", 15);
    handle->info.minValue = 0.0f;
    handle->info.maxValue = 100.0f;
    handle->info.resolution = 0.1f;
    handle->info.accuracy = 2.0f;
    handle->info.capabilities = SENSOR_CAP_SLEEP_MODE | SENSOR_CAP_INTERRUPT 
                              | SENSOR_CAP_SELF_TEST | SENSOR_CAP_TEMPCO_COMP;
    
    handle->state = SENSOR_STATE_READY;
    return SENSOR_OK;
}

static SensorStatus_t HumiditySensor_Deinit(SensorHandle_t *handle)
{
    handle->state = SENSOR_STATE_UNINITIALIZED;
    return SENSOR_OK;
}

static SensorStatus_t HumiditySensor_Read(SensorHandle_t *handle, float *value)
{
    (void)handle;
    *value = 45.0f + (float)(rand() % 100) / 100.0f - 0.5f;
    *value = Sensor_UpdateFilter(handle, *value);
    return SENSOR_OK;
}

static SensorStatus_t HumiditySensor_ReadRaw(SensorHandle_t *handle, int32_t *raw)
{
    (void)handle;
    *raw = 18000;  /* Simulated raw ADC value */
    return SENSOR_OK;
}

static SensorStatus_t HumiditySensor_ReadVector(SensorHandle_t *handle, Vector3D_t *vector)
{
    (void)handle; (void)vector;
    return SENSOR_ERROR_CONFIG;
}

static SensorStatus_t HumiditySensor_Trigger(SensorHandle_t *handle)
{
    handle->measurementPending = true;
    return SENSOR_OK;
}

static SensorStatus_t HumiditySensor_IsDataReady(SensorHandle_t *handle, bool *ready)
{
    *ready = !handle->measurementPending;
    return SENSOR_OK;
}

static SensorStatus_t HumiditySensor_Sleep(SensorHandle_t *handle)
{
    handle->state = SENSOR_STATE_SLEEP;
    return SENSOR_OK;
}

static SensorStatus_t HumiditySensor_Wake(SensorHandle_t *handle)
{
    handle->state = SENSOR_STATE_READY;
    return SENSOR_OK;
}

static SensorStatus_t HumiditySensor_SelfTest(SensorHandle_t *handle, bool *result)
{
    (void)handle;
    *result = true;
    return SENSOR_OK;
}

static SensorStatus_t HumiditySensor_Reset(SensorHandle_t *handle)
{
    handle->state = SENSOR_STATE_READY;
    return SENSOR_OK;
}

static const SensorVTable_t s_humidityVTable = {
    .init = HumiditySensor_Init,
    .deinit = HumiditySensor_Deinit,
    .read = HumiditySensor_Read,
    .readVector = HumiditySensor_ReadVector,
    .readRaw = HumiditySensor_ReadRaw,
    .trigger = HumiditySensor_Trigger,
    .isDataReady = HumiditySensor_IsDataReady,
    .sleep = HumiditySensor_Sleep,
    .wake = HumiditySensor_Wake,
    .selfTest = HumiditySensor_SelfTest,
    .reset = HumiditySensor_Reset
};

/*============================================================================*/
/*                              PUBLIC API IMPLEMENTATION                     */
/*============================================================================*/

SensorStatus_t Sensor_InitSubsystem(void)
{
    if (s_subsystemInitialized) {
        return SENSOR_OK;
    }
    
    Sensor_InitCRC8();
    memset(s_sensorPool, 0, sizeof(s_sensorPool));
    s_subsystemInitialized = true;
    
    return SENSOR_OK;
}

void Sensor_DeinitSubsystem(void)
{
    for (uint8_t i = 0; i < SENSOR_MAX_INSTANCES; i++) {
        if (s_sensorPool[i].inUse) {
            Sensor_Destroy(&s_sensorPool[i]);
        }
    }
    s_subsystemInitialized = false;
}

SensorStatus_t Sensor_Create(SensorType_t type,
                              uint8_t busId,
                              uint8_t address,
                              const SensorConfig_t *config,
                              SensorHandle_t **pHandle)
{
    if (!s_subsystemInitialized) {
        return SENSOR_ERROR_NOT_INITIALIZED;
    }
    
    if (pHandle == NULL) {
        return SENSOR_ERROR;
    }
    
    SensorHandle_t *handle = Sensor_FindFreeInstance();
    if (handle == NULL) {
        return SENSOR_ERROR;
    }
    
    memset(handle, 0, sizeof(SensorHandle_t));
    handle->inUse = true;
    handle->state = SENSOR_STATE_UNINITIALIZED;
    handle->info.busId = busId;
    handle->info.address = address;
    
    if (config != NULL) {
        handle->config = *config;
    } else {
        handle->config.sampleRateHz = SENSOR_DEFAULT_SAMPLE_RATE;
        handle->config.enableFiltering = true;
    }
    
    handle->calibration.isValid = false;
    handle->calibration.scale = 1.0f;
    
    switch (type) {
        case SENSOR_TYPE_TEMPERATURE:
            handle->vtable = &s_tempVTable;
            break;
        case SENSOR_TYPE_HUMIDITY:
            handle->vtable = &s_humidityVTable;
            break;
        default:
            handle->inUse = false;
            return SENSOR_ERROR_CONFIG;
    }
    
    *pHandle = handle;
    return SENSOR_OK;
}

void Sensor_Destroy(SensorHandle_t *handle)
{
    if (handle != NULL && handle->inUse) {
        if (handle->vtable != NULL && handle->vtable->deinit != NULL) {
            handle->vtable->deinit(handle);
        }
        handle->inUse = false;
    }
}

SensorStatus_t Sensor_Initialize(SensorHandle_t *handle)
{
    if (handle == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    if (handle->vtable == NULL || handle->vtable->init == NULL) {
        return SENSOR_ERROR_CONFIG;
    }
    
    handle->state = SENSOR_STATE_INITIALIZING;
    SensorStatus_t status = handle->vtable->init(handle);
    
    if (status != SENSOR_OK) {
        handle->state = SENSOR_STATE_ERROR;
    }
    
    return status;
}

SensorStatus_t Sensor_Deinitialize(SensorHandle_t *handle)
{
    if (handle == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    if (handle->vtable == NULL || handle->vtable->deinit == NULL) {
        return SENSOR_ERROR_CONFIG;
    }
    
    return handle->vtable->deinit(handle);
}

SensorStatus_t Sensor_Read(SensorHandle_t *handle, float *pValue)
{
    if (handle == NULL || pValue == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    if (handle->state != SENSOR_STATE_READY) {
        return SENSOR_ERROR_NOT_INITIALIZED;
    }
    
    if (handle->vtable == NULL || handle->vtable->read == NULL) {
        return SENSOR_ERROR_CONFIG;
    }
    
    SensorStatus_t status = handle->vtable->read(handle, pValue);
    
    if (status == SENSOR_OK) {
        uint32_t timestamp = 0;  /* Would get from system timer */
        Sensor_UpdateStats(handle, *pValue, timestamp);
        
        if (handle->dataCallback != NULL) {
            handle->dataCallback(handle, *pValue, timestamp, handle->callbackUserData);
        }
    } else {
        handle->stats.errorCount++;
        
        if (handle->errorCallback != NULL) {
            handle->errorCallback(handle, status, handle->callbackUserData);
        }
    }
    
    return status;
}

SensorStatus_t Sensor_ReadVector(SensorHandle_t *handle, Vector3D_t *pVector)
{
    if (handle == NULL || pVector == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    if (handle->vtable == NULL || handle->vtable->readVector == NULL) {
        return SENSOR_ERROR_CONFIG;
    }
    
    return handle->vtable->readVector(handle, pVector);
}

SensorStatus_t Sensor_ReadRaw(SensorHandle_t *handle, int32_t *pRawValue)
{
    if (handle == NULL || pRawValue == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    if (handle->vtable == NULL || handle->vtable->readRaw == NULL) {
        return SENSOR_ERROR_CONFIG;
    }
    
    return handle->vtable->readRaw(handle, pRawValue);
}

SensorStatus_t Sensor_TriggerMeasurement(SensorHandle_t *handle)
{
    if (handle == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    if (handle->vtable == NULL || handle->vtable->trigger == NULL) {
        return SENSOR_ERROR_CONFIG;
    }
    
    return handle->vtable->trigger(handle);
}

SensorStatus_t Sensor_IsDataReady(SensorHandle_t *handle, bool *pReady)
{
    if (handle == NULL || pReady == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    if (handle->vtable == NULL || handle->vtable->isDataReady == NULL) {
        return SENSOR_ERROR_CONFIG;
    }
    
    return handle->vtable->isDataReady(handle, pReady);
}

SensorStatus_t Sensor_Sleep(SensorHandle_t *handle)
{
    if (handle == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    if (handle->vtable == NULL || handle->vtable->sleep == NULL) {
        return SENSOR_ERROR_CONFIG;
    }
    
    return handle->vtable->sleep(handle);
}

SensorStatus_t Sensor_Wake(SensorHandle_t *handle)
{
    if (handle == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    if (handle->vtable == NULL || handle->vtable->wake == NULL) {
        return SENSOR_ERROR_CONFIG;
    }
    
    return handle->vtable->wake(handle);
}

SensorStatus_t Sensor_SelfTest(SensorHandle_t *handle, bool *pResult)
{
    if (handle == NULL || pResult == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    if (handle->vtable == NULL || handle->vtable->selfTest == NULL) {
        return SENSOR_ERROR_CONFIG;
    }
    
    return handle->vtable->selfTest(handle, pResult);
}

SensorStatus_t Sensor_Reset(SensorHandle_t *handle)
{
    if (handle == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    if (handle->vtable == NULL || handle->vtable->reset == NULL) {
        return SENSOR_ERROR_CONFIG;
    }
    
    return handle->vtable->reset(handle);
}

SensorStatus_t Sensor_GetInfo(SensorHandle_t *handle, SensorInfo_t *pInfo)
{
    if (handle == NULL || pInfo == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    *pInfo = handle->info;
    return SENSOR_OK;
}

SensorStatus_t Sensor_GetStats(SensorHandle_t *handle, SensorStats_t *pStats)
{
    if (handle == NULL || pStats == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    *pStats = handle->stats;
    return SENSOR_OK;
}

SensorStatus_t Sensor_ClearStats(SensorHandle_t *handle)
{
    if (handle == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    memset(&handle->stats, 0, sizeof(SensorStats_t));
    return SENSOR_OK;
}

SensorStatus_t Sensor_GetState(SensorHandle_t *handle, SensorState_t *pState)
{
    if (handle == NULL || pState == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    *pState = handle->state;
    return SENSOR_OK;
}

SensorStatus_t Sensor_RegisterDataCallback(SensorHandle_t *handle,
                                            SensorDataCallback_t callback,
                                            void *userData)
{
    if (handle == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    handle->dataCallback = callback;
    handle->callbackUserData = userData;
    return SENSOR_OK;
}

SensorStatus_t Sensor_RegisterErrorCallback(SensorHandle_t *handle,
                                             SensorErrorCallback_t callback,
                                             void *userData)
{
    if (handle == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    handle->errorCallback = callback;
    return SENSOR_OK;
}

SensorStatus_t Sensor_ApplyCalibration(SensorHandle_t *handle,
                                        const SensorCalibration_t *calData)
{
    if (handle == NULL || calData == NULL || !handle->inUse) {
        return SENSOR_ERROR;
    }
    
    handle->calibration = *calData;
    return SENSOR_OK;
}

SensorStatus_t Sensor_CalibrateZero(SensorHandle_t *handle, uint8_t numSamples)
{
    if (handle == NULL || !handle->inUse || numSamples == 0) {
        return SENSOR_ERROR;
    }
    
    float sum = 0.0f;
    float value;
    
    for (uint8_t i = 0; i < numSamples; i++) {
        if (Sensor_Read(handle, &value) != SENSOR_OK) {
            return SENSOR_ERROR;
        }
        sum += value;
    }
    
    handle->calibration.offset = sum / (float)numSamples;
    handle->calibration.isValid = true;
    
    return SENSOR_OK;
}
