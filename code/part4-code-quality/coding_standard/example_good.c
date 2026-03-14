/**
 * @file example_good.c
 * @brief Examples of Coding Standard Compliant Embedded C Code
 * 
 * This file demonstrates best practices for embedded C programming
 * that should be followed in production code. Each example includes
 * comments explaining WHY the pattern is recommended.
 * 
 * Key Principles Demonstrated:
 * - Consistent naming conventions
 * - Defensive programming
 * - Resource management
 * - Error handling
 * - Documentation
 * - Type safety
 * - Memory efficiency
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

/*============================================================================*/
/*                          INCLUDE GUARDS & INCLUDES                          */
/*============================================================================*/

/* Standard includes first (system headers) */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* Project includes second (use double quotes) */
#include "error_codes.h"
#include "app_assert.h"

/*============================================================================*/
/*                          FILE-SPECIFIC CONFIGURATION                        */
/*============================================================================*/

/**
 * @brief Maximum number of sensors supported
 * 
 * Configuration constants should be defined at the top of the file
 * with clear comments explaining their purpose and valid ranges.
 */
#define SENSOR_MAX_COUNT            (8U)

/**
 * @brief Sensor read timeout in milliseconds
 */
#define SENSOR_READ_TIMEOUT_MS      (1000U)

/**
 * @brief Minimum valid temperature reading (Celsius * 100)
 */
#define SENSOR_TEMP_MIN_CENTIGRADE  (-4000)  /* -40.00 C */

/**
 * @brief Maximum valid temperature reading (Celsius * 100)
 */
#define SENSOR_TEMP_MAX_CENTIGRADE  (8500)   /* 85.00 C */

/*============================================================================*/
/*                          TYPE DEFINITIONS                                    */
/*============================================================================*/

/**
 * @brief Sensor state enumeration
 * 
 * Each state has a clear, distinct meaning. Using an enum provides
 * type safety and enables compiler checking of switch statements.
 */
typedef enum {
    SENSOR_STATE_UNINITIALIZED  = 0,    /**< Initial state, not ready */
    SENSOR_STATE_READY          = 1,    /**< Initialized and ready to use */
    SENSOR_STATE_BUSY           = 2,    /**< Operation in progress */
    SENSOR_STATE_ERROR          = 3,    /**< Error state, needs recovery */
    SENSOR_STATE_COUNT                  /**< Count of states (for validation) */
} SensorState_t;

/* Compile-time validation of enum values */
APP_STATIC_ASSERT(SENSOR_STATE_COUNT <= 16, sensor_state_fits_in_nibble);

/**
 * @brief Sensor type enumeration
 */
typedef enum {
    SENSOR_TYPE_TEMPERATURE     = 0x01,
    SENSOR_TYPE_HUMIDITY        = 0x02,
    SENSOR_TYPE_PRESSURE        = 0x03,
    SENSOR_TYPE_ACCELEROMETER   = 0x04,
} SensorType_t;

/**
 * @brief Sensor configuration structure
 * 
 * Structures should be organized for:
 * 1. Natural alignment (largest types first)
 * 2. Logical grouping of related fields
 * 3. Clear documentation of each field
 */
typedef struct {
    /* Configuration fields (sorted by size, descending) */
    uint32_t            sample_interval_ms;     /**< Time between samples */
    uint32_t            filter_coefficient;     /**< Filter coefficient (0-1000) */
    
    SensorType_t        type;                   /**< Sensor type identifier */
    uint8_t             i2c_address;            /**< I2C bus address (7-bit) */
    uint8_t             gpio_pin;               /**< GPIO pin for interrupt */
    
    /* Status fields */
    SensorState_t       state;                  /**< Current sensor state */
    bool                enabled;                /**< Sensor enabled flag */
    bool                calibration_valid;      /**< Calibration data is valid */
} SensorConfig_t;

/* Verify structure size for memory planning */
APP_STATIC_ASSERT(sizeof(SensorConfig_t) <= 24, sensor_config_size);

/**
 * @brief Sensor reading structure
 */
typedef struct {
    int32_t             temperature_centi;      /**< Temperature (C * 100) */
    uint32_t            humidity_millipercent;  /**< Humidity (0.1% units) */
    uint32_t            timestamp_ms;           /**< Reading timestamp */
    ErrorCode_t         error;                  /**< Reading error code */
} SensorReading_t;

/**
 * @brief Sensor driver function pointer types
 * 
 * Using function pointers enables dependency injection and easier testing.
 */
typedef ErrorCode_t (*SensorInitFunc_t)(SensorConfig_t *config);
typedef ErrorCode_t (*SensorReadFunc_t)(SensorConfig_t *config, SensorReading_t *reading);
typedef ErrorCode_t (*SensorDeinitFunc_t)(SensorConfig_t *config);

/*============================================================================*/
/*                          PRIVATE DATA                                        */
/*============================================================================*/

/**
 * @brief Sensor instance data
 * 
 * File-scope data should be declared static to limit scope.
 * Initialize to safe defaults where possible.
 */
static SensorConfig_t s_sensors[SENSOR_MAX_COUNT] = {0};

/**
 * @brief Number of registered sensors
 */
static uint8_t s_sensor_count = 0U;

/**
 * @brief Module initialization flag
 */
static bool s_module_initialized = false;

/*============================================================================*/
/*                          PRIVATE FUNCTION DECLARATIONS                      */
/*============================================================================*/

/* Forward declarations for internal functions */
static ErrorCode_t validate_sensor_index(uint8_t index);
static ErrorCode_t validate_reading(const SensorReading_t *reading);
static bool is_temperature_valid(int32_t temp_centi);

/*============================================================================*/
/*                          PUBLIC FUNCTION IMPLEMENTATIONS                    */
/*============================================================================*/

/**
 * @brief Initialize the sensor module
 * 
 * Must be called before any other sensor functions.
 * Safe to call multiple times.
 * 
 * @return ErrorCode_t ERROR_NONE on success, error code on failure
 */
ErrorCode_t Sensor_ModuleInit(void)
{
    /* Check if already initialized */
    if (s_module_initialized) {
        return ERROR_NONE;  /* Idempotent initialization */
    }
    
    /* Clear all sensor configurations */
    memset(s_sensors, 0, sizeof(s_sensors));
    s_sensor_count = 0U;
    
    s_module_initialized = true;
    
    return ERROR_NONE;
}

/**
 * @brief Deinitialize the sensor module
 * 
 * Releases all resources and returns to uninitialized state.
 * 
 * @return ErrorCode_t ERROR_NONE on success
 */
ErrorCode_t Sensor_ModuleDeinit(void)
{
    /* Deinitialize all registered sensors */
    for (uint8_t i = 0U; i < s_sensor_count; i++) {
        if (s_sensors[i].state != SENSOR_STATE_UNINITIALIZED) {
            /* Attempt graceful shutdown */
            s_sensors[i].state = SENSOR_STATE_UNINITIALIZED;
            s_sensors[i].enabled = false;
        }
    }
    
    s_sensor_count = 0U;
    s_module_initialized = false;
    
    return ERROR_NONE;
}

/**
 * @brief Register a new sensor
 * 
 * Adds a sensor to the system and returns its index.
 * 
 * @param config Sensor configuration (must not be NULL)
 * @param[out] index Pointer to receive sensor index
 * @return ErrorCode_t ERROR_NONE on success
 */
ErrorCode_t Sensor_Register(const SensorConfig_t *config, uint8_t *index)
{
    /* Validate input parameters */
    APP_ASSERT_NOT_NULL(config);
    APP_ASSERT_NOT_NULL(index);
    
    /* Check module state */
    if (!s_module_initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    /* Check for available slots */
    if (s_sensor_count >= SENSOR_MAX_COUNT) {
        return ERROR_OUT_OF_MEMORY;
    }
    
    /* Validate configuration */
    if ((config->i2c_address == 0U) || (config->i2c_address > 127U)) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Store configuration */
    uint8_t new_index = s_sensor_count;
    s_sensors[new_index] = *config;
    s_sensors[new_index].state = SENSOR_STATE_UNINITIALIZED;
    
    *index = new_index;
    s_sensor_count++;
    
    return ERROR_NONE;
}

/**
 * @brief Initialize a specific sensor
 * 
 * @param index Sensor index from registration
 * @return ErrorCode_t ERROR_NONE on success
 */
ErrorCode_t Sensor_Init(uint8_t index)
{
    ErrorCode_t err;
    
    /* Validate index */
    err = validate_sensor_index(index);
    if (err != ERROR_NONE) {
        return err;
    }
    
    /* Check current state */
    SensorConfig_t *sensor = &s_sensors[index];
    if (sensor->state == SENSOR_STATE_READY) {
        return ERROR_NONE;  /* Already initialized */
    }
    
    /* Perform hardware initialization */
    /* In a real implementation, this would configure GPIOs, I2C, etc. */
    
    /* For this example, we simulate the initialization */
    sensor->state = SENSOR_STATE_READY;
    sensor->enabled = true;
    
    return ERROR_NONE;
}

/**
 * @brief Read sensor data
 * 
 * Performs a synchronous read of the sensor.
 * 
 * @param index Sensor index
 * @param[out] reading Pointer to receive sensor reading
 * @return ErrorCode_t ERROR_NONE on success
 */
ErrorCode_t Sensor_Read(uint8_t index, SensorReading_t *reading)
{
    ErrorCode_t err;
    
    /* Validate parameters */
    err = validate_sensor_index(index);
    if (err != ERROR_NONE) {
        return err;
    }
    
    APP_ASSERT_NOT_NULL(reading);
    
    /* Initialize output structure to safe defaults */
    memset(reading, 0, sizeof(*reading));
    
    /* Get sensor configuration */
    const SensorConfig_t *sensor = &s_sensors[index];
    
    /* Check sensor state */
    if (sensor->state != SENSOR_STATE_READY) {
        reading->error = ERROR_INVALID_STATE;
        return ERROR_INVALID_STATE;
    }
    
    if (!sensor->enabled) {
        reading->error = ERROR_INVALID_STATE;
        return ERROR_INVALID_STATE;
    }
    
    /* 
     * Perform actual sensor read here.
     * This example simulates a read with fixed values.
     */
    reading->temperature_centi = 2500;   /* 25.00 C */
    reading->humidity_millipercent = 5000;  /* 50.0% */
    reading->timestamp_ms = 0;  /* Would be filled with actual timestamp */
    reading->error = ERROR_NONE;
    
    /* Validate reading */
    err = validate_reading(reading);
    if (err != ERROR_NONE) {
        reading->error = err;
        return err;
    }
    
    return ERROR_NONE;
}

/**
 * @brief Set sensor sample interval
 * 
 * @param index Sensor index
 * @param interval_ms New interval in milliseconds
 * @return ErrorCode_t ERROR_NONE on success
 */
ErrorCode_t Sensor_SetSampleInterval(uint8_t index, uint32_t interval_ms)
{
    ErrorCode_t err;
    
    /* Validate index */
    err = validate_sensor_index(index);
    if (err != ERROR_NONE) {
        return err;
    }
    
    /* Validate interval (1 second to 1 hour) */
    if ((interval_ms < 1000U) || (interval_ms > 3600000U)) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Update configuration */
    s_sensors[index].sample_interval_ms = interval_ms;
    
    return ERROR_NONE;
}

/**
 * @brief Get sensor state
 * 
 * @param index Sensor index
 * @param[out] state Pointer to receive state
 * @return ErrorCode_t ERROR_NONE on success
 */
ErrorCode_t Sensor_GetState(uint8_t index, SensorState_t *state)
{
    ErrorCode_t err;
    
    /* Validate parameters */
    err = validate_sensor_index(index);
    if (err != ERROR_NONE) {
        return err;
    }
    
    APP_ASSERT_NOT_NULL(state);
    
    *state = s_sensors[index].state;
    
    return ERROR_NONE;
}

/*============================================================================*/
/*                          PRIVATE FUNCTION IMPLEMENTATIONS                   */
/*============================================================================*/

/**
 * @brief Validate sensor index
 * 
 * Internal helper to check if index is valid.
 * 
 * @param index Index to validate
 * @return ErrorCode_t ERROR_NONE if valid
 */
static ErrorCode_t validate_sensor_index(uint8_t index)
{
    if (!s_module_initialized) {
        return ERROR_NOT_INITIALIZED;
    }
    
    if (index >= s_sensor_count) {
        return ERROR_INVALID_PARAMETER;
    }
    
    return ERROR_NONE;
}

/**
 * @brief Validate sensor reading
 * 
 * Checks that reading values are within expected ranges.
 * 
 * @param reading Reading to validate
 * @return ErrorCode_t ERROR_NONE if valid
 */
static ErrorCode_t validate_reading(const SensorReading_t *reading)
{
    APP_ASSERT_NOT_NULL(reading);
    
    /* Validate temperature range */
    if (!is_temperature_valid(reading->temperature_centi)) {
        return ERROR_HARDWARE_FAULT;
    }
    
    /* Validate humidity range (0-100% in 0.1% units = 0-1000) */
    if (reading->humidity_millipercent > 1000U) {
        return ERROR_HARDWARE_FAULT;
    }
    
    return ERROR_NONE;
}

/**
 * @brief Check if temperature is within valid range
 * 
 * @param temp_centi Temperature in centigrade (C * 100)
 * @return true if valid, false otherwise
 */
static bool is_temperature_valid(int32_t temp_centi)
{
    return ((temp_centi >= SENSOR_TEMP_MIN_CENTIGRADE) &&
            (temp_centi <= SENSOR_TEMP_MAX_CENTIGRADE));
}

/*============================================================================*/
/*                          USAGE EXAMPLE                                       */
/*============================================================================*/

/*
 * Example of how to use this module:
 * 
 * void main(void)
 * {
 *     ErrorCode_t err;
 *     uint8_t sensor_idx;
 *     SensorReading_t reading;
 *     
 *     SensorConfig_t config = {
 *         .sample_interval_ms = 1000,
 *         .filter_coefficient = 500,
 *         .type = SENSOR_TYPE_TEMPERATURE,
 *         .i2c_address = 0x48,
 *         .gpio_pin = 5,
 *     };
 *     
 *     err = Sensor_ModuleInit();
 *     APP_ASSERT_ERROR(err == ERROR_NONE, err);
 *     
 *     err = Sensor_Register(&config, &sensor_idx);
 *     APP_ASSERT_ERROR(err == ERROR_NONE, err);
 *     
 *     err = Sensor_Init(sensor_idx);
 *     APP_ASSERT_ERROR(err == ERROR_NONE, err);
 *     
 *     err = Sensor_Read(sensor_idx, &reading);
 *     if (err == ERROR_NONE) {
 *         printf("Temperature: %.2f C\n", reading.temperature_centi / 100.0f);
 *     }
 * }
 */
