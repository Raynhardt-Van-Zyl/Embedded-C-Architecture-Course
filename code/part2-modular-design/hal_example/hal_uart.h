/**
 * @file Haluart.h
 * @brief Hardware Abstraction Layer for UART peripheral
 * 
 * This file demonstrates the proper design of a HAL interface following
 * the architectural principles outlined in Chapter 4.
 * 
 * Key Design Decisions:
 * 1. Opaque pointer pattern - hides implementation details
 * 2. Configuration via structs - not vendor-specific macros
 * 3. Standardized error codes - mapped from vendor errors
 * 4. Callback registration - for async/ISR operation
 * 5. Handle-based API - supports multiple instances
 * 
 * @author Embedded Architecture Team
 * @version 1.0.0
 * @date 2024
 */

#ifndef HalUART_H
#define HalUART_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/* INCLUDES                                                                   */
/*============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*============================================================================*/
/* PUBLIC TYPES                                                               */
/*============================================================================*/

/**
 * @brief Opaque handle to UART instance
 * 
 * The actual structure is defined only in the implementation file,
 * preventing application code from depending on internal details.
 * This is the PIMPL (Pointer to IMPLementation) pattern in C.
 */
typedef struct HalUart_s* HalUartHandle;

/**
 * @brief UART instance identifiers
 * 
 * These map to physical UART peripherals on the MCU.
 * The HAL implementation translates these to vendor-specific identifiers.
 */
typedef enum {
    HalUART_0 = 0,     /**< UART instance 0 */
    HalUART_1,         /**< UART instance 1 */
    HalUART_2,         /**< UART instance 2 */
    HalUART_3,         /**< UART instance 3 */
    HalUART_MAX        /**< Sentinel value for bounds checking */
} HalUartId_t;

/**
 * @brief Standardized baud rates
 * 
 * Using an enum instead of raw integers prevents invalid values
 * at compile time and documents supported rates.
 */
typedef enum {
    HalUART_BAUD_1200    = 1200,
    HalUART_BAUD_2400    = 2400,
    HalUART_BAUD_4800    = 4800,
    HalUART_BAUD_9600    = 9600,
    HalUART_BAUD_19200   = 19200,
    HalUART_BAUD_38400   = 38400,
    HalUART_BAUD_57600   = 57600,
    HalUART_BAUD_115200  = 115200,
    HalUART_BAUD_230400  = 230400,
    HalUART_BAUD_460800  = 460800,
    HalUART_BAUD_921600  = 921600,
    HalUART_BAUD_1000000 = 1000000
} HalUartBaudRate_t;

/**
 * @brief Number of data bits per frame
 */
typedef enum {
    HalUART_DATA_BITS_7 = 7,   /**< 7 data bits */
    HalUART_DATA_BITS_8 = 8,   /**< 8 data bits (most common) */
    HalUART_DATA_BITS_9 = 9    /**< 9 data bits (for address/mode) */
} HalUartDataBits_t;

/**
 * @brief Parity configuration
 */
typedef enum {
    HalUART_PARITY_NONE = 0,   /**< No parity bit */
    HalUART_PARITY_EVEN,       /**< Even parity */
    HalUART_PARITY_ODD         /**< Odd parity */
} HalUartParity_t;

/**
 * @brief Number of stop bits
 */
typedef enum {
    HalUART_STOP_BITS_1 = 1,   /**< 1 stop bit (most common) */
    HalUART_STOP_BITS_2 = 2    /**< 2 stop bits */
} HalUartStopBits_t;

/**
 * @brief Flow control options
 */
typedef enum {
    HalUART_FLOW_NONE = 0,     /**< No flow control */
    HalUART_FLOW_RTS,          /**< RTS hardware flow control */
    HalUART_FLOW_CTS,          /**< CTS hardware flow control */
    HalUART_FLOW_RTS_CTS       /**< Full RTS/CTS hardware flow control */
} HalUartFlowControl_t;

/**
 * @brief UART configuration structure
 * 
 * This structure contains all configuration parameters for a UART instance.
 * It is passed to HalUart_Init() to configure the peripheral.
 * 
 * DESIGN RULE: All configuration should be passed via a struct, not
 * individual function parameters. This allows adding new options
 * without changing the API signature (API stability).
 */
typedef struct {
    HalUartBaudRate_t      baud_rate;       /**< Desired baud rate */
    HalUartDataBits_t      data_bits;       /**< Data bits per frame */
    HalUartParity_t        parity;          /**< Parity mode */
    HalUartStopBits_t      stop_bits;       /**< Stop bits */
    HalUartFlowControl_t   flow_control;    /**< Flow control mode */
    
    /* Buffer configuration for async operation */
    uint16_t               tx_buffer_size;  /**< TX buffer size (0 = no buffering) */
    uint16_t               rx_buffer_size;  /**< RX buffer size (0 = no buffering) */
    
    /* Interrupt configuration */
    bool                   enable_rx_irq;   /**< Enable RX interrupt */
    bool                   enable_tx_irq;   /**< Enable TX empty interrupt */
    bool                   enable_error_irq; /**< Enable error interrupts */
    
} HalUartConfig_t;

/**
 * @brief Standardized error codes for UART HAL
 * 
 * These error codes are mapped from vendor-specific errors in the
 * implementation. The application never sees vendor error codes.
 * 
 * DESIGN RULE: Error codes should be positive integers for errors,
 * zero for success, and organized by category (high nibble).
 */
typedef enum {
    /* Success */
    HalUART_OK                     = 0,    /**< Operation successful */
    
    /* General errors (0x01-0x0F) */
    HalUART_ERROR                  = 0x01, /**< Unspecified error */
    HalUART_ERROR_INVALID_PARAM    = 0x02, /**< Invalid parameter */
    HalUART_ERROR_NULL_POINTER     = 0x03, /**< NULL pointer passed */
    HalUART_ERROR_NOT_INITIALIZED  = 0x04, /**< Instance not initialized */
    HalUART_ERROR_ALREADY_INIT     = 0x05, /**< Instance already initialized */
    
    /* Resource errors (0x10-0x1F) */
    HalUART_ERROR_NO_MEMORY        = 0x10, /**< Memory allocation failed */
    HalUART_ERROR_BUFFER_FULL      = 0x11, /**< TX buffer full */
    HalUART_ERROR_BUFFER_EMPTY     = 0x12, /**< RX buffer empty */
    HalUART_ERROR_TIMEOUT          = 0x13, /**< Operation timed out */
    
    /* Hardware errors (0x20-0x2F) */
    HalUART_ERROR_HARDWARE         = 0x20, /**< Hardware fault */
    HalUART_ERROR_FRAMING          = 0x21, /**< Framing error detected */
    HalUART_ERROR_PARITY           = 0x22, /**< Parity error detected */
    HalUART_ERROR_OVERRUN          = 0x23, /**< Overrun error detected */
    HalUART_ERROR_NOISE            = 0x24, /**< Noise detected */
    HalUART_ERROR_BREAK            = 0x25, /**< Break condition detected */
    
    /* Configuration errors (0x30-0x3F) */
    HalUART_ERROR_BAUD_UNSUPPORTED = 0x30, /**< Baud rate not supported */
    HalUART_ERROR_CONFIG_INVALID   = 0x31, /**< Invalid configuration combo */
    
    /* Instance errors (0x40-0x4F) */
    HalUART_ERROR_INVALID_ID       = 0x40, /**< Invalid UART ID */
    HalUART_ERROR_NOT_AVAILABLE    = 0x41, /**< UART not available/in use */
    
} HalUartError_t;

/**
 * @brief UART event types for callback notification
 */
typedef enum {
    HalUART_EVENT_RX_COMPLETE   = 0x01, /**< RX transfer complete */
    HalUART_EVENT_TX_COMPLETE   = 0x02, /**< TX transfer complete */
    HalUART_EVENT_RX_DATA       = 0x04, /**< Data received (byte available) */
    HalUART_EVENT_RX_OVERFLOW   = 0x08, /**< RX buffer overflow */
    HalUART_EVENT_FRAMING_ERROR = 0x10, /**< Framing error */
    HalUART_EVENT_PARITY_ERROR  = 0x20, /**< Parity error */
    HalUART_EVENT_BREAK_DETECT  = 0x40, /**< Break detected */
    HalUART_EVENT_ERROR         = 0x80, /**< General error */
} HalUartEvent_t;

/**
 * @brief Callback function type for UART events
 * 
 * @param uart     Handle to the UART instance that generated the event
 * @param event    Bitmask of events that occurred (see HalUartEvent_t)
 * @param context  User-provided context pointer
 */
typedef void (*HalUartCallback_t)(HalUart_t *uart, uint32_t event, void *context);

/*============================================================================*/
/* PUBLIC CONSTANTS                                                          */
/*============================================================================*/

/**
 * @brief Default UART configuration
 * 
 * Use this as a starting point and modify only what you need.
 * This is the "most common" configuration for embedded systems.
 */
static const HalUartConfig_t HalUART_CONFIG_DEFAULT = {
    .baud_rate       = HalUART_BAUD_115200,
    .data_bits       = HalUART_DATA_BITS_8,
    .parity          = HalUART_PARITY_NONE,
    .stop_bits       = HalUART_STOP_BITS_1,
    .flow_control    = HalUART_FLOW_NONE,
    .tx_buffer_size  = 256,
    .rx_buffer_size  = 256,
    .enable_rx_irq   = true,
    .enable_tx_irq   = false,
    .enable_error_irq = true,
};

/*============================================================================*/
/* PUBLIC FUNCTION PROTOTYPES                                                 */
/*============================================================================*/

/*----------------------------------------------------------------------------*/
/* Initialization / Deinitialization                                          */
/*----------------------------------------------------------------------------*/

/**
 * @brief Initialize a UART instance
 * 
 * This function allocates and initializes a UART instance. The returned
 * handle must be used for all subsequent operations on this UART.
 * 
 * @param id      UART instance identifier (e.g., HalUART_0)
 * @param config  Pointer to configuration structure
 * @return        Handle to UART instance, or NULL on error
 * 
 * @pre           config must not be NULL
 * @post          UART peripheral is configured and ready for use
 * 
 * @note          This function is NOT thread-safe. Do not call from ISRs.
 * @note          Call HalUart_Deinit() when done to release resources.
 */
HalUartHandle HalUart_Init(HalUartId_t id, const HalUartConfig_t *config);
HalUartError_t HalUart_Deinit(HalUartHandle uart);
int32_t HalUart_WriteBlocking(HalUartHandle uart, const uint8_t *data, 
                                uint32_t length, uint32_t timeout_ms);
int32_t HalUart_ReadBlocking(HalUartHandle uart, uint8_t *buffer, 
                               uint32_t length, uint32_t timeout_ms);
HalUartError_t HalUart_WriteAsync(HalUartHandle uart, const uint8_t *data, 
                                    uint32_t length);
bool HalUart_IsTxDone(HalUartHandle uart);
uint32_t HalUart_GetRxAvailable(HalUartHandle uart);
uint32_t HalUart_Read(HalUartHandle uart, uint8_t *buffer, uint32_t length);
HalUartError_t HalUart_RegisterCallback(HalUartHandle uart, 
                                          HalUartCallback_t callback,
                                          void *context);
HalUartError_t HalUart_ClearErrors(HalUartHandle uart);
HalUartError_t HalUart_FlushTx(HalUartHandle uart);
HalUartError_t HalUart_FlushRx(HalUartHandle uart);
uint32_t HalUart_GetBaudRate(HalUartHandle uart);
HalUartError_t HalUart_SetBaudRate(HalUartHandle uart, HalUartBaudRate_t baud_rate);

/*============================================================================*/
/* INLINE HELPER FUNCTIONS                                                    */
/*============================================================================*/

/**
 * @brief Check if error code indicates success
 */
static inline bool HalUart_IsOk(HalUartError_t err) {
    return (err == HalUART_OK);
}

/**
 * @brief Check if error code indicates a hardware error
 */
static inline bool HalUart_IsHardwareError(HalUartError_t err) {
    return (err >= HalUART_ERROR_HARDWARE && err <= HalUART_ERROR_BREAK);
}

/**
 * @brief Get human-readable error string
 * 
 * @param err    Error code
 * @return       Pointer to static error string (do not free)
 */
const char* HalUart_ErrorToString(HalUartError_t err);

#ifdef __cplusplus
}
#endif

#endif /* HalUART_H */
