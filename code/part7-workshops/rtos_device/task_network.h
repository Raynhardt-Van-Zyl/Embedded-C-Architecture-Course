/**
 * @file task_network.h
 * @brief Network Task Interface - FreeRTOS-based network communication
 * 
 * @details This header defines the interface for the network task that handles
 * all TCP/IP communication in an RTOS-based embedded system. The task manages
 * connections, handles protocols, and provides an abstraction layer for
 * application-level networking.
 * 
 * Architecture Principles:
 * - Single Responsibility: Network task handles only networking
 * - Queue-based Communication: Thread-safe message passing
 * - Event-driven Design: Responds to network and application events
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#ifndef TASK_NETWORK_H
#define TASK_NETWORK_H

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

/** @brief Network task name for FreeRTOS */
#define NETWORK_TASK_NAME           "NetworkTask"

/** @brief Network task stack size in words (4 bytes each on 32-bit) */
#define NETWORK_TASK_STACK_SIZE     (1024U)

/** @brief Network task priority (higher = more important) */
#define NETWORK_TASK_PRIORITY       (3U)

/** @brief Network event queue length */
#define NETWORK_QUEUE_LENGTH        (16U)

/** @brief Maximum packet size for transmission */
#define NETWORK_MAX_PACKET_SIZE     (1500U)

/** @brief Maximum number of simultaneous connections */
#define NETWORK_MAX_CONNECTIONS     (4U)

/** @brief Connection timeout in milliseconds */
#define NETWORK_CONNECT_TIMEOUT_MS  (10000U)

/** @brief Receive timeout in milliseconds */
#define NETWORK_RECV_TIMEOUT_MS     (5000U)

/** @brief Retry delay on connection failure */
#define NETWORK_RETRY_DELAY_MS      (5000U)

/** @brief Maximum retry attempts */
#define NETWORK_MAX_RETRIES         (3U)

/*============================================================================*/
/*                              ENUMERATIONS                                  */
/*============================================================================*/

/**
 * @brief Network connection state
 */
typedef enum {
    NET_STATE_DISCONNECTED = 0,     /**< Not connected */
    NET_STATE_CONNECTING,           /**< Connection in progress */
    NET_STATE_CONNECTED,            /**< Connected and ready */
    NET_STATE_AUTHENTICATING,       /**< Performing authentication */
    NET_STATE_ERROR,                /**< Error state */
    NET_STATE_SHUTDOWN              /**< Graceful shutdown */
} NetworkState_t;

/**
 * @brief Network event types
 */
typedef enum {
    NET_EVENT_NONE = 0,             /**< No event */
    NET_EVENT_DATA_RECEIVED,        /**< Data received on socket */
    NET_EVENT_CONNECTED,            /**< Connection established */
    NET_EVENT_DISCONNECTED,         /**< Connection closed */
    NET_EVENT_ERROR,                /**< Network error occurred */
    NET_EVENT_TX_COMPLETE,          /**< Transmission complete */
    NET_EVENT_DNS_RESOLVED,         /**< DNS resolution complete */
    NET_EVENT_TIMEOUT,              /**< Operation timeout */
    NET_EVENT_SEND_DATA,            /**< Request to send data */
    NET_EVENT_CONNECT,              /**< Request to connect */
    NET_EVENT_DISCONNECT,           /**< Request to disconnect */
    NET_EVENT_RECONNECT             /**< Request to reconnect */
} NetworkEventType_t;

/**
 * @brief Network protocol types
 */
typedef enum {
    NET_PROTO_TCP = 0,              /**< TCP protocol */
    NET_PROTO_UDP,                  /**< UDP protocol */
    NET_PROTO_TLS,                  /**< TLS over TCP */
    NET_PROTO_DTLS,                 /**< DTLS over UDP */
    NET_PROTO_HTTP,                 /**< HTTP over TCP */
    NET_PROTO_HTTPS,                /**< HTTPS over TLS */
    NET_PROTO_MQTT,                 /**< MQTT protocol */
    NET_PROTO_WEBSOCKET             /**< WebSocket protocol */
} NetworkProtocol_t;

/**
 * @brief Network error codes
 */
typedef enum {
    NET_OK = 0,                     /**< Success */
    NET_ERR_UNKNOWN = -1,           /**< Unknown error */
    NET_ERR_SOCKET = -2,            /**< Socket error */
    NET_ERR_BIND = -3,              /**< Bind error */
    NET_ERR_CONNECT = -4,           /**< Connection error */
    NET_ERR_TIMEOUT = -5,           /**< Timeout error */
    NET_ERR_DNS = -6,               /**< DNS resolution error */
    NET_ERR_MEMORY = -7,            /**< Memory allocation error */
    NET_ERR_BUFFER = -8,            /**< Buffer overflow */
    NET_ERR_CLOSED = -9,            /**< Connection closed */
    NET_ERR_WOULD_BLOCK = -10,      /**< Operation would block */
    NET_ERR_INVALID_ARG = -11,      /**< Invalid argument */
    NET_ERR_NOT_INITIALIZED = -12,  /**< Not initialized */
    NET_ERR_TLS = -13,              /**< TLS/SSL error */
    NET_ERR_AUTH = -14              /**< Authentication error */
} NetworkError_t;

/*============================================================================*/
/*                              STRUCTURES                                    */
/*============================================================================*/

/**
 * @brief Network address structure
 */
typedef struct {
    uint8_t  ip[4];                 /**< IPv4 address (network byte order) */
    uint16_t port;                  /**< Port number (host byte order) */
    char     hostname[64];          /**< Hostname for DNS resolution */
} NetworkAddress_t;

/**
 * @brief Network connection configuration
 */
typedef struct {
    NetworkProtocol_t protocol;     /**< Protocol to use */
    NetworkAddress_t  remote;       /**< Remote endpoint */
    NetworkAddress_t  local;        /**< Local endpoint (for bind) */
    uint32_t          connectTimeoutMs; /**< Connection timeout */
    uint32_t          recvTimeoutMs;    /**< Receive timeout */
    uint32_t          sendTimeoutMs;    /**< Send timeout */
    uint8_t           retryCount;       /**< Number of retry attempts */
    bool              keepAlive;        /**< Enable keep-alive */
    bool              useTLS;           /**< Use TLS encryption */
    bool              verifyCert;       /**< Verify server certificate */
} NetworkConfig_t;

/**
 * @brief Network buffer structure
 */
typedef struct {
    uint8_t  data[NETWORK_MAX_PACKET_SIZE]; /**< Data buffer */
    uint16_t length;                        /**< Data length */
    uint16_t offset;                        /**< Read/write offset */
} NetworkBuffer_t;

/**
 * @brief Network event message structure
 * 
 * @details Message structure for inter-task communication via queues.
 * Used to send commands and receive notifications.
 */
typedef struct {
    NetworkEventType_t  type;           /**< Event type */
    NetworkError_t      error;          /**< Error code (if applicable) */
    uint8_t             connectionId;   /**< Connection identifier */
    uint16_t            dataLength;     /**< Length of data payload */
    uint8_t             data[NETWORK_MAX_PACKET_SIZE]; /**< Data payload */
    void               *context;        /**< User context pointer */
} NetworkEvent_t;

/**
 * @brief Network statistics structure
 */
typedef struct {
    uint32_t bytesSent;             /**< Total bytes transmitted */
    uint32_t bytesReceived;         /**< Total bytes received */
    uint32_t packetsSent;           /**< Total packets sent */
    uint32_t packetsReceived;       /**< Total packets received */
    uint32_t connectCount;          /**< Number of connections */
    uint32_t disconnectCount;       /**< Number of disconnections */
    uint32_t errorCount;            /**< Total errors */
    uint32_t retryCount;            /**< Total retries */
    uint32_t uptime;                /**< Connection uptime (seconds) */
    int32_t  rtt;                   /**< Round-trip time (ms) */
} NetworkStats_t;

/**
 * @brief Network connection context
 */
typedef struct {
    NetworkState_t      state;          /**< Current state */
    NetworkConfig_t     config;         /**< Connection configuration */
    NetworkStats_t      stats;          /**< Connection statistics */
    uint32_t            lastActivity;   /**< Last activity timestamp */
    int32_t             socket;         /**< Socket handle */
    uint8_t             id;             /**< Connection ID */
    bool                active;         /**< Connection active flag */
} NetworkConnection_t;

/*============================================================================*/
/*                              CALLBACK TYPES                                */
/*============================================================================*/

/**
 * @brief Network event callback function type
 * 
 * @param event     Network event data
 * @param userData  User context pointer
 */
typedef void (*NetworkEventCallback_t)(const NetworkEvent_t *event, void *userData);

/*============================================================================*/
/*                              PUBLIC API                                    */
/*============================================================================*/

/**
 * @brief Create the network task
 * 
 * @details Creates and starts the FreeRTOS task for network handling.
 * Must be called after FreeRTOS scheduler is running.
 * 
 * @return true on success, false on failure
 */
bool NetworkTask_Create(void);

/**
 * @brief Get the network task handle
 * 
 * @return Task handle or NULL if not created
 */
void* NetworkTask_GetHandle(void);

/**
 * @brief Register a network event callback
 * 
 * @param callback  Callback function pointer
 * @param userData  User context passed to callback
 * @return true on success
 */
bool NetworkTask_RegisterCallback(NetworkEventCallback_t callback, void *userData);

/**
 * @brief Send data over the network
 * 
 * @details Queues a send request to the network task. Returns immediately.
 * 
 * @param data      Pointer to data buffer
 * @param length    Length of data in bytes
 * @param connId    Connection ID (0 for default)
 * @return NET_OK if queued successfully, error code otherwise
 */
NetworkError_t NetworkTask_Send(const uint8_t *data, uint16_t length, uint8_t connId);

/**
 * @brief Connect to a remote server
 * 
 * @details Initiates a connection to the specified remote endpoint.
 * 
 * @param config    Connection configuration
 * @param pConnId   Output pointer for connection ID
 * @return NET_OK on success, error code otherwise
 */
NetworkError_t NetworkTask_Connect(const NetworkConfig_t *config, uint8_t *pConnId);

/**
 * @brief Disconnect from remote server
 * 
 * @param connId    Connection ID to disconnect
 * @return NET_OK on success
 */
NetworkError_t NetworkTask_Disconnect(uint8_t connId);

/**
 * @brief Get current network state
 * 
 * @param connId    Connection ID
 * @param pState    Output pointer for state
 * @return NET_OK on success
 */
NetworkError_t NetworkTask_GetState(uint8_t connId, NetworkState_t *pState);

/**
 * @brief Get network statistics
 * 
 * @param connId    Connection ID
 * @param pStats    Output pointer for statistics
 * @return NET_OK on success
 */
NetworkError_t NetworkTask_GetStats(uint8_t connId, NetworkStats_t *pStats);

/**
 * @brief Check if network is connected
 * 
 * @param connId    Connection ID
 * @return true if connected
 */
bool NetworkTask_IsConnected(uint8_t connId);

/**
 * @brief Process incoming data (called by network task)
 * 
 * @param connId    Connection ID
 * @param data      Received data
 * @param length    Data length
 */
void NetworkTask_ProcessRxData(uint8_t connId, const uint8_t *data, uint16_t length);

/**
 * @brief Initialize network subsystem
 * 
 * @details Initializes the network stack and creates required resources.
 * 
 * @return NET_OK on success
 */
NetworkError_t NetworkTask_Init(void);

/**
 * @brief Deinitialize network subsystem
 */
void NetworkTask_Deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_NETWORK_H */
