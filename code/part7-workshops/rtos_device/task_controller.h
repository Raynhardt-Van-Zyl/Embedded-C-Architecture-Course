/**
 * @file task_controller.h
 * @brief Controller Task Interface - Main application logic task
 * 
 * @details This header defines the interface for the main controller task
 * that orchestrates the entire application. It receives data from sensor
 * and network tasks, makes decisions, and coordinates system behavior.
 * 
 * Architecture Principles:
 * - Mediator Pattern: Coordinates between other tasks
 * - Command Pattern: Encapsulates actions as commands
 * - State Machine: Manages application state
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#ifndef TASK_CONTROLLER_H
#define TASK_CONTROLLER_H

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

/** @brief Controller task name */
#define CONTROLLER_TASK_NAME        "ControllerTask"

/** @brief Controller task stack size in words */
#define CONTROLLER_TASK_STACK_SIZE  (1024U)

/** @brief Controller task priority */
#define CONTROLLER_TASK_PRIORITY    (4U)

/** @brief Controller event queue length */
#define CONTROLLER_QUEUE_LENGTH     (32U)

/** @brief Maximum number of rules in the rule engine */
#define CONTROLLER_MAX_RULES        (16U)

/** @brief Maximum number of actions per rule */
#define CONTROLLER_MAX_ACTIONS      (4U)

/** @brief Command buffer size */
#define CONTROLLER_CMD_BUFFER_SIZE  (64U)

/** @brief Maximum devices in the system */
#define CONTROLLER_MAX_DEVICES      (8U)

/*============================================================================*/
/*                              ENUMERATIONS                                  */
/*============================================================================*/

/**
 * @brief System operating mode
 */
typedef enum {
    CTRL_MODE_NORMAL = 0,           /**< Normal operation */
    CTRL_MODE_MAINTENANCE,          /**< Maintenance mode */
    CTRL_MODE_EMERGENCY,            /**< Emergency mode */
    CTRL_MODE_STANDBY,              /**< Standby/low-power mode */
    CTRL_MODE_CONFIGURATION,        /**< Configuration mode */
    CTRL_MODE_COUNT                 /**< Mode count */
} ControllerMode_e;

/**
 * @brief Controller event types
 */
typedef enum {
    CTRL_EVENT_NONE = 0,            /**< No event */
    CTRL_EVENT_SENSOR_DATA,         /**< New sensor data available */
    CTRL_EVENT_NETWORK_DATA,        /**< Network data received */
    CTRL_EVENT_NETWORK_STATUS,      /**< Network status change */
    CTRL_EVENT_THRESHOLD,           /**< Threshold crossed */
    CTRL_EVENT_ALARM,               /**< Alarm condition */
    CTRL_EVENT_USER_INPUT,          /**< User input received */
    CTRL_EVENT_TIMER,               /**< Timer expired */
    CTRL_EVENT_COMMAND,             /**< Command received */
    CTRL_EVENT_DEVICE_STATUS,       /**< Device status change */
    CTRL_EVENT_SYSTEM_ERROR         /**< System error */
} ControllerEventType_e;

/**
 * @brief Action types for rule engine
 */
typedef enum {
    CTRL_ACTION_NONE = 0,           /**< No action */
    CTRL_ACTION_SEND_DATA,          /**< Send data over network */
    CTRL_ACTION_SET_OUTPUT,         /**< Set output pin/state */
    CTRL_ACTION_LOG_EVENT,          /**< Log event to storage */
    CTRL_ACTION_TRIGGER_ALARM,      /**< Trigger alarm */
    CTRL_ACTION_CLEAR_ALARM,        /**< Clear alarm */
    CTRL_ACTION_CHANGE_MODE,        /**< Change operating mode */
    CTRL_ACTION_REQUEST_DATA,       /**< Request sensor data */
    CTRL_ACTION_SEND_COMMAND,       /**< Send command to device */
    CTRL_ACTION_SLEEP,              /**< Enter low-power mode */
    CTRL_ACTION_WAKE                /**< Wake from low-power mode */
} ActionType_e;

/**
 * @brief Condition operators for rule engine
 */
typedef enum {
    CTRL_OP_EQUAL = 0,              /**< Equal to */
    CTRL_OP_NOT_EQUAL,              /**< Not equal to */
    CTRL_OP_GREATER,                /**< Greater than */
    CTRL_OP_LESS,                   /**< Less than */
    CTRL_OP_GREATER_EQUAL,          /**< Greater than or equal */
    CTRL_OP_LESS_EQUAL,             /**< Less than or equal */
    CTRL_OP_AND,                    /**< Logical AND */
    CTRL_OP_OR,                     /**< Logical OR */
    CTRL_OP_ALWAYS                  /**< Always true */
} ConditionOperator_e;

/**
 * @brief Controller error codes
 */
typedef enum {
    CTRL_OK = 0,                    /**< Success */
    CTRL_ERR_UNKNOWN = -1,          /**< Unknown error */
    CTRL_ERR_INVALID_PARAM = -2,    /**< Invalid parameter */
    CTRL_ERR_QUEUE_FULL = -3,       /**< Queue full */
    CTRL_ERR_NOT_INITIALIZED = -4,  /**< Not initialized */
    CTRL_ERR_RULE_NOT_FOUND = -5,   /**< Rule not found */
    CTRL_ERR_DEVICE_NOT_FOUND = -6, /**< Device not found */
    CTRL_ERR_TIMEOUT = -7           /**< Timeout */
} ControllerError_e;

/**
 * @brief Device status enumeration
 */
typedef enum {
    CTRL_DEVICE_OFFLINE = 0,        /**< Device offline */
    CTRL_DEVICE_ONLINE,             /**< Device online */
    CTRL_DEVICE_ERROR,              /**< Device error */
    CTRL_DEVICE_BUSY,               /**< Device busy */
    CTRL_DEVICE_UNKNOWN             /**< Status unknown */
} DeviceStatus_e;

/*============================================================================*/
/*                              STRUCTURES                                    */
/*============================================================================*/

/**
 * @brief Controller event message structure
 */
typedef struct {
    ControllerEventType_e  type;            /**< Event type */
    uint8_t                source;          /**< Event source ID */
    uint8_t                target;          /**< Target ID (0 = broadcast) */
    int16_t                errorCode;       /**< Error code if applicable */
    uint16_t               dataLength;      /**< Data payload length */
    uint8_t                data[CONTROLLER_CMD_BUFFER_SIZE]; /**< Data payload */
    void*                  context;         /**< User context */
} ControllerEvent_t;

/**
 * @brief Rule condition structure
 */
typedef struct {
    uint8_t                sensorId;        /**< Sensor to evaluate */
    ConditionOperator_e    op;              /**< Comparison operator */
    float                  value;           /**< Comparison value */
} RuleCondition_t;

/**
 * @brief Rule action structure
 */
typedef struct {
    ActionType_e           type;            /**< Action type */
    uint8_t                targetId;        /**< Target device/sensor ID */
    int16_t                parameter;       /**< Action parameter */
    float                  value;           /**< Action value */
} RuleAction_t;

/**
 * @brief Rule definition structure
 */
typedef struct {
    uint8_t                id;              /**< Rule identifier */
    char                   name[16];        /**< Rule name */
    RuleCondition_t        condition;       /**< Trigger condition */
    RuleAction_t           actions[CONTROLLER_MAX_ACTIONS]; /**< Actions */
    uint8_t                actionCount;     /**< Number of actions */
    bool                   enabled;         /**< Rule enabled flag */
    bool                   oneShot;         /**< Trigger once only */
    bool                   triggered;       /**< Already triggered flag */
} ControllerRule_t;

/**
 * @brief Device descriptor structure
 */
typedef struct {
    uint8_t                id;              /**< Device ID */
    char                   name[16];        /**< Device name */
    DeviceStatus_e         status;          /**< Current status */
    uint32_t               lastUpdate;      /**< Last update timestamp */
    uint8_t                capabilities;    /**< Capability flags */
    void*                  handle;          /**< Device handle */
} ControllerDevice_t;

/**
 * @brief Controller statistics structure
 */
typedef struct {
    uint32_t               eventsProcessed; /**< Total events processed */
    uint32_t               rulesTriggered;  /**< Rules triggered */
    uint32_t               alarmsActive;    /**< Active alarm count */
    uint32_t               alarmsTotal;     /**< Total alarm count */
    uint32_t               modeChanges;     /**< Mode change count */
    uint32_t               commandsSent;    /**< Commands sent */
    uint32_t               errorsCount;     /**< Error count */
    uint32_t               uptime;          /**< Uptime in seconds */
    uint32_t               lastEventTime;   /**< Last event timestamp */
} ControllerStats_t;

/**
 * @brief Controller configuration structure
 */
typedef struct {
    ControllerMode_e       defaultMode;     /**< Default operating mode */
    uint32_t               heartbeatMs;     /**< Heartbeat interval */
    uint32_t               watchdogTimeoutMs;/**< Watchdog timeout */
    bool                   autoRecovery;    /**< Auto-recovery enabled */
    bool                   logEvents;       /**< Event logging enabled */
} ControllerConfig_t;

/*============================================================================*/
/*                              CALLBACK TYPES                                */
/*============================================================================*/

/**
 * @brief Controller event callback
 * 
 * @param event     Event data
 * @param userData  User context
 */
typedef void (*ControllerCallback_t)(const ControllerEvent_t *event, void *userData);

/*============================================================================*/
/*                              PUBLIC API                                    */
/*============================================================================*/

/**
 * @brief Initialize the controller task subsystem
 * 
 * @return true on success
 */
bool ControllerTask_Init(void);

/**
 * @brief Create and start the controller task
 * 
 * @return true on success
 */
bool ControllerTask_Create(void);

/**
 * @brief Get controller task handle
 * 
 * @return Task handle or NULL
 */
void* ControllerTask_GetHandle(void);

/**
 * @brief Send event to controller
 * 
 * @param event Event to send
 * @return CTRL_OK on success
 */
ControllerError_e ControllerTask_SendEvent(const ControllerEvent_t *event);

/**
 * @brief Register a rule with the rule engine
 * 
 * @param rule Rule to register
 * @return CTRL_OK on success
 */
ControllerError_e ControllerTask_RegisterRule(const ControllerRule_t *rule);

/**
 * @brief Unregister a rule
 * 
 * @param ruleId Rule ID to unregister
 * @return CTRL_OK on success
 */
ControllerError_e ControllerTask_UnregisterRule(uint8_t ruleId);

/**
 * @brief Enable/disable a rule
 * 
 * @param ruleId  Rule ID
 * @param enabled Enable/disable flag
 * @return CTRL_OK on success
 */
ControllerError_e ControllerTask_SetRuleEnabled(uint8_t ruleId, bool enabled);

/**
 * @brief Register a device
 * 
 * @param device Device descriptor
 * @return CTRL_OK on success
 */
ControllerError_e ControllerTask_RegisterDevice(const ControllerDevice_t *device);

/**
 * @brief Unregister a device
 * 
 * @param deviceId Device ID to unregister
 * @return CTRL_OK on success
 */
ControllerError_e ControllerTask_UnregisterDevice(uint8_t deviceId);

/**
 * @brief Get current operating mode
 * 
 * @param pMode Output pointer for mode
 * @return CTRL_OK on success
 */
ControllerError_e ControllerTask_GetMode(ControllerMode_e *pMode);

/**
 * @brief Set operating mode
 * 
 * @param mode New operating mode
 * @return CTRL_OK on success
 */
ControllerError_e ControllerTask_SetMode(ControllerMode_e mode);

/**
 * @brief Get controller statistics
 * 
 * @param pStats Output pointer for statistics
 * @return CTRL_OK on success
 */
ControllerError_e ControllerTask_GetStats(ControllerStats_t *pStats);

/**
 * @brief Register event callback
 * 
 * @param callback Callback function
 * @param userData User context
 * @return true on success
 */
bool ControllerTask_RegisterCallback(ControllerCallback_t callback, void *userData);

/**
 * @brief Trigger an alarm
 * 
 * @param alarmId   Alarm identifier
 * @param severity  Alarm severity (0-255)
 * @param message   Alarm message
 * @return CTRL_OK on success
 */
ControllerError_e ControllerTask_TriggerAlarm(uint8_t alarmId, uint8_t severity,
                                               const char *message);

/**
 * @brief Clear an alarm
 * 
 * @param alarmId Alarm identifier
 * @return CTRL_OK on success
 */
ControllerError_e ControllerTask_ClearAlarm(uint8_t alarmId);

/**
 * @brief Process sensor data (internal use)
 * 
 * @param sensorId  Sensor ID
 * @param value     Sensor value
 * @param timestamp Sample timestamp
 */
void ControllerTask_ProcessSensorData(uint8_t sensorId, float value, uint32_t timestamp);

/**
 * @brief Set controller configuration
 * 
 * @param config Configuration to apply
 * @return CTRL_OK on success
 */
ControllerError_e ControllerTask_SetConfig(const ControllerConfig_t *config);

#ifdef __cplusplus
}
#endif

#endif /* TASK_CONTROLLER_H */
