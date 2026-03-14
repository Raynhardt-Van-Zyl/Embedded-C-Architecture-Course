/**
 * @file task_controller.c
 * @brief Controller Task Implementation - Main application logic
 * 
 * @details Implements the main controller task that coordinates all system
 * activities. Uses a rule engine for automated responses and manages
 * communication between all other tasks.
 * 
 * Key Features:
 * - Rule-based automation engine
 * - Event-driven processing
 * - Device management
 * - Alarm handling
 * - State machine for operating modes
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#include "task_controller.h"
#include "task_sensor.h"
#include "task_network.h"
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

/** @brief Controller processing tick interval */
#define CONTROLLER_TICK_MS          (50U)

/** @brief Heartbeat timer period */
#define HEARTBEAT_PERIOD_MS         (1000U)

/** @brief Watchdog check interval */
#define WATCHDOG_CHECK_MS           (500U)

/** @brief Maximum alarm message length */
#define ALARM_MSG_LENGTH            (32U)

/*============================================================================*/
/*                              PRIVATE TYPES                                 */
/*============================================================================*/

/**
 * @brief Alarm structure
 */
typedef struct {
    uint8_t     id;                 /**< Alarm ID */
    uint8_t     severity;           /**< Alarm severity */
    bool        active;             /**< Alarm active flag */
    uint32_t    triggerTime;        /**< Trigger timestamp */
    char        message[ALARM_MSG_LENGTH]; /**< Alarm message */
} Alarm_t;

/**
 * @brief Controller task context structure
 */
typedef struct {
    TaskHandle_t        taskHandle;         /**< Task handle */
    QueueHandle_t       eventQueue;         /**< Event queue */
    SemaphoreHandle_t   mutex;              /**< Resource mutex */
    TimerHandle_t       heartbeatTimer;     /**< Heartbeat timer */
    
    ControllerMode_e    currentMode;        /**< Current operating mode */
    ControllerConfig_t  config;             /**< Configuration */
    ControllerStats_t   stats;              /**< Statistics */
    
    ControllerRule_t    rules[CONTROLLER_MAX_RULES]; /**< Rule engine */
    uint8_t             ruleCount;          /**< Active rule count */
    
    ControllerDevice_t  devices[CONTROLLER_MAX_DEVICES]; /**< Device registry */
    uint8_t             deviceCount;        /**< Active device count */
    
    Alarm_t             alarms[8];          /**< Alarm registry */
    uint8_t             alarmCount;         /**< Active alarm count */
    
    ControllerCallback_t callback;          /**< Event callback */
    void*               callbackUserData;   /**< Callback user data */
    
    bool                initialized;        /**< Initialization flag */
    bool                running;            /**< Task running flag */
} ControllerContext_t;

/*============================================================================*/
/*                              PRIVATE VARIABLES                             */
/*============================================================================*/

/** @brief Controller context instance */
static ControllerContext_t s_ctrlCtx = {0};

/*============================================================================*/
/*                              PRIVATE FUNCTIONS                             */
/*============================================================================*/

/**
 * @brief Evaluate a rule condition
 * 
 * @param condition Condition to evaluate
 * @param sensorValue Current sensor value
 * @return true if condition is met
 */
static bool Controller_EvaluateCondition(const RuleCondition_t *condition, float sensorValue)
{
    switch (condition->op) {
        case CTRL_OP_EQUAL:
            return (sensorValue == condition->value);
        case CTRL_OP_NOT_EQUAL:
            return (sensorValue != condition->value);
        case CTRL_OP_GREATER:
            return (sensorValue > condition->value);
        case CTRL_OP_LESS:
            return (sensorValue < condition->value);
        case CTRL_OP_GREATER_EQUAL:
            return (sensorValue >= condition->value);
        case CTRL_OP_LESS_EQUAL:
            return (sensorValue <= condition->value);
        case CTRL_OP_ALWAYS:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Execute a rule action
 * 
 * @param action Action to execute
 * @param sourceId Source that triggered the action
 */
static void Controller_ExecuteAction(const RuleAction_t *action, uint8_t sourceId)
{
    NetworkEvent_t netEvent;
    
    switch (action->type) {
        case CTRL_ACTION_SEND_DATA:
            netEvent.type = NET_EVENT_SEND_DATA;
            netEvent.connectionId = action->targetId;
            netEvent.dataLength = 4;
            memcpy(netEvent.data, &action->value, 4);
            break;
            
        case CTRL_ACTION_SET_OUTPUT:
            break;
            
        case CTRL_ACTION_LOG_EVENT:
            break;
            
        case CTRL_ACTION_TRIGGER_ALARM:
            ControllerTask_TriggerAlarm(action->targetId, (uint8_t)action->parameter, "Rule triggered");
            break;
            
        case CTRL_ACTION_CLEAR_ALARM:
            ControllerTask_ClearAlarm(action->targetId);
            break;
            
        case CTRL_ACTION_CHANGE_MODE:
            s_ctrlCtx.currentMode = (ControllerMode_e)action->targetId;
            s_ctrlCtx.stats.modeChanges++;
            break;
            
        case CTRL_ACTION_SLEEP:
            break;
            
        default:
            break;
    }
    
    s_ctrlCtx.stats.commandsSent++;
}

/**
 * @brief Process rules against sensor data
 * 
 * @param sensorId Sensor ID
 * @param value Sensor value
 */
static void Controller_ProcessRules(uint8_t sensorId, float value)
{
    for (uint8_t i = 0; i < CONTROLLER_MAX_RULES; i++) {
        ControllerRule_t *rule = &s_ctrlCtx.rules[i];
        
        if (!rule->enabled) {
            continue;
        }
        
        if (rule->condition.sensorId != sensorId) {
            continue;
        }
        
        if (rule->oneShot && rule->triggered) {
            continue;
        }
        
        if (Controller_EvaluateCondition(&rule->condition, value)) {
            for (uint8_t j = 0; j < rule->actionCount; j++) {
                Controller_ExecuteAction(&rule->actions[j], sensorId);
            }
            
            rule->triggered = true;
            s_ctrlCtx.stats.rulesTriggered++;
        }
    }
}

/**
 * @brief Handle sensor data event
 * 
 * @param event Sensor data event
 */
static void Controller_HandleSensorData(const ControllerEvent_t *event)
{
    if (event->dataLength >= sizeof(float)) {
        float value;
        memcpy(&value, event->data, sizeof(float));
        
        Controller_ProcessRules(event->source, value);
        s_ctrlCtx.stats.lastEventTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    }
}

/**
 * @brief Handle network status event
 * 
 * @param event Network status event
 */
static void Controller_HandleNetworkStatus(const ControllerEvent_t *event)
{
    for (uint8_t i = 0; i < CONTROLLER_MAX_DEVICES; i++) {
        if (s_ctrlCtx.devices[i].id == event->source) {
            s_ctrlCtx.devices[i].status = (DeviceStatus_e)event->errorCode;
            s_ctrlCtx.devices[i].lastUpdate = xTaskGetTickCount() * portTICK_PERIOD_MS;
            break;
        }
    }
}

/**
 * @brief Handle user input event
 * 
 * @param event User input event
 */
static void Controller_HandleUserInput(const ControllerEvent_t *event)
{
    switch (event->data[0]) {
        case 'M':
            if (event->dataLength > 1) {
                ControllerTask_SetMode((ControllerMode_e)event->data[1]);
            }
            break;
        case 'S':
            break;
        case 'C':
            break;
        default:
            break;
    }
}

/**
 * @brief Handle timer event
 * 
 * @param event Timer event
 */
static void Controller_HandleTimer(const ControllerEvent_t *event)
{
    (void)event;
    
    s_ctrlCtx.stats.uptime++;
    
    if (s_ctrlCtx.alarmCount > 0) {
        s_ctrlCtx.stats.alarmsActive = s_ctrlCtx.alarmCount;
    }
}

/**
 * @brief Heartbeat timer callback
 * 
 * @param timer Timer handle
 */
static void Controller_HeartbeatCallback(TimerHandle_t timer)
{
    (void)timer;
    
    ControllerEvent_t event = {
        .type = CTRL_EVENT_TIMER,
        .source = 0,
        .dataLength = 0
    };
    
    xQueueSendFromISR(s_ctrlCtx.eventQueue, &event, NULL);
}

/**
 * @brief Process a single event
 * 
 * @param event Event to process
 */
static void Controller_ProcessEvent(const ControllerEvent_t *event)
{
    s_ctrlCtx.stats.eventsProcessed++;
    
    switch (event->type) {
        case CTRL_EVENT_SENSOR_DATA:
            Controller_HandleSensorData(event);
            break;
            
        case CTRL_EVENT_NETWORK_STATUS:
            Controller_HandleNetworkStatus(event);
            break;
            
        case CTRL_EVENT_NETWORK_DATA:
            break;
            
        case CTRL_EVENT_THRESHOLD:
            break;
            
        case CTRL_EVENT_ALARM:
            break;
            
        case CTRL_EVENT_USER_INPUT:
            Controller_HandleUserInput(event);
            break;
            
        case CTRL_EVENT_TIMER:
            Controller_HandleTimer(event);
            break;
            
        case CTRL_EVENT_COMMAND:
            break;
            
        case CTRL_EVENT_DEVICE_STATUS:
            break;
            
        case CTRL_EVENT_SYSTEM_ERROR:
            s_ctrlCtx.stats.errorsCount++;
            if (s_ctrlCtx.config.autoRecovery) {
            }
            break;
            
        default:
            break;
    }
    
    if (s_ctrlCtx.callback != NULL) {
        s_ctrlCtx.callback(event, s_ctrlCtx.callbackUserData);
    }
}

/**
 * @brief Main controller task function
 * 
 * @param pvParameters Task parameters
 */
static void Controller_TaskFunction(void *pvParameters)
{
    (void)pvParameters;
    ControllerEvent_t event;
    
    s_ctrlCtx.heartbeatTimer = xTimerCreate(
        "CtrlHeartbeat",
        pdMS_TO_TICKS(HEARTBEAT_PERIOD_MS),
        pdTRUE,
        NULL,
        Controller_HeartbeatCallback
    );
    
    if (s_ctrlCtx.heartbeatTimer != NULL) {
        xTimerStart(s_ctrlCtx.heartbeatTimer, 0);
    }
    
    s_ctrlCtx.running = true;
    s_ctrlCtx.currentMode = s_ctrlCtx.config.defaultMode;
    
    while (s_ctrlCtx.running) {
        if (xQueueReceive(s_ctrlCtx.eventQueue, &event, 
                          pdMS_TO_TICKS(CONTROLLER_TICK_MS)) == pdPASS) {
            if (xSemaphoreTake(s_ctrlCtx.mutex, pdMS_TO_TICKS(100)) == pdPASS) {
                Controller_ProcessEvent(&event);
                xSemaphoreGive(s_ctrlCtx.mutex);
            }
        }
    }
    
    if (s_ctrlCtx.heartbeatTimer != NULL) {
        xTimerStop(s_ctrlCtx.heartbeatTimer, 0);
        xTimerDelete(s_ctrlCtx.heartbeatTimer, 0);
    }
    
    vTaskDelete(NULL);
}

/*============================================================================*/
/*                              PUBLIC API IMPLEMENTATION                     */
/*============================================================================*/

bool ControllerTask_Init(void)
{
    if (s_ctrlCtx.initialized) {
        return true;
    }
    
    memset(&s_ctrlCtx, 0, sizeof(ControllerContext_t));
    
    s_ctrlCtx.eventQueue = xQueueCreate(CONTROLLER_QUEUE_LENGTH, sizeof(ControllerEvent_t));
    if (s_ctrlCtx.eventQueue == NULL) {
        return false;
    }
    
    s_ctrlCtx.mutex = xSemaphoreCreateMutex();
    if (s_ctrlCtx.mutex == NULL) {
        vQueueDelete(s_ctrlCtx.eventQueue);
        return false;
    }
    
    s_ctrlCtx.config.defaultMode = CTRL_MODE_NORMAL;
    s_ctrlCtx.config.heartbeatMs = HEARTBEAT_PERIOD_MS;
    s_ctrlCtx.config.autoRecovery = true;
    s_ctrlCtx.config.logEvents = true;
    
    s_ctrlCtx.currentMode = CTRL_MODE_NORMAL;
    
    s_ctrlCtx.initialized = true;
    return true;
}

bool ControllerTask_Create(void)
{
    if (!s_ctrlCtx.initialized) {
        if (!ControllerTask_Init()) {
            return false;
        }
    }
    
    BaseType_t result = xTaskCreate(
        Controller_TaskFunction,
        CONTROLLER_TASK_NAME,
        CONTROLLER_TASK_STACK_SIZE,
        NULL,
        CONTROLLER_TASK_PRIORITY,
        &s_ctrlCtx.taskHandle
    );
    
    return (result == pdPASS);
}

void* ControllerTask_GetHandle(void)
{
    return s_ctrlCtx.taskHandle;
}

ControllerError_e ControllerTask_SendEvent(const ControllerEvent_t *event)
{
    if (event == NULL) {
        return CTRL_ERR_INVALID_PARAM;
    }
    
    if (xQueueSend(s_ctrlCtx.eventQueue, event, pdMS_TO_TICKS(100)) != pdPASS) {
        return CTRL_ERR_QUEUE_FULL;
    }
    
    return CTRL_OK;
}

ControllerError_e ControllerTask_RegisterRule(const ControllerRule_t *rule)
{
    if (rule == NULL) {
        return CTRL_ERR_INVALID_PARAM;
    }
    
    if (xSemaphoreTake(s_ctrlCtx.mutex, pdMS_TO_TICKS(100)) != pdPASS) {
        return CTRL_ERR_TIMEOUT;
    }
    
    if (s_ctrlCtx.ruleCount >= CONTROLLER_MAX_RULES) {
        xSemaphoreGive(s_ctrlCtx.mutex);
        return CTRL_ERR_UNKNOWN;
    }
    
    s_ctrlCtx.rules[s_ctrlCtx.ruleCount] = *rule;
    s_ctrlCtx.ruleCount++;
    
    xSemaphoreGive(s_ctrlCtx.mutex);
    return CTRL_OK;
}

ControllerError_e ControllerTask_UnregisterRule(uint8_t ruleId)
{
    for (uint8_t i = 0; i < CONTROLLER_MAX_RULES; i++) {
        if (s_ctrlCtx.rules[i].id == ruleId) {
            s_ctrlCtx.rules[i].enabled = false;
            return CTRL_OK;
        }
    }
    return CTRL_ERR_RULE_NOT_FOUND;
}

ControllerError_e ControllerTask_SetRuleEnabled(uint8_t ruleId, bool enabled)
{
    for (uint8_t i = 0; i < CONTROLLER_MAX_RULES; i++) {
        if (s_ctrlCtx.rules[i].id == ruleId) {
            s_ctrlCtx.rules[i].enabled = enabled;
            if (!enabled) {
                s_ctrlCtx.rules[i].triggered = false;
            }
            return CTRL_OK;
        }
    }
    return CTRL_ERR_RULE_NOT_FOUND;
}

ControllerError_e ControllerTask_RegisterDevice(const ControllerDevice_t *device)
{
    if (device == NULL) {
        return CTRL_ERR_INVALID_PARAM;
    }
    
    if (s_ctrlCtx.deviceCount >= CONTROLLER_MAX_DEVICES) {
        return CTRL_ERR_UNKNOWN;
    }
    
    s_ctrlCtx.devices[s_ctrlCtx.deviceCount] = *device;
    s_ctrlCtx.deviceCount++;
    
    return CTRL_OK;
}

ControllerError_e ControllerTask_UnregisterDevice(uint8_t deviceId)
{
    for (uint8_t i = 0; i < CONTROLLER_MAX_DEVICES; i++) {
        if (s_ctrlCtx.devices[i].id == deviceId) {
            s_ctrlCtx.devices[i].status = CTRL_DEVICE_OFFLINE;
            return CTRL_OK;
        }
    }
    return CTRL_ERR_DEVICE_NOT_FOUND;
}

ControllerError_e ControllerTask_GetMode(ControllerMode_e *pMode)
{
    if (pMode == NULL) {
        return CTRL_ERR_INVALID_PARAM;
    }
    *pMode = s_ctrlCtx.currentMode;
    return CTRL_OK;
}

ControllerError_e ControllerTask_SetMode(ControllerMode_e mode)
{
    if (mode >= CTRL_MODE_COUNT) {
        return CTRL_ERR_INVALID_PARAM;
    }
    
    s_ctrlCtx.currentMode = mode;
    s_ctrlCtx.stats.modeChanges++;
    
    return CTRL_OK;
}

ControllerError_e ControllerTask_GetStats(ControllerStats_t *pStats)
{
    if (pStats == NULL) {
        return CTRL_ERR_INVALID_PARAM;
    }
    *pStats = s_ctrlCtx.stats;
    return CTRL_OK;
}

bool ControllerTask_RegisterCallback(ControllerCallback_t callback, void *userData)
{
    if (xSemaphoreTake(s_ctrlCtx.mutex, pdMS_TO_TICKS(100)) == pdPASS) {
        s_ctrlCtx.callback = callback;
        s_ctrlCtx.callbackUserData = userData;
        xSemaphoreGive(s_ctrlCtx.mutex);
        return true;
    }
    return false;
}

ControllerError_e ControllerTask_TriggerAlarm(uint8_t alarmId, uint8_t severity,
                                               const char *message)
{
    if (alarmId >= 8) {
        return CTRL_ERR_INVALID_PARAM;
    }
    
    Alarm_t *alarm = &s_ctrlCtx.alarms[alarmId];
    alarm->id = alarmId;
    alarm->severity = severity;
    alarm->active = true;
    alarm->triggerTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (message != NULL) {
        strncpy(alarm->message, message, ALARM_MSG_LENGTH - 1);
    }
    
    s_ctrlCtx.stats.alarmsTotal++;
    s_ctrlCtx.stats.alarmsActive++;
    
    ControllerEvent_t event = {
        .type = CTRL_EVENT_ALARM,
        .source = alarmId,
        .errorCode = severity
    };
    ControllerTask_SendEvent(&event);
    
    return CTRL_OK;
}

ControllerError_e ControllerTask_ClearAlarm(uint8_t alarmId)
{
    if (alarmId >= 8) {
        return CTRL_ERR_INVALID_PARAM;
    }
    
    s_ctrlCtx.alarms[alarmId].active = false;
    
    if (s_ctrlCtx.stats.alarmsActive > 0) {
        s_ctrlCtx.stats.alarmsActive--;
    }
    
    return CTRL_OK;
}

void ControllerTask_ProcessSensorData(uint8_t sensorId, float value, uint32_t timestamp)
{
    ControllerEvent_t event = {
        .type = CTRL_EVENT_SENSOR_DATA,
        .source = sensorId,
        .dataLength = sizeof(float) + sizeof(uint32_t)
    };
    memcpy(event.data, &value, sizeof(float));
    memcpy(event.data + sizeof(float), &timestamp, sizeof(uint32_t));
    
    ControllerTask_SendEvent(&event);
}

ControllerError_e ControllerTask_SetConfig(const ControllerConfig_t *config)
{
    if (config == NULL) {
        return CTRL_ERR_INVALID_PARAM;
    }
    
    s_ctrlCtx.config = *config;
    return CTRL_OK;
}
