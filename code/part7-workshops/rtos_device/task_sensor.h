/**
 * @file task_sensor.h
 * @brief Sensor Task Interface - FreeRTOS-based sensor data acquisition
 * 
 * @details This header defines the interface for the sensor task that handles
 * all sensor data acquisition and processing in an RTOS-based embedded system.
 * The task reads sensors at configurable intervals, applies filtering and
 * calibration, and publishes data to other tasks via queues.
 * 
 * Architecture Principles:
 * - Producer-Consumer Pattern: Produces sensor data for consumers
 * - Periodic Execution: Time-triggered sensor reading
 * - Decoupled Design: Sensor task doesn't know about consumers
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#ifndef TASK_SENSOR_H
#define TASK_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*                              INCLUDES                                      */
/*============================================================================*/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*============================================================================*/
/*                              DEFINES                                       */
/*============================================================================*/

/** @brief Sensor task name for FreeRTOS */
#define SENSOR_TASK_NAME            "SensorTask"

/** @brief Sensor task stack size in words */
#define SENSOR_TASK_STACK_SIZE      (512U)

/** @brief Sensor task priority */
#define SENSOR_TASK_PRIORITY        (2U)

/** @brief Sensor data queue length */
#define SENSOR_DATA_QUEUE_LENGTH    (16U)

/** @brief Maximum number of sensors managed by this task */
#define SENSOR_MAX_COUNT            (8U)

/** @brief Default sampling interval in milliseconds */
#define SENSOR_DEFAULT_SAMPLE_MS    (1000U)

/** @brief Moving average filter size */
#define SENSOR_FILTER_SIZE          (8U)

/** @brief Sensor name maximum length */
#define SENSOR_NAME_LENGTH          (16U)

/*============================================================================*/
/*                              ENUMERATIONS                                  */
/*============================================================================*/

/**
 * @brief Sensor type enumeration
 */
typedef enum {
    SENSOR_TYPE_NONE = 0,           /**< No sensor */
    SENSOR_TYPE_TEMPERATURE,        /**< Temperature sensor */
    SENSOR_TYPE_HUMIDITY,           /**< Humidity sensor */
    SENSOR_TYPE_PRESSURE,           /**< Pressure sensor */
    SENSOR_TYPE_LIGHT,              /**< Light sensor */
    SENSOR_TYPE_ACCELEROMETER,      /**< Accelerometer */
    SENSOR_TYPE_GYROSCOPE,          /**< Gyroscope */
    SENSOR_TYPE_MAGNETOMETER,       /**< Magnetometer */
    SENSOR_TYPE_PROXIMITY,          /**< Proximity sensor */
    SENSOR_TYPE_GAS,                /**< Gas/VOC sensor */
    SENSOR_TYPE_CURRENT,            /**< Current sensor */
    SENSOR_TYPE_VOLTAGE,            /**< Voltage sensor */
    SENSOR_TYPE_COUNT               /**< Sensor type count */
} SensorType_e;

/**
 * @brief Sensor task event types
 */
typedef enum {
    SENSOR_EVENT_NONE = 0,          /**< No event */
    SENSOR_EVENT_DATA_READY,        /**< New sensor data available */
    SENSOR_EVENT_ERROR,             /**< Sensor error occurred */
    SENSOR_EVENT_THRESHOLD,         /**< Threshold crossed */
    SENSOR_EVENT_CALIBRATE,         /**< Calibration request */
    SENSOR_EVENT_CONFIG,            /**< Configuration change request */
    SENSOR_EVENT_START,             /**< Start sampling request */
    SENSOR_EVENT_STOP,              /**< Stop sampling request */
    SENSOR_EVENT_SELFTEST           /**< Self-test request */
} SensorEventType_e;

/**
 * @brief Sensor error codes
 */
typedef enum {
    SENSOR_OK = 0,                  /**< Success */
    SENSOR_ERR_UNKNOWN = -1,        /**< Unknown error */
    SENSOR_ERR_NOT_FOUND = -2,      /**< Sensor not found */
    SENSOR_ERR_TIMEOUT = -3,        /**< Communication timeout */
    SENSOR_ERR_RANGE = -4,          /**< Value out of range */
    SENSOR_ERR_CALIBRATION = -5,    /**< Calibration error */
    SENSOR_ERR_HARDWARE = -6,       /**< Hardware failure */
    SENSOR_ERR_CONFIG = -7,         /**< Configuration error */
    SENSOR_ERR_BUSY = -8            /**< Sensor busy */
} SensorError_e;

/**
 * @brief Sensor state enumeration
 */
typedef enum {
    SENSOR_STATE_UNINIT = 0,        /**< Uninitialized */
    SENSOR_STATE_READY,             /**< Ready to sample */
    SENSOR_STATE_SAMPLING,          /**< Sampling in progress */
    SENSOR_STATE_ERROR,             /**< Error state */
    SENSOR_STATE_DISABLED           /**< Disabled */
} SensorState_e;

/*============================================================================*/
/*                              STRUCTURES                                    */
/*============================================================================*/

/**
 * @brief Sensor threshold configuration
 */
typedef struct {
    float   lowThreshold;           /**< Low threshold value */
    float   highThreshold;          /**< High threshold value */
    float   hysteresis;             /**< Hysteresis band */
    bool    lowEnabled;             /**< Enable low threshold detection */
    bool    highEnabled;            /**< Enable high threshold detection */
} SensorThreshold_t;

/**
 * @brief Sensor configuration structure
 */
typedef struct {
    SensorType_e        type;           /**< Sensor type */
    uint8_t             instance;       /**< Sensor instance number */
    uint32_t            sampleIntervalMs;/**< Sampling interval */
    uint8_t             oversampling;   /**< Oversampling factor */
    bool                enableFilter;   /**< Enable moving average filter */
    bool                enableThreshold;/**< Enable threshold detection */
    SensorThreshold_t   threshold;      /**< Threshold configuration */
    float               offset;         /**< Calibration offset */
    float               scale;          /**< Calibration scale factor */
} SensorConfig_t;

/**
 * @brief Sensor data sample structure
 */
typedef struct {
    SensorType_e    type;               /**< Sensor type */
    uint8_t         instance;           /**< Instance number */
    float           value;              /**< Sensor value */
    float           rawValue;           /**< Raw (uncalibrated) value */
    float           filteredValue;      /**< Filtered value */
    uint32_t        timestamp;          /**< Sample timestamp (ms) */
    uint16_t        quality;            /**< Quality indicator (0-100%) */
    uint8_t         flags;              /**< Status flags */
} SensorSample_t;

/**
 * @brief Sensor statistics structure
 */
typedef struct {
    uint32_t    sampleCount;            /**< Total samples taken */
    uint32_t    errorCount;             /**< Total errors */
    uint32_t    thresholdCrossings;     /**< Threshold crossing count */
    float       minValue;               /**< Minimum value seen */
    float       maxValue;               /**< Maximum value seen */
    float       avgValue;               /**< Running average */
    uint32_t    lastSampleTime;         /**< Last sample timestamp */
    uint32_t    lastErrorTime;          /**< Last error timestamp */
    SensorError_e lastError;            /**< Last error code */
} SensorStats_t;

/**
 * @brief Sensor event message structure
 */
typedef struct {
    SensorEventType_e  type;            /**< Event type */
    SensorError_e      error;           /**< Error code */
    SensorType_e       sensorType;      /**< Sensor type */
    uint8_t            instance;        /**< Instance number */
    SensorSample_t     sample;          /**< Sensor sample data */
    void              *context;         /**< User context */
} SensorEvent_t;

/**
 * @brief Sensor information structure
 */
typedef struct {
    char            name[SENSOR_NAME_LENGTH]; /**< Sensor name */
    SensorType_e    type;               /**< Sensor type */
    uint8_t         instance;           /**< Instance number */
    SensorState_e   state;              /**< Current state */
    float           minValue;           /**< Minimum valid value */
    float           maxValue;           /**< Maximum valid value */
    float           resolution;         /**< Sensor resolution */
    float           accuracy;           /**< Sensor accuracy */
    bool            available;          /**< Sensor available flag */
} SensorInfo_t;

/*============================================================================*/
/*                              CALLBACK TYPES                                */
/*============================================================================*/

/**
 * @brief Sensor event callback function type
 * 
 * @param event     Sensor event data
 * @param userData  User context pointer
 */
typedef void (*SensorEventCallback_t)(const SensorEvent_t *event, void *userData);

/*============================================================================*/
/*                              PUBLIC API                                    */
/*============================================================================*/

/**
 * @brief Initialize the sensor task subsystem
 * 
 * @return true on success
 */
bool SensorTask_Init(void);

/**
 * @brief Create and start the sensor task
 * 
 * @return true on success
 */
bool SensorTask_Create(void);

/**
 * @brief Get sensor task handle
 * 
 * @return Task handle or NULL
 */
void* SensorTask_GetHandle(void);

/**
 * @brief Register a sensor with the task
 * 
 * @param config    Sensor configuration
 * @param pSensorId Output pointer for sensor ID
 * @return SENSOR_OK on success
 */
SensorError_e SensorTask_RegisterSensor(const SensorConfig_t *config, uint8_t *pSensorId);

/**
 * @brief Unregister a sensor
 * 
 * @param sensorId Sensor ID to unregister
 * @return SENSOR_OK on success
 */
SensorError_e SensorTask_UnregisterSensor(uint8_t sensorId);

/**
 * @brief Get latest sensor sample
 * 
 * @param sensorId  Sensor ID
 * @param pSample   Output pointer for sample data
 * @return SENSOR_OK on success
 */
SensorError_e SensorTask_GetSample(uint8_t sensorId, SensorSample_t *pSample);

/**
 * @brief Get sensor statistics
 * 
 * @param sensorId  Sensor ID
 * @param pStats    Output pointer for statistics
 * @return SENSOR_OK on success
 */
SensorError_e SensorTask_GetStats(uint8_t sensorId, SensorStats_t *pStats);

/**
 * @brief Get sensor information
 * 
 * @param sensorId  Sensor ID
 * @param pInfo     Output pointer for info
 * @return SENSOR_OK on success
 */
SensorError_e SensorTask_GetInfo(uint8_t sensorId, SensorInfo_t *pInfo);

/**
 * @brief Set sensor sampling interval
 * 
 * @param sensorId      Sensor ID
 * @param intervalMs    New interval in milliseconds
 * @return SENSOR_OK on success
 */
SensorError_e SensorTask_SetInterval(uint8_t sensorId, uint32_t intervalMs);

/**
 * @brief Configure sensor thresholds
 * 
 * @param sensorId  Sensor ID
 * @param threshold Threshold configuration
 * @return SENSOR_OK on success
 */
SensorError_e SensorTask_SetThreshold(uint8_t sensorId, const SensorThreshold_t *threshold);

/**
 * @brief Start sensor sampling
 * 
 * @param sensorId Sensor ID (0xFF for all sensors)
 * @return SENSOR_OK on success
 */
SensorError_e SensorTask_Start(uint8_t sensorId);

/**
 * @brief Stop sensor sampling
 * 
 * @param sensorId Sensor ID (0xFF for all sensors)
 * @return SENSOR_OK on success
 */
SensorError_e SensorTask_Stop(uint8_t sensorId);

/**
 * @brief Perform sensor calibration
 * 
 * @param sensorId Sensor ID
 * @return SENSOR_OK on success
 */
SensorError_e SensorTask_Calibrate(uint8_t sensorId);

/**
 * @brief Perform sensor self-test
 * 
 * @param sensorId Sensor ID
 * @param pResult  Output pointer for test result
 * @return SENSOR_OK on success
 */
SensorError_e SensorTask_SelfTest(uint8_t sensorId, bool *pResult);

/**
 * @brief Register event callback
 * 
 * @param callback  Callback function
 * @param userData  User context
 * @return true on success
 */
bool SensorTask_RegisterCallback(SensorEventCallback_t callback, void *userData);

/**
 * @brief Get sensor data queue handle
 * 
 * @details Use this to receive sensor data events from another task.
 * 
 * @return Queue handle or NULL
 */
void* SensorTask_GetDataQueue(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_SENSOR_H */
