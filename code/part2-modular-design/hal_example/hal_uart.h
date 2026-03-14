/**
 * @file hal_uart.h
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

#ifndef HAL_UART_H
#define HAL_UART_H

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
typedef struct HalUartInstance HalUart_t;

/**
 * @brief UART instance identifiers
 * 
 * These map to physical UART peripherals on the MCU.
 * The HAL implementation translates these to vendor-specific identifiers.
 */
typedef enum {
    HAL_UART_0 = 0,     /**< UART instance 0 */
    HAL_UART_1,         /**< UART instance 1 */
    HAL_UART_2,         /**< UART instance 2 */
    HAL_UART_3,         /**< UART instance 3 */
    HAL_UART_MAX        /**< Sentinel value for bounds checking */
} HalUartId_t;

/**
 * @brief Standardized baud rates
 * 
 * Using an enum instead of raw integers prevents invalid values
 * at compile time and documents supported rates.
 */
typedef enum {
    HAL_UART_BAUD_1200    = 1200,
    HAL_UART_BAUD_2400    = 2400,
    HAL_UART_BAUD_4800    = 4800,
    HAL_UART_BAUD_9600    = 9600,
    HAL_UART_BAUD_19200   = 19200,
    HAL_UART_BAUD_38400   = 38400,
    HAL_UART_BAUD_57600   = 57600,
    HAL_UART_BAUD_115200  = 115200,
    HAL_UART_BAUD_230400  = 230400,
    HAL_UART_BAUD_460800  = 460800,
    HAL_UART_BAUD_921600  = 921600,
    HAL_UART_BAUD_1000000 = 1000000
} HalUartBaudRate_t;

/**
 * @brief Number of data bits per frame
 */
typedef enum {
    HAL_UART_DATA_BITS_7 = 7,   /**< 7 data bits */
    HAL_UART_DATA_BITS_8 = 8,   /**< 8 data bits (most common) */
    HAL_UART_DATA_BITS_9 = 9    /**< 9 data bits (for address/mode) */
} HalUartDataBits_t;

/**
 * @brief Parity configuration
 */
typedef enum {
    HAL_UART_PARITY_NONE = 0,   /**< No parity bit */
    HAL_UART_PARITY_EVEN,       /**< Even parity */
    HAL_UART_PARITY_ODD         /**< Odd parity */
} HalUartParity_t;

/**
 * @brief Number of stop bits
 */
typedef enum {
    HAL_UART_STOP_BITS_1 = 1,   /**< 1 stop bit (most common) */
    HAL_UART_STOP_BITS_2 = 2    /**< 2 stop bits */
} HalUartStopBits_t;

/**
 * @brief Flow control options
 */
typedef enum {
    HAL_UART_FLOW_NONE = 0,     /**< No flow control */
    HAL_UART_FLOW_RTS,          /**< RTS hardware flow control */
    HAL_UART_FLOW_CTS,          /**< CTS hardware flow control */
    HAL_UART_FLOW_RTS_CTS       /**< Full RTS/CTS hardware flow control */
} HalUartFlowControl_t;

/**
 * @brief UART configuration structure
 * 
 * This structure contains all configuration parameters for a UART instance.
 * It is passed to Hal_Uart_Init() to configure the peripheral.
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
    HAL_UART_OK                     = 0,    /**< Operation successful */
    
    /* General errors (0x01-0x0F) */
    HAL_UART_ERROR                  = 0x01, /**< Unspecified error */
    HAL_UART_ERROR_INVALID_PARAM    = 0x02, /**< Invalid parameter */
    HAL_UART_ERROR_NULL_POINTER     = 0x03, /**< NULL pointer passed */
    HAL_UART_ERROR_NOT_INITIALIZED  = 0x04, /**< Instance not initialized */
    HAL_UART_ERROR_ALREADY_INIT     = 0x05, /**< Instance already initialized */
    
    /* Resource errors (0x10-0x1F) */
    HAL_UART_ERROR_NO_MEMORY        = 0x10, /**< Memory allocation failed */
    HAL_UART_ERROR_BUFFER_FULL      = 0x11, /**< TX buffer full */
    HAL_UART_ERROR_BUFFER_EMPTY     = 0x12, /**< RX buffer empty */
    HAL_UART_ERROR_TIMEOUT          = 0x13, /**< Operation timed out */
    
    /* Hardware errors (0x20-0x2F) */
    HAL_UART_ERROR_HARDWARE         = 0x20, /**< Hardware fault */
    HAL_UART_ERROR_FRAMING          = 0x21, /**< Framing error detected */
    HAL_UART_ERROR_PARITY           = 0x22, /**< Parity error detected */
    HAL_UART_ERROR_OVERRUN          = 0x23, /**< Overrun error detected */
    HAL_UART_ERROR_NOISE            = 0x24, /**< Noise detected */
    HAL_UART_ERROR_BREAK            = 0x25, /**< Break condition detected */
    
    /* Configuration errors (0x30-0x3F) */
    HAL_UART_ERROR_BAUD_UNSUPPORTED = 0x30, /**< Baud rate not supported */
    HAL_UART_ERROR_CONFIG_INVALID   = 0x31, /**< Invalid configuration combo */
    
    /* Instance errors (0x40-0x4F) */
    HAL_UART_ERROR_INVALID_ID       = 0x40, /**< Invalid UART ID */
    HAL_UART_ERROR_NOT_AVAILABLE    = 0x41, /**< UART not available/in use */
    
} HalUartError_t;

/**
 * @brief UART event types for callback notification
 */
typedef enum {
    HAL_UART_EVENT_RX_COMPLETE   = 0x01, /**< RX transfer complete */
    HAL_UART_EVENT_TX_COMPLETE   = 0x02, /**< TX transfer complete */
    HAL_UART_EVENT_RX_DATA       = 0x04, /**< Data received (byte available) */
    HAL_UART_EVENT_RX_OVERFLOW   = 0x08, /**< RX buffer overflow */
    HAL_UART_EVENT_FRAMING_ERROR = 0x10, /**< Framing error */
    HAL_UART_EVENT_PARITY_ERROR  = 0x20, /**< Parity error */
    HAL_UART_EVENT_BREAK_DETECT  = 0x40, /**< Break detected */
    HAL_UART_EVENT_ERROR         = 0x80, /**< General error */
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
static const HalUartConfig_t HAL_UART_CONFIG_DEFAULT = {
    .baud_rate       = HAL_UART_BAUD_115200,
    .data_bits       = HAL_UART_DATA_BITS_8,
    .parity          = HAL_UART_PARITY_NONE,
    .stop_bits       = HAL_UART_STOP_BITS_1,
    .flow_control    = HAL_UART_FLOW_NONE,
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
 * @param id      UART instance identifier (e.g., HAL_UART_0)
 * @param config  Pointer to configuration structure
 * @return        Handle to UART instance, or NULL on error
 * 
 * @pre           config must not be NULL
 * @post          UART peripheral is configured and ready for use
 * 
 * @note          This function is NOT thread-safe. Do not call from ISRs.
 * @note          Call Hal_Uart_Deinit() when done to release resources.
 */
HalUart_t* Hal_Uart_Init(HalUartId_t id, const HalUartConfig_t *config);

/**
 * @brief Deinitialize a UART instance
 * 
 * Releases all resources associated with the UART instance and disables
 * the peripheral. The handle becomes invalid after this call.
 * 
 * @param uart    Handle to UART instance (from Hal_Uart_Init)
 * @return        HAL_UART_OK on success, error code otherwise
 * 
 * @pre           uart must be a valid handle from Hal_Uart_Init()
 * @post          UART peripheral is disabled, handle is invalid
 * 
 * @note          Safe to call with NULL handle (returns error)
 */
HalUartError_t Hal_Uart_Deinit(HalUart_t *uart);

/*----------------------------------------------------------------------------*/
/* Blocking I/O (Polling Mode)                                                */
/*----------------------------------------------------------------------------*/

/**
 * @brief Transmit data in blocking mode
 * 
 * Blocks until all data has been transmitted or timeout expires.
 * This function should NOT be called from an ISR.
 * 
 * @param uart      Handle to UART instance
 * @param data      Pointer to data buffer to transmit
 * @param length    Number of bytes to transmit
 * @param timeout_ms Maximum time to wait in milliseconds (0 = wait forever)
 * @return          Number of bytes actually transmitted, or negative error code
 * 
 * @pre             uart must be initialized
 * @pre             data must not be NULL if length > 0
 * 
 * @warning         Do not call from ISR context - use async functions instead
 */
int32_t Hal_Uart_WriteBlocking(HalUart_t *uart, const uint8_t *data, 
                                uint32_t length, uint32_t timeout_ms);

/**
 * @brief Receive data in blocking mode
 * 
 * Blocks until the requested number of bytes have been received or
 * timeout expires.
 * 
 * @param uart      Handle to UART instance
 * @param buffer    Pointer to buffer for received data
 * @param length    Maximum number of bytes to receive
 * @param timeout_ms Maximum time to wait in milliseconds (0 = wait forever)
 * @return          Number of bytes actually received, or negative error code
 * 
 * @pre             uart must be initialized
 * @pre             buffer must not be NULL if length > 0
 */
int32_t Hal_Uart_ReadBlocking(HalUart_t *uart, uint8_t *buffer, 
                               uint32_t length, uint32_t timeout_ms);

/*----------------------------------------------------------------------------*/
/* Non-Blocking I/O (Interrupt/DMA Mode)                                      */
/*----------------------------------------------------------------------------*/

/**
 * @brief Transmit data asynchronously
 * 
 * Starts an asynchronous transmission. The function returns immediately.
 * When transmission is complete, the HAL_UART_EVENT_TX_COMPLETE event
 * is generated (if callback is registered).
 * 
 * @param uart      Handle to UART instance
 * @param data      Pointer to data buffer to transmit
 * @param length    Number of bytes to transmit
 * @return          HAL_UART_OK if transmission started, error code otherwise
 * 
 * @pre             uart must be initialized with tx_buffer_size > 0
 * @pre             data buffer must remain valid until TX_COMPLETE event
 * 
 * @note            For DMA transfers, ensure buffer is in DMA-accessible memory
 */
HalUartError_t Hal_Uart_WriteAsync(HalUart_t *uart, const uint8_t *data, 
                                    uint32_t length);

/**
 * @brief Check if transmission is complete
 * 
 * @param uart    Handle to UART instance
 * @return        true if no transmission in progress, false otherwise
 */
bool Hal_Uart_IsTxDone(HalUart_t *uart);

/**
 * @brief Get number of bytes available in RX buffer
 * 
 * @param uart    Handle to UART instance
 * @return        Number of bytes available to read, or 0 if error/empty
 */
uint32_t Hal_Uart_GetRxAvailable(HalUart_t *uart);

/**
 * @brief Read data from RX buffer (non-blocking)
 * 
 * Reads up to 'length' bytes from the internal RX buffer without blocking.
 * 
 * @param uart      Handle to UART instance
 * @param buffer    Pointer to buffer for received data
 * @param length    Maximum number of bytes to read
 * @return          Number of bytes actually read (may be 0)
 * 
 * @note            This function is safe to call from ISR context
 */
uint32_t Hal_Uart_Read(HalUart_t *uart, uint8_t *buffer, uint32_t length);

/*----------------------------------------------------------------------------*/
/* Callback Registration                                                      */
/*----------------------------------------------------------------------------*/

/**
 * @brief Register a callback for UART events
 * 
 * The callback will be invoked from ISR context when events occur.
 * Keep the callback function short and fast.
 * 
 * @param uart      Handle to UART instance
 * @param callback  Callback function pointer (NULL to disable)
 * @param context   User context passed to callback (can be NULL)
 * @return          HAL_UART_OK on success, error code otherwise
 * 
 * @warning         Callback runs in ISR context - keep it short!
 * @warning         Do NOT call blocking functions from the callback
 */
HalUartError_t Hal_Uart_RegisterCallback(HalUart_t *uart, 
                                          HalUartCallback_t callback,
                                          void *context);

/*----------------------------------------------------------------------------*/
/* Status and Control                                                         */
/*----------------------------------------------------------------------------*/

/**
 * @brief Clear all error flags
 * 
 * @param uart    Handle to UART instance
 * @return        HAL_UART_OK on success, error code otherwise
 */
HalUartError_t Hal_Uart_ClearErrors(HalUart_t *uart);

/**
 * @brief Flush TX buffer
 * 
 * Waits for all pending TX data to be transmitted.
 * 
 * @param uart    Handle to UART instance
 * @return        HAL_UART_OK on success, error code otherwise
 */
HalUartError_t Hal_Uart_FlushTx(HalUart_t *uart);

/**
 * @brief Flush RX buffer
 * 
 * Discards all received data in the RX buffer.
 * 
 * @param uart    Handle to UART instance
 * @return        HAL_UART_OK on success, error code otherwise
 */
HalUartError_t Hal_Uart_FlushRx(HalUart_t *uart);

/**
 * @brief Get current baud rate
 * 
 * @param uart    Handle to UART instance
 * @return        Current baud rate, or 0 if error
 */
uint32_t Hal_Uart_GetBaudRate(HalUart_t *uart);

/**
 * @brief Set baud rate at runtime
 * 
 * Changes the baud rate without reinitializing the entire peripheral.
 * 
 * @param uart      Handle to UART instance
 * @param baud_rate New baud rate
 * @return          HAL_UART_OK on success, error code otherwise
 */
HalUartError_t Hal_Uart_SetBaudRate(HalUart_t *uart, HalUartBaudRate_t baud_rate);

/*============================================================================*/
/* INLINE HELPER FUNCTIONS                                                    */
/*============================================================================*/

/**
 * @brief Check if error code indicates success
 */
static inline bool Hal_Uart_IsOk(HalUartError_t err) {
    return (err == HAL_UART_OK);
}

/**
 * @brief Check if error code indicates a hardware error
 */
static inline bool Hal_Uart_IsHardwareError(HalUartError_t err) {
    return (err >= HAL_UART_ERROR_HARDWARE && err <= HAL_UART_ERROR_BREAK);
}

/**
 * @brief Get human-readable error string
 * 
 * @param err    Error code
 * @return       Pointer to static error string (do not free)
 */
const char* Hal_Uart_ErrorToString(HalUartError_t err);

#ifdef __cplusplus
}
#endif

#endif /* HAL_UART_H */
