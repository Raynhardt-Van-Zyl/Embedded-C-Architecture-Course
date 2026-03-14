/**
 * @file sensor_driver.h
 * @brief Sensor Driver Interface - Abstract sensor hardware abstraction layer
 * 
 * @details This header defines the interface for sensor drivers following the
 * factory pattern. It provides a consistent API for different sensor types
 * (temperature, humidity, pressure, etc.) while hiding hardware-specific details.
 * 
 * Architecture Principles:
 * - Interface Segregation: Minimal, focused API
 * - Dependency Inversion: High-level modules depend on this abstraction
 * - Open/Closed: Easy to add new sensor types without modifying existing code
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

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

/** @brief Maximum sensor name length including null terminator */
#define SENSOR_MAX_NAME_LENGTH      (32U)

/** @brief Maximum number of samples for averaging filter */
#define SENSOR_FILTER_SAMPLES       (8U)

/** @brief Sensor read timeout in milliseconds */
#define SENSOR_READ_TIMEOUT_MS      (1000U)

/** @brief Sensor initialization timeout in milliseconds */
#define SENSOR_INIT_TIMEOUT_MS      (5000U)

/*============================================================================*/
/*                              ENUMERATIONS                                  */
/*============================================================================*/

/**
 * @brief Sensor type enumeration
 * 
 * @details Defines the category of physical measurement the sensor provides.
 * Used for sensor discovery and type-safe operations.
 */
typedef enum {
    SENSOR_TYPE_UNKNOWN = 0,        /**< Unknown or uninitialized sensor */
    SENSOR_TYPE_TEMPERATURE,        /**< Temperature sensor (Celsius) */
    SENSOR_TYPE_HUMIDITY,           /**< Relative humidity sensor (%) */
    SENSOR_TYPE_PRESSURE,           /**< Barometric pressure sensor (Pa) */
    SENSOR_TYPE_LIGHT,              /**< Ambient light sensor (lux) */
    SENSOR_TYPE_ACCELEROMETER,      /**< 3-axis accelerometer (m/s²) */
    SENSOR_TYPE_GYROSCOPE,          /**< 3-axis gyroscope (deg/s) */
    SENSOR_TYPE_MAGNETOMETER,       /**< 3-axis magnetometer (µT) */
    SENSOR_TYPE_PROXIMITY,          /**< Proximity/distance sensor (mm) */
    SENSOR_TYPE_GAS,                /**< Gas/VOC sensor (ppm) */
    SENSOR_TYPE_CURRENT,            /**< Current sensor (mA) */
    SENSOR_TYPE_VOLTAGE,            /**< Voltage sensor (mV) */
    SENSOR_TYPE_COUNT               /**< Total number of sensor types */
} SensorType_t;

/**
 * @brief Sensor status/error codes
 * 
 * @details Comprehensive status codes for all sensor operations.
 * Negative values indicate errors, zero indicates success.
 */
typedef enum {
    SENSOR_OK = 0,                  /**< Operation completed successfully */
    SENSOR_ERROR = -1,              /**< Generic error */
    SENSOR_ERROR_NOT_INITIALIZED = -2,  /**< Sensor not initialized */
    SENSOR_ERROR_TIMEOUT = -3,      /**< Communication timeout */
    SENSOR_ERROR_CRC = -4,          /**< CRC/checksum validation failed */
    SENSOR_ERROR_OUT_OF_RANGE = -5, /**< Value outside valid range */
    SENSOR_ERROR_NOT_RESPONDING = -6, /**< Sensor not responding */
    SENSOR_ERROR_BUSY = -7,         /**< Sensor busy, retry later */
    SENSOR_ERROR_CONFIG = -8,       /**< Configuration error */
    SENSOR_ERROR_NO_DATA = -9,      /**< No new data available */
    SENSOR_ERROR_HARDWARE = -10     /**< Hardware fault detected */
} SensorStatus_t;

/**
 * @brief Sensor state machine states
 * 
 * @details Defines the operational states a sensor can be in.
 * Used for power management and diagnostics.
 */
typedef enum {
    SENSOR_STATE_UNINITIALIZED = 0, /**< Driver not initialized */
    SENSOR_STATE_INITIALIZING,      /**< Initialization in progress */
    SENSOR_STATE_READY,             /**< Ready to take measurements */
    SENSOR_STATE_MEASURING,         /**< Measurement in progress */
    SENSOR_STATE_SLEEP,             /**< Low-power sleep mode */
    SENSOR_STATE_ERROR,             /**< Error state, needs recovery */
    SENSOR_STATE_SHUTDOWN           /**< Sensor powered off */
} SensorState_t;

/**
 * @brief Sensor capability flags
 * 
 * @details Bitfield flags indicating optional sensor features.
 * Used for runtime capability discovery.
 */
typedef enum {
    SENSOR_CAP_NONE             = 0x0000U,  /**< No special capabilities */
    SENSOR_CAP_SLEEP_MODE       = 0x0001U,  /**< Supports low-power sleep */
    SENSOR_CAP_CONTINUOUS       = 0x0002U,  /**< Continuous measurement mode */
    SENSOR_CAP_TRIGGER          = 0x0004U,  /**< Hardware trigger input */
    SENSOR_CAP_INTERRUPT        = 0x0008U,  /**< Data-ready interrupt */
    SENSOR_CAP_SELF_TEST        = 0x0010U,  /**< Built-in self-test */
    SENSOR_CAP_CALIBRATION      = 0x0020U,  /**< Factory calibration data */
    SENSOR_CAP_TEMPCO_COMP      = 0x0040U,  /**< Temperature compensation */
    SENSOR_CAP_FILTERING        = 0x0080U,  /**< Hardware filtering */
    SENSOR_CAP_OVERRANGE        = 0x0100U,  /**< Overrange detection */
    SENSOR_CAP_FAULT_DETECT     = 0x0200U   /**< Fault detection */
} SensorCapability_t;

/*============================================================================*/
/*                              STRUCTURES                                    */
/*============================================================================*/

/**
 * @brief 3-axis vector data structure
 * 
 * @details Used for accelerometer, gyroscope, and magnetometer data.
 * Uses fixed-point representation for deterministic behavior.
 */
typedef struct {
    float x;                        /**< X-axis value */
    float y;                        /**< Y-axis value */
    float z;                        /**< Z-axis value */
} Vector3D_t;

/**
 * @brief Sensor configuration parameters
 * 
 * @details Configuration options for sensor initialization and runtime
 * behavior. Not all parameters apply to all sensor types.
 */
typedef struct {
    uint32_t sampleRateHz;          /**< Desired sample rate in Hz */
    uint8_t  oversampling;          /**< Oversampling ratio (1, 2, 4, 8, ...) */
    uint8_t  filterCutoff;          /**< Filter cutoff frequency (sensor-specific) */
    uint8_t  resolution;            /**< ADC resolution in bits (if applicable) */
    bool     enableInterrupt;       /**< Enable data-ready interrupt */
    bool     enableFiltering;       /**< Enable on-chip filtering */
    bool     enableTempComp;        /**< Enable temperature compensation */
    int8_t   userOffset;            /**< User-specified offset correction */
} SensorConfig_t;

/**
 * @brief Sensor calibration data
 * 
 * @details Factory or user calibration parameters stored in non-volatile
 * memory. Used to correct sensor readings for systematic errors.
 */
typedef struct {
    float    offset;                /**< Zero-point offset correction */
    float    scale;                 /**< Scale factor correction (multiplier) */
    float    tempCoefficient;       /**< Temperature coefficient (ppm/°C) */
    int32_t  calibrationDate;       /**< Unix timestamp of calibration */
    uint16_t calibrationCRC;        /**< CRC of calibration data */
    bool     isValid;               /**< Calibration data validity flag */
} SensorCalibration_t;

/**
 * @brief Sensor metadata structure
 * 
 * @details Static information about the sensor device, populated during
 * initialization. Used for system diagnostics and UI display.
 */
typedef struct {
    char     name[SENSOR_MAX_NAME_LENGTH];  /**< Human-readable sensor name */
    char     manufacturer[16];              /**< Manufacturer name */
    char     partNumber[16];                /**< Part number string */
    SensorType_t type;                      /**< Sensor type classification */
    uint32_t capabilities;                  /**< Capability flags bitmask */
    float    minValue;                      /**< Minimum measurable value */
    float    maxValue;                      /**< Maximum measurable value */
    float    resolution;                    /**< Smallest detectable change */
    float    accuracy;                      /**< Typical accuracy (±value) */
    uint8_t  address;                       /**< I2C/SPI address */
    uint8_t  busId;                         /**< Bus identifier */
} SensorInfo_t;

/**
 * @brief Sensor statistics structure
 * 
 * @details Runtime statistics for monitoring sensor health and performance.
 * Updated continuously during operation.
 */
typedef struct {
    uint32_t readCount;             /**< Total number of successful reads */
    uint32_t errorCount;            /**< Total number of errors */
    uint32_t timeoutCount;          /**< Total number of timeouts */
    uint32_t outOfRangeCount;       /**< Out-of-range readings */
    float    lastValue;             /**< Most recent valid reading */
    float    minValue;              /**< Minimum value since reset */
    float    maxValue;              /**< Maximum value since reset */
    float    avgValue;              /**< Running average value */
    uint32_t lastReadTime;          /**< Timestamp of last read (ms) */
} SensorStats_t;

/**
 * @brief Forward declaration of sensor handle
 * 
 * @details Opaque handle to sensor instance. Implementation details are
 * hidden from the application layer for encapsulation.
 */
typedef struct SensorHandle SensorHandle_t;

/*============================================================================*/
/*                              FUNCTION POINTER TYPES                        */
/*============================================================================*/

/**
 * @brief Sensor data callback function type
 * 
 * @details Callback invoked when new sensor data is available (asynchronous mode).
 * 
 * @param handle    Sensor handle that generated the data
 * @param value     New sensor value
 * @param timestamp Sample timestamp in milliseconds
 * @param userData  User-provided context pointer
 */
typedef void (*SensorDataCallback_t)(SensorHandle_t *handle, 
                                      float value, 
                                      uint32_t timestamp,
                                      void *userData);

/**
 * @brief Sensor error callback function type
 * 
 * @details Callback invoked when a sensor error occurs.
 * 
 * @param handle    Sensor handle that encountered the error
 * @param status    Error code indicating the type of error
 * @param userData  User-provided context pointer
 */
typedef void (*SensorErrorCallback_t)(SensorHandle_t *handle,
                                       SensorStatus_t status,
                                       void *userData);

/*============================================================================*/
/*                              PUBLIC API                                    */
/*============================================================================*/

/**
 * @brief Initialize the sensor driver subsystem
 * 
 * @details Must be called before any sensor operations. Initializes the
 * driver registry and hardware interfaces.
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_InitSubsystem(void);

/**
 * @brief Deinitialize the sensor subsystem
 * 
 * @details Releases all resources and puts all sensors in shutdown state.
 * Called during system shutdown or before reinitialization.
 */
void Sensor_DeinitSubsystem(void);

/**
 * @brief Create a new sensor instance
 * 
 * @details Factory function that creates a sensor handle based on type and
 * configuration. The actual sensor driver is selected internally.
 * 
 * @param type       Sensor type to create
 * @param busId      Communication bus identifier (I2C/SPI bus number)
 * @param address    Device address on the bus
 * @param config     Initial configuration parameters
 * @param pHandle    Output pointer to receive the sensor handle
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_Create(SensorType_t type,
                              uint8_t busId,
                              uint8_t address,
                              const SensorConfig_t *config,
                              SensorHandle_t **pHandle);

/**
 * @brief Destroy a sensor instance
 * 
 * @details Releases resources associated with the sensor handle.
 * The handle becomes invalid after this call.
 * 
 * @param handle Sensor handle to destroy
 */
void Sensor_Destroy(SensorHandle_t *handle);

/**
 * @brief Initialize the sensor hardware
 * 
 * @details Performs hardware initialization sequence including reset,
 * configuration register programming, and self-test.
 * 
 * @param handle Sensor handle to initialize
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_Initialize(SensorHandle_t *handle);

/**
 * @brief Deinitialize the sensor hardware
 * 
 * @details Puts the sensor in a low-power state and releases hardware
 * resources. The handle remains valid for re-initialization.
 * 
 * @param handle Sensor handle to deinitialize
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_Deinitialize(SensorHandle_t *handle);

/**
 * @brief Read the current sensor value
 * 
 * @details Triggers a measurement (if necessary) and reads the result.
 * For sensors with continuous measurement mode, returns the latest value.
 * 
 * @param handle   Sensor handle to read from
 * @param pValue   Output pointer for the sensor value
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_Read(SensorHandle_t *handle, float *pValue);

/**
 * @brief Read 3-axis sensor data
 * 
 * @details Specialized read for accelerometer/gyroscope/magnetometer sensors.
 * Returns values for all three axes in a single operation.
 * 
 * @param handle   Sensor handle to read from
 * @param pVector  Output pointer for the 3-axis data
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_ReadVector(SensorHandle_t *handle, Vector3D_t *pVector);

/**
 * @brief Read raw (uncalibrated) sensor value
 * 
 * @details Returns raw ADC or register value without applying calibration
 * or conversion. Used for diagnostics and custom processing.
 * 
 * @param handle     Sensor handle to read from
 * @param pRawValue  Output pointer for the raw value
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_ReadRaw(SensorHandle_t *handle, int32_t *pRawValue);

/**
 * @brief Trigger a new measurement
 * 
 * @details Initiates a measurement cycle without blocking for the result.
 * Use with Sensor_IsDataReady() for non-blocking operation.
 * 
 * @param handle Sensor handle to trigger
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_TriggerMeasurement(SensorHandle_t *handle);

/**
 * @brief Check if new data is available
 * 
 * @details Polls the data-ready status without blocking.
 * 
 * @param handle       Sensor handle to check
 * @param pReady       Output pointer for data-ready flag
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_IsDataReady(SensorHandle_t *handle, bool *pReady);

/**
 * @brief Put sensor in low-power sleep mode
 * 
 * @details Reduces power consumption by putting the sensor in sleep mode.
 * Use Sensor_Wake() to resume normal operation.
 * 
 * @param handle Sensor handle to put to sleep
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_Sleep(SensorHandle_t *handle);

/**
 * @brief Wake sensor from low-power mode
 * 
 * @details Restores normal operation after sleep mode. May require time
 * for the sensor to stabilize before accurate readings are available.
 * 
 * @param handle Sensor handle to wake
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_Wake(SensorHandle_t *handle);

/**
 * @brief Run sensor self-test
 * 
 * @details Executes built-in self-test routine if supported by the sensor.
 * Results indicate if the sensor is functioning correctly.
 * 
 * @param handle   Sensor handle to test
 * @param pResult  Output pointer for test result (true = pass)
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_SelfTest(SensorHandle_t *handle, bool *pResult);

/**
 * @brief Reset sensor to default configuration
 * 
 * @details Performs a hardware or software reset of the sensor.
 * All configuration is lost and must be reapplied.
 * 
 * @param handle Sensor handle to reset
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_Reset(SensorHandle_t *handle);

/**
 * @brief Get sensor information structure
 * 
 * @details Returns static metadata about the sensor device.
 * 
 * @param handle  Sensor handle to query
 * @param pInfo   Output pointer for sensor information
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_GetInfo(SensorHandle_t *handle, SensorInfo_t *pInfo);

/**
 * @brief Get sensor statistics
 * 
 * @details Returns runtime statistics for monitoring sensor health.
 * 
 * @param handle  Sensor handle to query
 * @param pStats  Output pointer for sensor statistics
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_GetStats(SensorHandle_t *handle, SensorStats_t *pStats);

/**
 * @brief Clear sensor statistics
 * 
 * @details Resets all statistical counters to zero.
 * 
 * @param handle Sensor handle to clear statistics for
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_ClearStats(SensorHandle_t *handle);

/**
 * @brief Get current sensor state
 * 
 * @details Returns the current operational state of the sensor.
 * 
 * @param handle  Sensor handle to query
 * @param pState  Output pointer for sensor state
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_GetState(SensorHandle_t *handle, SensorState_t *pState);

/**
 * @brief Register data callback
 * 
 * @details Registers a callback function to be invoked when new data
 * is available. Used for interrupt-driven or asynchronous operation.
 * 
 * @param handle    Sensor handle to register callback for
 * @param callback  Callback function pointer
 * @param userData  User context pointer passed to callback
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_RegisterDataCallback(SensorHandle_t *handle,
                                            SensorDataCallback_t callback,
                                            void *userData);

/**
 * @brief Register error callback
 * 
 * @details Registers a callback function to be invoked when errors occur.
 * 
 * @param handle    Sensor handle to register callback for
 * @param callback  Callback function pointer
 * @param userData  User context pointer passed to callback
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_RegisterErrorCallback(SensorHandle_t *handle,
                                             SensorErrorCallback_t callback,
                                             void *userData);

/**
 * @brief Apply calibration data
 * 
 * @details Applies calibration correction to sensor readings.
 * Calibration data is typically stored in non-volatile memory.
 * 
 * @param handle    Sensor handle to calibrate
 * @param calData   Calibration data to apply
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_ApplyCalibration(SensorHandle_t *handle,
                                        const SensorCalibration_t *calData);

/**
 * @brief Perform zero-point calibration
 * 
 * @details Performs an in-situ zero-point calibration by taking multiple
 * readings and calculating the offset. Sensor should be in a known
 * reference state during calibration.
 * 
 * @param handle     Sensor handle to calibrate
 * @param numSamples Number of samples to average for calibration
 * 
 * @return SENSOR_OK on success, error code otherwise
 */
SensorStatus_t Sensor_CalibrateZero(SensorHandle_t *handle, uint8_t numSamples);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_DRIVER_H */
