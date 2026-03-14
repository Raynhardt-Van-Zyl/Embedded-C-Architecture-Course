/**
 * @file driver_interface.h
 * @brief Generic Driver Interface - Abstract driver abstraction layer
 * 
 * @details This header defines a generic driver interface that provides
 * a consistent API for all types of device drivers. It follows the
 * interface segregation and dependency inversion principles.
 * 
 * Architecture Overview:
 * ┌─────────────────────────────────────────────────────────────┐
 * │                    APPLICATION LAYER                        │
 * │  ┌─────────────────────────────────────────────────────┐   │
 * │  │              Driver Interface (This File)            │   │
 * │  │  - Generic Operations (Init, Read, Write, Control)   │   │
 * │  │  - Driver Registry for Discovery                     │   │
 * │  │  - Event Notification System                         │   │
 * │  └─────────────────────────────────────────────────────┘   │
 * │                           ↓                                 │
 * │  ┌─────────────────────────────────────────────────────┐   │
 * │  │              Concrete Drivers                        │   │
 * │  │  UART │ SPI │ I2C │ GPIO │ ADC │ PWM │ Timer │ DMA  │   │
 * │  └─────────────────────────────────────────────────────┘   │
 * │                           ↓                                 │
 * │  ┌─────────────────────────────────────────────────────┐   │
 * │  │              Hardware Abstraction Layer              │   │
 * │  └─────────────────────────────────────────────────────┘   │
 * └─────────────────────────────────────────────────────────────┘
 * 
 * Key Design Patterns:
 * - Strategy Pattern: Different drivers implement the same interface
 * - Factory Pattern: Driver creation through registry
 * - Observer Pattern: Event notification through callbacks
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#ifndef DRIVER_INTERFACE_H
#define DRIVER_INTERFACE_H

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

/** @brief Maximum driver name length */
#define DRIVER_MAX_NAME_LEN         (32U)

/** @brief Maximum number of registered drivers */
#define DRIVER_MAX_REGISTRY         (32U)

/** @brief Maximum event data size */
#define DRIVER_EVENT_DATA_SIZE      (64U)

/** @brief Driver version encoding macro */
#define DRIVER_VERSION(major, minor, patch) \
    (((major) << 16) | ((minor) << 8) | (patch))

/*============================================================================*/
/*                              ENUMERATIONS                                  */
/*============================================================================*/

/**
 * @brief Driver category/types
 */
typedef enum {
    DRIVER_TYPE_NONE = 0,           /**< No/unknown driver type */
    DRIVER_TYPE_UART,               /**< UART/Serial driver */
    DRIVER_TYPE_SPI,                /**< SPI driver */
    DRIVER_TYPE_I2C,                /**< I2C driver */
    DRIVER_TYPE_GPIO,               /**< GPIO driver */
    DRIVER_TYPE_ADC,                /**< ADC driver */
    DRIVER_TYPE_DAC,                /**< DAC driver */
    DRIVER_TYPE_PWM,                /**< PWM driver */
    DRIVER_TYPE_TIMER,              /**< Timer driver */
    DRIVER_TYPE_RTC,                /**< Real-time clock driver */
    DRIVER_TYPE_WATCHDOG,           /**< Watchdog driver */
    DRIVER_TYPE_DMA,                /**< DMA driver */
    DRIVER_TYPE_CAN,                /**< CAN bus driver */
    DRIVER_TYPE_USB,                /**< USB driver */
    DRIVER_TYPE_ETHERNET,           /**< Ethernet driver */
    DRIVER_TYPE_FLASH,              /**< Flash memory driver */
    DRIVER_TYPE_EEPROM,             /**< EEPROM driver */
    DRIVER_TYPE_SDIO,               /**< SD/SDIO driver */
    DRIVER_TYPE_DISPLAY,            /**< Display driver */
    DRIVER_TYPE_TOUCH,              /**< Touch sensor driver */
    DRIVER_TYPE_SENSOR,             /**< Generic sensor driver */
    DRIVER_TYPE_ACTUATOR,           /**< Actuator driver */
    DRIVER_TYPE_COUNT               /**< Number of driver types */
} DriverType_e;

/**
 * @brief Driver status/error codes
 */
typedef enum {
    DRIVER_OK = 0,                  /**< Operation successful */
    DRIVER_ERROR = -1,              /**< Generic error */
    DRIVER_ERR_INVALID_PARAM = -2,  /**< Invalid parameter */
    DRIVER_ERR_NOT_INITIALIZED = -3,/**< Driver not initialized */
    DRIVER_ERR_ALREADY_INIT = -4,   /**< Already initialized */
    DRIVER_ERR_NOT_SUPPORTED = -5,  /**< Operation not supported */
    DRIVER_ERR_BUSY = -6,           /**< Driver busy */
    DRIVER_ERR_TIMEOUT = -7,        /**< Operation timeout */
    DRIVER_ERR_NO_MEMORY = -8,      /**< Memory allocation failed */
    DRIVER_ERR_HARDWARE = -9,       /**< Hardware error */
    DRIVER_ERR_BUFFER_OVERFLOW = -10,/**< Buffer overflow */
    DRIVER_ERR_UNDERFLOW = -11,     /**< Buffer underflow */
    DRIVER_ERR_NOT_FOUND = -12,     /**< Driver/device not found */
    DRIVER_ERR_PERMISSION = -13     /**< Permission denied */
} DriverStatus_e;

/**
 * @brief Driver state enumeration
 */
typedef enum {
    DRIVER_STATE_UNINITIALIZED = 0, /**< Not initialized */
    DRIVER_STATE_READY,             /**< Ready for operations */
    DRIVER_STATE_BUSY,              /**< Operation in progress */
    DRIVER_STATE_ERROR,             /**< Error state */
    DRIVER_STATE_SUSPENDED,         /**< Suspended/low power */
    DRIVER_STATE_SHUTDOWN           /**< Shutdown state */
} DriverState_e;

/**
 * @brief Driver event types
 */
typedef enum {
    DRIVER_EVENT_NONE = 0,          /**< No event */
    DRIVER_EVENT_DATA_READY,        /**< Data available for read */
    DRIVER_EVENT_TX_COMPLETE,       /**< Transmission complete */
    DRIVER_EVENT_RX_COMPLETE,       /**< Reception complete */
    DRIVER_EVENT_ERROR,             /**< Error occurred */
    DRIVER_EVENT_THRESHOLD,         /**< Threshold crossed (ADC) */
    DRIVER_EVENT_TIMEOUT,           /**< Timeout occurred */
    DRIVER_EVENT_STATE_CHANGE,      /**< State changed */
    DRIVER_EVENT_CUSTOM             /**< Custom event */
} DriverEventType_e;

/**
 * @brief Driver capability flags
 */
typedef enum {
    DRIVER_CAP_NONE             = 0x00000000U,
    DRIVER_CAP_READ             = 0x00000001U,
    DRIVER_CAP_WRITE            = 0x00000002U,
    DRIVER_CAP_READ_WRITE       = 0x00000003U,
    DRIVER_CAP_ASYNC            = 0x00000004U,
    DRIVER_CAP_DMA              = 0x00000008U,
    DRIVER_CAP_INTERRUPT        = 0x00000010U,
    DRIVER_CAP_POLLING          = 0x00000020U,
    DRIVER_CAP_BLOCKING         = 0x00000040U,
    DRIVER_CAP_NON_BLOCKING     = 0x00000080U,
    DRIVER_CAP_POWER_MGMT       = 0x00000100U,
    DRIVER_CAP_AUTO_BAUD        = 0x00000200U,
    DRIVER_CAP_HW_FLOW_CTRL     = 0x00000400U,
    DRIVER_CAP_SLAVE            = 0x00000800U,
    DRIVER_CAP_MASTER           = 0x00001000U,
    DRIVER_CAP_MULTI_CHANNEL    = 0x00002000U,
    DRIVER_CAP_CALLBACK         = 0x00004000U,
    DRIVER_CAP_CIRCULAR         = 0x00008000U
} DriverCapability_e;

/**
 * @brief Driver power states
 */
typedef enum {
    DRIVER_POWER_FULL = 0,         /**< Full power, all features available */
    DRIVER_POWER_LOW,              /**< Low power mode */
    DRIVER_POWER_SLEEP,            /**< Sleep mode, minimal power */
    DRIVER_POWER_OFF               /**< Power off, requires reinit */
} DriverPowerState_e;

/*============================================================================*/
/*                              STRUCTURES                                    */
/*============================================================================*/

/**
 * @brief Driver version structure
 */
typedef struct {
    uint8_t major;                  /**< Major version */
    uint8_t minor;                  /**< Minor version */
    uint8_t patch;                  /**< Patch version */
    uint8_t reserved;               /**< Reserved for alignment */
} DriverVersion_t;

/**
 * @brief Driver information structure
 */
typedef struct {
    char            name[DRIVER_MAX_NAME_LEN];  /**< Driver name */
    DriverType_e    type;                       /**< Driver type */
    DriverVersion_t version;                    /**< Driver version */
    uint32_t        capabilities;               /**< Capability flags */
    uint32_t        instance;                   /**< Instance number */
    DriverState_e   state;                      /**< Current state */
} DriverInfo_t;

/**
 * @brief Driver configuration (base structure)
 */
typedef struct {
    uint32_t        baudRate;       /**< Baud rate (serial) */
    uint32_t        clockSpeed;     /**< Clock speed (SPI, I2C) */
    uint8_t         dataBits;       /**< Data bits */
    uint8_t         stopBits;       /**< Stop bits */
    uint8_t         parity;         /**< Parity setting */
    uint8_t         mode;           /**< Operating mode */
    bool            enableInterrupt;/**< Enable interrupts */
    bool            enableDma;      /**< Enable DMA */
    void            *customConfig;  /**< Custom configuration */
} DriverConfig_t;

/**
 * @brief Driver event structure
 */
typedef struct {
    DriverEventType_e   type;       /**< Event type */
    DriverStatus_e      status;     /**< Associated status */
    uint32_t            timestamp;  /**< Event timestamp */
    uint16_t            dataLength; /**< Data length */
    uint8_t             data[DRIVER_EVENT_DATA_SIZE]; /**< Event data */
    void               *context;    /**< User context */
} DriverEvent_t;

/**
 * @brief Driver statistics structure
 */
typedef struct {
    uint32_t    readCount;          /**< Total read operations */
    uint32_t    writeCount;         /**< Total write operations */
    uint32_t    errorCount;         /**< Total errors */
    uint32_t    bytesTransferred;   /**< Total bytes transferred */
    uint32_t    lastErrorTime;      /**< Last error timestamp */
    DriverStatus_e lastError;       /**< Last error code */
} DriverStats_t;

/*============================================================================*/
/*                              FORWARD DECLARATIONS                          */
/*============================================================================*/

/**
 * @brief Opaque driver handle type
 */
typedef struct Driver Driver_t;

/*============================================================================*/
/*                              CALLBACK TYPES                                */
/*============================================================================*/

/**
 * @brief Driver event callback function type
 * 
 * @param driver    Driver instance that generated the event
 * @param event     Event data
 * @param userData  User-provided context
 */
typedef void (*DriverEventCallback_t)(Driver_t *driver, 
                                       const DriverEvent_t *event,
                                       void *userData);

/*============================================================================*/
/*                              DRIVER OPERATIONS                             */
/*============================================================================*/

/**
 * @brief Driver operation function pointers (virtual function table)
 * 
 * @details This structure defines all possible driver operations.
 * Drivers implement only the operations they support.
 */
typedef struct {
    DriverStatus_e (*init)(Driver_t *driver, const DriverConfig_t *config);
    DriverStatus_e (*deinit)(Driver_t *driver);
    DriverStatus_e (*open)(Driver_t *driver);
    DriverStatus_e (*close)(Driver_t *driver);
    
    int32_t (*read)(Driver_t *driver, void *buffer, uint32_t size);
    int32_t (*write)(Driver_t *driver, const void *data, uint32_t size);
    
    int32_t (*readAsync)(Driver_t *driver, void *buffer, uint32_t size);
    int32_t (*writeAsync)(Driver_t *driver, const void *data, uint32_t size);
    
    DriverStatus_e (*ioctl)(Driver_t *driver, uint32_t cmd, void *arg);
    
    DriverStatus_e (*suspend)(Driver_t *driver);
    DriverStatus_e (*resume)(Driver_t *driver);
    
    DriverStatus_e (*selfTest)(Driver_t *driver);
    
    DriverStatus_e (*getInfo)(Driver_t *driver, DriverInfo_t *info);
    DriverStatus_e (*getStats)(Driver_t *driver, DriverStats_t *stats);
    DriverStatus_e (*clearStats)(Driver_t *driver);
    
    DriverStatus_e (*registerCallback)(Driver_t *driver, 
                                        DriverEventCallback_t callback,
                                        void *userData);
} DriverOperations_t;

/*============================================================================*/
/*                              DRIVER BASE STRUCTURE                         */
/*============================================================================*/

/**
 * @brief Base driver structure
 * 
 * @details All concrete drivers embed this structure as their first member
 * to enable polymorphic behavior through the operations table.
 */
struct Driver {
    const DriverOperations_t    *ops;       /**< Operation function table */
    DriverInfo_t                info;       /**< Driver information */
    DriverConfig_t              config;     /**< Current configuration */
    DriverStats_t               stats;      /**< Runtime statistics */
    DriverEventCallback_t       callback;   /**< Event callback */
    void                        *userData;  /**< Callback user data */
    void                        *privateData;/**< Driver private data */
};

/*============================================================================*/
/*                              PUBLIC API                                    */
/*============================================================================*/

/**
 * @brief Initialize the driver subsystem
 * 
 * @details Must be called before using any driver functionality.
 * Initializes the driver registry.
 * 
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_InitSubsystem(void);

/**
 * @brief Deinitialize the driver subsystem
 */
void Driver_DeinitSubsystem(void);

/**
 * @brief Register a driver with the system
 * 
 * @param driver Driver instance to register
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_Register(Driver_t *driver);

/**
 * @brief Unregister a driver
 * 
 * @param driver Driver instance to unregister
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_Unregister(Driver_t *driver);

/**
 * @brief Find a driver by name
 * 
 * @param name Driver name to search for
 * @return Driver pointer or NULL if not found
 */
Driver_t* Driver_FindByName(const char *name);

/**
 * @brief Find a driver by type and instance
 * 
 * @param type      Driver type
 * @param instance  Driver instance number
 * @return Driver pointer or NULL if not found
 */
Driver_t* Driver_FindByType(DriverType_e type, uint32_t instance);

/**
 * @brief Initialize a driver
 * 
 * @param driver Driver to initialize
 * @param config Configuration parameters
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_Init(Driver_t *driver, const DriverConfig_t *config);

/**
 * @brief Deinitialize a driver
 * 
 * @param driver Driver to deinitialize
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_Deinit(Driver_t *driver);

/**
 * @brief Open a driver for use
 * 
 * @param driver Driver to open
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_Open(Driver_t *driver);

/**
 * @brief Close a driver
 * 
 * @param driver Driver to close
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_Close(Driver_t *driver);

/**
 * @brief Read data from a driver
 * 
 * @param driver Driver to read from
 * @param buffer Buffer to receive data
 * @param size   Maximum bytes to read
 * @return Number of bytes read, or negative error code
 */
int32_t Driver_Read(Driver_t *driver, void *buffer, uint32_t size);

/**
 * @brief Write data to a driver
 * 
 * @param driver Driver to write to
 * @param data   Data to write
 * @param size   Number of bytes to write
 * @return Number of bytes written, or negative error code
 */
int32_t Driver_Write(Driver_t *driver, const void *data, uint32_t size);

/**
 * @brief Asynchronous read from driver
 * 
 * @param driver Driver to read from
 * @param buffer Buffer to receive data
 * @param size   Maximum bytes to read
 * @return DRIVER_OK if operation started successfully
 */
int32_t Driver_ReadAsync(Driver_t *driver, void *buffer, uint32_t size);

/**
 * @brief Asynchronous write to driver
 * 
 * @param driver Driver to write to
 * @param data   Data to write
 * @param size   Number of bytes to write
 * @return DRIVER_OK if operation started successfully
 */
int32_t Driver_WriteAsync(Driver_t *driver, const void *data, uint32_t size);

/**
 * @brief Send control command to driver
 * 
 * @param driver Driver to control
 * @param cmd    Command code
 * @param arg    Command argument
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_Ioctl(Driver_t *driver, uint32_t cmd, void *arg);

/**
 * @brief Suspend driver (low power)
 * 
 * @param driver Driver to suspend
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_Suspend(Driver_t *driver);

/**
 * @brief Resume driver from suspend
 * 
 * @param driver Driver to resume
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_Resume(Driver_t *driver);

/**
 * @brief Run driver self-test
 * 
 * @param driver Driver to test
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_SelfTest(Driver_t *driver);

/**
 * @brief Get driver information
 * 
 * @param driver Driver to query
 * @param info   Output buffer for info
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_GetInfo(Driver_t *driver, DriverInfo_t *info);

/**
 * @brief Get driver statistics
 * 
 * @param driver Driver to query
 * @param stats  Output buffer for stats
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_GetStats(Driver_t *driver, DriverStats_t *stats);

/**
 * @brief Clear driver statistics
 * 
 * @param driver Driver to clear stats for
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_ClearStats(Driver_t *driver);

/**
 * @brief Register event callback
 * 
 * @param driver   Driver to register callback for
 * @param callback Callback function
 * @param userData User context
 * @return DRIVER_OK on success
 */
DriverStatus_e Driver_RegisterCallback(Driver_t *driver,
                                        DriverEventCallback_t callback,
                                        void *userData);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_INTERFACE_H */
