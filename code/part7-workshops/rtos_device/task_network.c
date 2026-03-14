/**
 * @file task_network.c
 * @brief Network Task Implementation - FreeRTOS-based network communication
 * 
 * @details Implements the network task that handles all TCP/IP communication.
 * Uses FreeRTOS queues for inter-task communication and manages multiple
 * connections simultaneously.
 * 
 * Key Features:
 * - Event-driven architecture
 * - Non-blocking I/O operations
 * - Automatic reconnection
 * - Connection pooling
 * - Statistics tracking
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

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

/** @brief Network task tick interval in milliseconds */
#define NETWORK_TICK_MS             (100U)

/** @brief Maximum pending events in internal queue */
#define NETWORK_EVENT_QUEUE_LEN     (32U)

/** @brief Connection keepalive interval in seconds */
#define NETWORK_KEEPALIVE_INTERVAL  (30U)

/** @brief DNS cache entry lifetime in seconds */
#define DNS_CACHE_LIFETIME          (300U)

/*============================================================================*/
/*                              PRIVATE TYPES                                 */
/*============================================================================*/

/**
 * @brief Network task context structure
 */
typedef struct {
    TaskHandle_t        taskHandle;         /**< FreeRTOS task handle */
    QueueHandle_t       eventQueue;         /**< Event message queue */
    SemaphoreHandle_t   mutex;              /**< Resource protection mutex */
    TimerHandle_t       keepaliveTimer;     /**< Keepalive timer */
    
    NetworkConnection_t connections[NETWORK_MAX_CONNECTIONS]; /**< Connection pool */
    uint8_t             connectionCount;    /**< Active connection count */
    
    NetworkEventCallback_t callback;        /**< Event callback function */
    void*               callbackUserData;   /**< Callback user data */
    
    NetworkStats_t      globalStats;        /**< Global network statistics */
    bool                initialized;        /**< Initialization flag */
    bool                running;            /**< Task running flag */
} NetworkContext_t;

/*============================================================================*/
/*                              PRIVATE VARIABLES                             */
/*============================================================================*/

/** @brief Network task context instance */
static NetworkContext_t s_netCtx = {0};

/** 
 * @brief Static storage pool for connection configs
 * 
 * BUGFIX: This prevents lifetime bugs when callers pass stack-allocated
 * NetworkConfig_t to NetworkTask_Connect(). The config must remain valid
 * until the network task processes the connect event.
 * 
 * Without this pool, the pointer passed via event.context would become
 * invalid when the caller's stack frame is destroyed.
 */
static NetworkConfig_t s_configPool[NETWORK_MAX_CONNECTIONS];
static uint8_t s_configPoolUsed[NETWORK_MAX_CONNECTIONS] = {0};

/*============================================================================*/
/*                              PRIVATE FUNCTIONS                             */
/*============================================================================*/

/**
 * @brief Find a free connection slot
 * 
 * @return Connection slot index or -1 if none available
 */
static int8_t Network_FindFreeConnection(void)
{
    for (uint8_t i = 0; i < NETWORK_MAX_CONNECTIONS; i++) {
        if (!s_netCtx.connections[i].active) {
            return (int8_t)i;
        }
    }
    return -1;
}

/**
 * @brief Find connection by ID
 * 
 * @param connId Connection ID to find
 * @return Pointer to connection or NULL if not found
 */
static NetworkConnection_t* Network_FindConnection(uint8_t connId)
{
    if (connId < NETWORK_MAX_CONNECTIONS && s_netCtx.connections[connId].active) {
        return &s_netCtx.connections[connId];
    }
    return NULL;
}

/**
 * @brief Initialize a connection slot
 * 
 * @param conn     Connection to initialize
 * @param config   Connection configuration
 * @param id       Connection ID
 */
static void Network_InitConnection(NetworkConnection_t *conn, 
                                    const NetworkConfig_t *config,
                                    uint8_t id)
{
    memset(conn, 0, sizeof(NetworkConnection_t));
    conn->config = *config;
    conn->id = id;
    conn->state = NET_STATE_DISCONNECTED;
    conn->socket = -1;
    conn->active = true;
}

/**
 * @brief Process received data
 * 
 * @param conn     Connection context
 * @param data     Received data buffer
 * @param length   Data length
 */
static void Network_ProcessReceivedData(NetworkConnection_t *conn,
                                         const uint8_t *data,
                                         uint16_t length)
{
    conn->stats.bytesReceived += length;
    conn->stats.packetsReceived++;
    conn->lastActivity = xTaskGetTickCount() * portTICK_PERIOD_MS;
    s_netCtx.globalStats.bytesReceived += length;
    s_netCtx.globalStats.packetsReceived++;
    
    if (s_netCtx.callback != NULL) {
        NetworkEvent_t event = {
            .type = NET_EVENT_DATA_RECEIVED,
            .error = NET_OK,
            .connectionId = conn->id,
            .dataLength = length,
            .context = NULL
        };
        memcpy(event.data, data, length);
        s_netCtx.callback(&event, s_netCtx.callbackUserData);
    }
}

/**
 * @brief Handle connection state machine
 * 
 * @param conn Connection to process
 */
static void Network_ProcessConnectionState(NetworkConnection_t *conn)
{
    switch (conn->state) {
        case NET_STATE_CONNECTING:
            conn->state = NET_STATE_CONNECTED;
            conn->stats.connectCount++;
            s_netCtx.globalStats.connectCount++;
            
            if (s_netCtx.callback != NULL) {
                NetworkEvent_t event = {
                    .type = NET_EVENT_CONNECTED,
                    .connectionId = conn->id
                };
                s_netCtx.callback(&event, s_netCtx.callbackUserData);
            }
            break;
            
        case NET_STATE_CONNECTED:
            break;
            
        case NET_STATE_ERROR:
            if (conn->config.retryCount > 0) {
                conn->config.retryCount--;
                conn->state = NET_STATE_CONNECTING;
                conn->stats.retryCount++;
                s_netCtx.globalStats.retryCount++;
            } else {
                if (s_netCtx.callback != NULL) {
                    NetworkEvent_t event = {
                        .type = NET_EVENT_ERROR,
                        .error = NET_ERR_CONNECT,
                        .connectionId = conn->id
                    };
                    s_netCtx.callback(&event, s_netCtx.callbackUserData);
                }
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief Handle send data event
 * 
 * @param event Event containing data to send
 */
static void Network_HandleSendEvent(const NetworkEvent_t *event)
{
    NetworkConnection_t *conn = Network_FindConnection(event->connectionId);
    if (conn == NULL || conn->state != NET_STATE_CONNECTED) {
        return;
    }
    
    conn->stats.bytesSent += event->dataLength;
    conn->stats.packetsSent++;
    conn->lastActivity = xTaskGetTickCount() * portTICK_PERIOD_MS;
    s_netCtx.globalStats.bytesSent += event->dataLength;
    s_netCtx.globalStats.packetsSent++;
    
    if (s_netCtx.callback != NULL) {
        NetworkEvent_t txEvent = {
            .type = NET_EVENT_TX_COMPLETE,
            .connectionId = conn->id,
            .dataLength = event->dataLength
        };
        s_netCtx.callback(&txEvent, s_netCtx.callbackUserData);
    }
}

/**
 * @brief Handle connect event
 * 
 * @param event Connect event
 */
static void Network_HandleConnectEvent(const NetworkEvent_t *event)
{
    int8_t slot = Network_FindFreeConnection();
    if (slot < 0) {
        if (s_netCtx.callback != NULL) {
            NetworkEvent_t errEvent = {
                .type = NET_EVENT_ERROR,
                .error = NET_ERR_MEMORY
            };
            s_netCtx.callback(&errEvent, s_netCtx.callbackUserData);
        }
        return;
    }
    
    NetworkConnection_t *conn = &s_netCtx.connections[slot];
    Network_InitConnection(conn, (const NetworkConfig_t *)event->context, (uint8_t)slot);
    conn->state = NET_STATE_CONNECTING;
    s_netCtx.connectionCount++;
}

/**
 * @brief Handle disconnect event
 * 
 * @param event Disconnect event
 */
static void Network_HandleDisconnectEvent(const NetworkEvent_t *event)
{
    NetworkConnection_t *conn = Network_FindConnection(event->connectionId);
    if (conn == NULL) {
        return;
    }
    
    conn->state = NET_STATE_DISCONNECTED;
    conn->stats.disconnectCount++;
    s_netCtx.globalStats.disconnectCount++;
    conn->active = false;
    s_netCtx.connectionCount--;
    
    if (s_netCtx.callback != NULL) {
        NetworkEvent_t discEvent = {
            .type = NET_EVENT_DISCONNECTED,
            .connectionId = event->connectionId
        };
        s_netCtx.callback(&discEvent, s_netCtx.callbackUserData);
    }
}

/**
 * @brief Keepalive timer callback
 * 
 * @param timer Timer handle
 */
static void Network_KeepaliveCallback(TimerHandle_t timer)
{
    (void)timer;
    
    for (uint8_t i = 0; i < NETWORK_MAX_CONNECTIONS; i++) {
        NetworkConnection_t *conn = &s_netCtx.connections[i];
        if (conn->active && conn->state == NET_STATE_CONNECTED && conn->config.keepAlive) {
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if ((now - conn->lastActivity) > (NETWORK_KEEPALIVE_INTERVAL * 1000)) {
            }
        }
    }
}

/**
 * @brief Main network task function
 * 
 * @param pvParameters Task parameters (unused)
 */
static void Network_TaskFunction(void *pvParameters)
{
    (void)pvParameters;
    NetworkEvent_t event;
    
    s_netCtx.keepaliveTimer = xTimerCreate("NetKeepalive",
                                            pdMS_TO_TICKS(NETWORK_KEEPALIVE_INTERVAL * 1000),
                                            pdTRUE,
                                            NULL,
                                            Network_KeepaliveCallback);
    if (s_netCtx.keepaliveTimer != NULL) {
        xTimerStart(s_netCtx.keepaliveTimer, 0);
    }
    
    s_netCtx.running = true;
    
    while (s_netCtx.running) {
        if (xQueueReceive(s_netCtx.eventQueue, &event, 
                          pdMS_TO_TICKS(NETWORK_TICK_MS)) == pdPASS) {
            switch (event.type) {
                case NET_EVENT_SEND_DATA:
                    Network_HandleSendEvent(&event);
                    break;
                    
                case NET_EVENT_CONNECT:
                    Network_HandleConnectEvent(&event);
                    break;
                    
                case NET_EVENT_DISCONNECT:
                    Network_HandleDisconnectEvent(&event);
                    break;
                    
                case NET_EVENT_DATA_RECEIVED:
                    {
                        NetworkConnection_t *conn = Network_FindConnection(event.connectionId);
                        if (conn != NULL) {
                            Network_ProcessReceivedData(conn, event.data, event.dataLength);
                        }
                    }
                    break;
                    
                default:
                    break;
            }
        }
        
        if (xSemaphoreTake(s_netCtx.mutex, pdMS_TO_TICKS(10)) == pdPASS) {
            for (uint8_t i = 0; i < NETWORK_MAX_CONNECTIONS; i++) {
                if (s_netCtx.connections[i].active) {
                    Network_ProcessConnectionState(&s_netCtx.connections[i]);
                }
            }
            xSemaphoreGive(s_netCtx.mutex);
        }
    }
    
    if (s_netCtx.keepaliveTimer != NULL) {
        xTimerStop(s_netCtx.keepaliveTimer, 0);
        xTimerDelete(s_netCtx.keepaliveTimer, 0);
    }
    
    vTaskDelete(NULL);
}

/*============================================================================*/
/*                              PUBLIC API IMPLEMENTATION                     */
/*============================================================================*/

NetworkError_t NetworkTask_Init(void)
{
    if (s_netCtx.initialized) {
        return NET_OK;
    }
    
    memset(&s_netCtx, 0, sizeof(NetworkContext_t));
    
    s_netCtx.eventQueue = xQueueCreate(NETWORK_EVENT_QUEUE_LEN, sizeof(NetworkEvent_t));
    if (s_netCtx.eventQueue == NULL) {
        return NET_ERR_MEMORY;
    }
    
    s_netCtx.mutex = xSemaphoreCreateMutex();
    if (s_netCtx.mutex == NULL) {
        vQueueDelete(s_netCtx.eventQueue);
        return NET_ERR_MEMORY;
    }
    
    s_netCtx.initialized = true;
    return NET_OK;
}

void NetworkTask_Deinit(void)
{
    if (!s_netCtx.initialized) {
        return;
    }
    
    s_netCtx.running = false;
    
    vTaskDelay(pdMS_TO_TICKS(NETWORK_TICK_MS * 2));
    
    if (s_netCtx.mutex != NULL) {
        vSemaphoreDelete(s_netCtx.mutex);
    }
    if (s_netCtx.eventQueue != NULL) {
        vQueueDelete(s_netCtx.eventQueue);
    }
    
    memset(&s_netCtx, 0, sizeof(NetworkContext_t));
}

bool NetworkTask_Create(void)
{
    if (!s_netCtx.initialized) {
        if (NetworkTask_Init() != NET_OK) {
            return false;
        }
    }
    
    BaseType_t result = xTaskCreate(
        Network_TaskFunction,
        NETWORK_TASK_NAME,
        NETWORK_TASK_STACK_SIZE,
        NULL,
        NETWORK_TASK_PRIORITY,
        &s_netCtx.taskHandle
    );
    
    return (result == pdPASS);
}

void* NetworkTask_GetHandle(void)
{
    return s_netCtx.taskHandle;
}

bool NetworkTask_RegisterCallback(NetworkEventCallback_t callback, void *userData)
{
    if (xSemaphoreTake(s_netCtx.mutex, pdMS_TO_TICKS(100)) == pdPASS) {
        s_netCtx.callback = callback;
        s_netCtx.callbackUserData = userData;
        xSemaphoreGive(s_netCtx.mutex);
        return true;
    }
    return false;
}

NetworkError_t NetworkTask_Send(const uint8_t *data, uint16_t length, uint8_t connId)
{
    if (data == NULL || length == 0 || length > NETWORK_MAX_PACKET_SIZE) {
        return NET_ERR_INVALID_ARG;
    }
    
    NetworkEvent_t event = {
        .type = NET_EVENT_SEND_DATA,
        .connectionId = connId,
        .dataLength = length
    };
    memcpy(event.data, data, length);
    
    if (xQueueSend(s_netCtx.eventQueue, &event, pdMS_TO_TICKS(100)) != pdPASS) {
        return NET_ERR_WOULD_BLOCK;
    }
    
    return NET_OK;
}

NetworkError_t NetworkTask_Connect(const NetworkConfig_t *config, uint8_t *pConnId)
{
    if (config == NULL || pConnId == NULL) {
        return NET_ERR_INVALID_ARG;
    }
    
    int8_t slot = Network_FindFreeConnection();
    if (slot < 0) {
        return NET_ERR_MEMORY;
    }
    
    /*
     * BUGFIX: Copy config into static storage pool instead of storing raw pointer.
     * 
     * ARCHITECTURE RATIONALE:
     * In RTOS message-passing, you must NEVER pass pointers to stack-allocated
     * data across task boundaries. The caller's NetworkConfig_t might be on their
     * stack frame. When their function returns, the stack is reclaimed and the
     * pointer becomes invalid (dangling pointer). When the network task later
     * dereferences event->context, it reads garbage memory.
     * 
     * PROPER PATTERN:
     * Option A: Copy data into the event structure (if it fits)
     * Option B: Use a static storage pool (this implementation)
     * Option C: Use dynamic allocation with explicit ownership transfer
     * 
     * This implementation uses Option B - a static pool of NetworkConfig_t
     * structures. Each connection slot has a corresponding config storage slot.
     */
    
    /* Copy caller's config into our static storage pool */
    memcpy(&s_configPool[slot], config, sizeof(NetworkConfig_t));
    s_configPoolUsed[slot] = 1;
    
    NetworkEvent_t event = {
        .type = NET_EVENT_CONNECT,
        .connectionId = (uint8_t)slot,
        .context = &s_configPool[slot]  /* Safe: points to our static storage */
    };
    
    if (xQueueSend(s_netCtx.eventQueue, &event, pdMS_TO_TICKS(100)) != pdPASS) {
        s_configPoolUsed[slot] = 0;  /* Rollback on failure */
        return NET_ERR_WOULD_BLOCK;
    }
    
    *pConnId = (uint8_t)slot;
    return NET_OK;
}

NetworkError_t NetworkTask_Disconnect(uint8_t connId)
{
    NetworkEvent_t event = {
        .type = NET_EVENT_DISCONNECT,
        .connectionId = connId
    };
    
    if (xQueueSend(s_netCtx.eventQueue, &event, pdMS_TO_TICKS(100)) != pdPASS) {
        return NET_ERR_WOULD_BLOCK;
    }
    
    return NET_OK;
}

NetworkError_t NetworkTask_GetState(uint8_t connId, NetworkState_t *pState)
{
    if (pState == NULL) {
        return NET_ERR_INVALID_ARG;
    }
    
    NetworkConnection_t *conn = Network_FindConnection(connId);
    if (conn == NULL) {
        return NET_ERR_INVALID_ARG;
    }
    
    *pState = conn->state;
    return NET_OK;
}

NetworkError_t NetworkTask_GetStats(uint8_t connId, NetworkStats_t *pStats)
{
    if (pStats == NULL) {
        return NET_ERR_INVALID_ARG;
    }
    
    if (connId >= NETWORK_MAX_CONNECTIONS) {
        *pStats = s_netCtx.globalStats;
        return NET_OK;
    }
    
    NetworkConnection_t *conn = Network_FindConnection(connId);
    if (conn == NULL) {
        return NET_ERR_INVALID_ARG;
    }
    
    *pStats = conn->stats;
    return NET_OK;
}

bool NetworkTask_IsConnected(uint8_t connId)
{
    NetworkConnection_t *conn = Network_FindConnection(connId);
    return (conn != NULL && conn->state == NET_STATE_CONNECTED);
}

void NetworkTask_ProcessRxData(uint8_t connId, const uint8_t *data, uint16_t length)
{
    NetworkEvent_t event = {
        .type = NET_EVENT_DATA_RECEIVED,
        .connectionId = connId,
        .dataLength = length
    };
    memcpy(event.data, data, length);
    
    xQueueSendFromISR(s_netCtx.eventQueue, &event, NULL);
}
