/**
 * @file mock_hal.h
 * @brief Mock HAL (Hardware Abstraction Layer) for Unit Testing
 * 
 * This module provides a mock implementation of hardware abstraction
 * functions for unit testing embedded code. It enables:
 * 
 * - Testing without real hardware
 * - Controlled stimulus/response testing
 * - Verification of hardware interactions
 * - Fault injection for error path testing
 * 
 * Design Principles:
 * - Mocks track all calls with parameters for verification
 * - Return values can be configured per-test
 * - Call history enables behavioral verification
 * - Minimal dependencies for easy test integration
 * 
 * Compatible with Unity/CMock testing frameworks but also works
 * standalone for simple test setups.
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#ifndef MOCK_HalH
#define MOCK_HalH

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*============================================================================*/
/*                          CONFIGURATION CONSTANTS                           */
/*============================================================================*/

/**
 * @brief Maximum number of calls to track per mock function
 * 
 * Adjust based on your testing needs. Larger values consume more
 * memory but allow testing longer sequences.
 */
#ifndef MOCK_MAX_CALL_HISTORY
#define MOCK_MAX_CALL_HISTORY       (32U)
#endif

/**
 * @brief Maximum data buffer size for mock I/O operations
 */
#ifndef MOCK_MAX_BUFFER_SIZE
#define MOCK_MAX_BUFFER_SIZE        (256U)
#endif

/**
 * @brief Enable/disable mock call order verification
 * 
 * When enabled, tracks the global call order across all mocks.
 */
#ifndef MOCK_ENABLE_CALL_ORDER
#define MOCK_ENABLE_CALL_ORDER      (1)
#endif

/*============================================================================*/
/*                          MOCK STATUS AND ERROR CODES                        */
/*============================================================================*/

/**
 * @brief Mock operation result codes
 */
typedef enum {
    MOCK_OK                     = 0,    /**< Operation successful */
    MOCK_ERROR_BUFFER_FULL      = -1,   /**< Call history buffer full */
    MOCK_ERROR_NO_DATA          = -2,   /**< No data available for read */
    MOCK_ERROR_NOT_EXPECTED     = -3,   /**< Call was not expected */
    MOCK_ERROR_PARAM_MISMATCH   = -4,   /**< Parameter value mismatch */
    MOCK_ERROR_ORDER_MISMATCH   = -5,   /**< Call order mismatch */
} MockResult_t;

/*============================================================================*/
/*                          UNIVERSAL MOCK CALL RECORD                          */
/*============================================================================*/

/**
 * @brief Universal mock call record structure
 * 
 * Stores information about a single mock function call. The structure
 * is designed to accommodate various function signatures.
 */
typedef struct MockCallRecord {
    const char             *function_name;      /**< Name of mocked function */
    uint32_t                call_count;         /**< Call count for this function */
    
    /**
     * @brief Generic parameter storage
     * 
     * Stores up to 8 generic parameters. Interpretation depends on
     * the specific mock function being called.
     */
    uint32_t                params[8];
    
    /**
     * @brief Pointer parameter storage
     * 
     * For functions that take pointer parameters, stores copies
     * of the data pointed to (up to MOCK_MAX_BUFFER_SIZE bytes).
     */
    uint8_t                 buffer_data[MOCK_MAX_BUFFER_SIZE];
    uint16_t                buffer_size;
    
#if (MOCK_ENABLE_CALL_ORDER == 1)
    uint32_t                global_order;       /**< Global call sequence number */
#endif
    
    struct MockCallRecord  *next;               /**< For linked list implementations */
    
} MockCallRecord_t;

/*============================================================================*/
/*                          UART MOCK INTERFACE                                */
/*============================================================================*/

/**
 * @brief UART mock configuration structure
 */
typedef struct {
    bool                    return_error;       /**< Return error on next call */
    uint32_t                error_code;         /**< Error code to return */
    bool                    capture_tx_data;    /**< Capture transmitted data */
    bool                    provide_rx_data;    /**< Provide fake receive data */
} MockUART_Config_t;

/**
 * @brief UART mock state structure
 */
typedef struct {
    MockCallRecord_t        tx_history[MOCK_MAX_CALL_HISTORY];
    MockCallRecord_t        rx_history[MOCK_MAX_CALL_HISTORY];
    uint32_t                tx_count;
    uint32_t                rx_count;
    
    uint8_t                 tx_buffer[MOCK_MAX_BUFFER_SIZE];
    uint16_t                tx_buffer_len;
    
    uint8_t                 rx_buffer[MOCK_MAX_BUFFER_SIZE];
    uint16_t                rx_buffer_len;
    uint16_t                rx_buffer_pos;
    
    MockUART_Config_t       config;
    
} MockUART_t;

/**
 * @brief Initialize UART mock
 * 
 * @param mock Pointer to mock state structure
 */
void Mock_UART_Init(MockUART_t *mock);

/**
 * @brief Configure UART mock behavior
 * 
 * @param mock Pointer to mock state structure
 * @param config Configuration settings
 */
void Mock_UART_Configure(MockUART_t *mock, const MockUART_Config_t *config);

/**
 * @brief Queue data to be "received" by UART
 * 
 * Use this to simulate incoming data for read operations.
 * 
 * @param mock Pointer to mock state structure
 * @param data Data to queue
 * @param len Length of data
 */
void Mock_UART_QueueRxData(MockUART_t *mock, const uint8_t *data, uint16_t len);

/**
 * @brief Get data that was "transmitted" by UART
 * 
 * Use this to verify what your code sent.
 * 
 * @param mock Pointer to mock state structure
 * @param data Buffer to receive transmitted data
 * @param max_len Maximum bytes to copy
 * @return uint16_t Actual bytes copied
 */
uint16_t Mock_UART_GetTxData(MockUART_t *mock, uint8_t *data, uint16_t max_len);

/**
 * @brief Mock UART transmit function
 * 
 * Records the call and data for later verification.
 * 
 * @param mock Pointer to mock state structure
 * @param data Data to transmit
 * @param len Length of data
 * @return int32_t 0 on success, negative on error
 */
int32_t Mock_UART_Transmit(MockUART_t *mock, const uint8_t *data, uint16_t len);

/**
 * @brief Mock UART receive function
 * 
 * Returns queued data or simulates timeout/error.
 * 
 * @param mock Pointer to mock state structure
 * @param data Buffer for received data
 * @param max_len Maximum bytes to receive
 * @return int32_t Bytes received, or negative on error
 */
int32_t Mock_UART_Receive(MockUART_t *mock, uint8_t *data, uint16_t max_len);

/**
 * @brief Verify UART transmit was called with specific data
 * 
 * @param mock Pointer to mock state structure
 * @param expected Expected data
 * @param len Length of expected data
 * @return bool true if match, false otherwise
 */
bool Mock_UART_VerifyTxData(MockUART_t *mock, const uint8_t *expected, uint16_t len);

/*============================================================================*/
/*                          GPIO MOCK INTERFACE                                */
/*============================================================================*/

/**
 * @brief GPIO pin mode enumeration
 */
typedef enum {
    MOCK_GPIO_MODE_INPUT,
    MOCK_GPIO_MODE_OUTPUT,
    MOCK_GPIO_MODE_ALTERNATE,
    MOCK_GPIO_MODE_ANALOG,
} MockGPIO_Mode_t;

/**
 * @brief GPIO pin state enumeration
 */
typedef enum {
    MOCK_GPIO_STATE_LOW  = 0,
    MOCK_GPIO_STATE_HIGH = 1,
} MockGPIO_State_t;

/**
 * @brief GPIO pin mock state structure
 */
typedef struct {
    MockGPIO_Mode_t         mode;
    MockGPIO_State_t        state;
    bool                    initialized;
} MockGPIO_Pin_t;

/**
 * @brief GPIO mock state structure
 */
typedef struct {
    MockGPIO_Pin_t          pins[32];           /**< Up to 32 pins per port */
    MockCallRecord_t        history[MOCK_MAX_CALL_HISTORY];
    uint32_t                call_count;
    
} MockGPIO_t;

/**
 * @brief Initialize GPIO mock
 * 
 * @param mock Pointer to mock state structure
 */
void Mock_GPIO_Init(MockGPIO_t *mock);

/**
 * @brief Mock GPIO initialization
 * 
 * @param mock Pointer to mock state structure
 * @param pin Pin number
 * @param mode Pin mode
 * @return int32_t 0 on success
 */
int32_t Mock_GPIO_InitPin(MockGPIO_t *mock, uint8_t pin, MockGPIO_Mode_t mode);

/**
 * @brief Mock GPIO write
 * 
 * @param mock Pointer to mock state structure
 * @param pin Pin number
 * @param state State to write
 * @return int32_t 0 on success
 */
int32_t Mock_GPIO_Write(MockGPIO_t *mock, uint8_t pin, MockGPIO_State_t state);

/**
 * @brief Mock GPIO read
 * 
 * @param mock Pointer to mock state structure
 * @param pin Pin number
 * @return MockGPIO_State_t Pin state
 */
MockGPIO_State_t Mock_GPIO_Read(MockGPIO_t *mock, uint8_t pin);

/**
 * @brief Set GPIO pin state for read simulation
 * 
 * Use this to set what value will be read by your code.
 * 
 * @param mock Pointer to mock state structure
 * @param pin Pin number
 * @param state State to return on read
 */
void Mock_GPIO_SetInputState(MockGPIO_t *mock, uint8_t pin, MockGPIO_State_t state);

/**
 * @brief Verify GPIO write was called with specific state
 * 
 * @param mock Pointer to mock state structure
 * @param pin Pin number
 * @param expected_state Expected state
 * @return bool true if match found in history
 */
bool Mock_GPIO_VerifyWrite(MockGPIO_t *mock, uint8_t pin, MockGPIO_State_t expected_state);

/*============================================================================*/
/*                          TIMER MOCK INTERFACE                               */
/*============================================================================*/

/**
 * @brief Timer mock state structure
 */
typedef struct {
    uint32_t                current_tick;       /**< Simulated tick count */
    uint32_t                period;             /**< Timer period */
    bool                    running;            /**< Timer running state */
    bool                    overflow;           /**< Overflow flag */
    
    MockCallRecord_t        history[MOCK_MAX_CALL_HISTORY];
    uint32_t                call_count;
    
} MockTimer_t;

/**
 * @brief Initialize timer mock
 * 
 * @param mock Pointer to mock state structure
 */
void Mock_Timer_Init(MockTimer_t *mock);

/**
 * @brief Mock timer start
 * 
 * @param mock Pointer to mock state structure
 * @return int32_t 0 on success
 */
int32_t Mock_Timer_Start(MockTimer_t *mock);

/**
 * @brief Mock timer stop
 * 
 * @param mock Pointer to mock state structure
 * @return int32_t 0 on success
 */
int32_t Mock_Timer_Stop(MockTimer_t *mock);

/**
 * @brief Mock timer get tick
 * 
 * @param mock Pointer to mock state structure
 * @return uint32_t Current tick count
 */
uint32_t Mock_Timer_GetTick(MockTimer_t *mock);

/**
 * @brief Advance simulated time
 * 
 * Use this to simulate time passing in your tests.
 * 
 * @param mock Pointer to mock state structure
 * @param ticks Number of ticks to advance
 */
void Mock_Timer_Advance(MockTimer_t *mock, uint32_t ticks);

/**
 * @brief Set timer period for overflow simulation
 * 
 * @param mock Pointer to mock state structure
 * @param period Period in ticks
 */
void Mock_Timer_SetPeriod(MockTimer_t *mock, uint32_t period);

/*============================================================================*/
/*                          SPI MOCK INTERFACE                                 */
/*============================================================================*/

/**
 * @brief SPI mock state structure
 */
typedef struct {
    uint8_t                 tx_buffer[MOCK_MAX_BUFFER_SIZE];
    uint16_t                tx_buffer_len;
    
    uint8_t                 rx_buffer[MOCK_MAX_BUFFER_SIZE];
    uint16_t                rx_buffer_len;
    uint16_t                rx_buffer_pos;
    
    bool                    return_error;
    
    MockCallRecord_t        transfer_history[MOCK_MAX_CALL_HISTORY];
    uint32_t                transfer_count;
    
} MockSPI_t;

/**
 * @brief Initialize SPI mock
 * 
 * @param mock Pointer to mock state structure
 */
void Mock_SPI_Init(MockSPI_t *mock);

/**
 * @brief Queue data to be received during SPI transfer
 * 
 * @param mock Pointer to mock state structure
 * @param data Data to queue
 * @param len Length of data
 */
void Mock_SPI_QueueRxData(MockSPI_t *mock, const uint8_t *data, uint16_t len);

/**
 * @brief Mock SPI transfer (simultaneous TX/RX)
 * 
 * @param mock Pointer to mock state structure
 * @param tx_data Data to transmit
 * @param rx_data Buffer for received data
 * @param len Transfer length
 * @return int32_t 0 on success, negative on error
 */
int32_t Mock_SPI_Transfer(MockSPI_t *mock, const uint8_t *tx_data, 
                          uint8_t *rx_data, uint16_t len);

/**
 * @brief Get data transmitted in SPI transfers
 * 
 * @param mock Pointer to mock state structure
 * @param data Buffer to receive data
 * @param max_len Maximum bytes to copy
 * @return uint16_t Actual bytes copied
 */
uint16_t Mock_SPI_GetTxData(MockSPI_t *mock, uint8_t *data, uint16_t max_len);

/*============================================================================*/
/*                          ADC MOCK INTERFACE                                 */
/*============================================================================*/

/**
 * @brief ADC mock state structure
 */
typedef struct {
    uint16_t                channels[16];       /**< Simulated channel values */
    bool                    channel_valid[16];  /**< Channel has valid data */
    bool                    return_error;
    
    MockCallRecord_t        history[MOCK_MAX_CALL_HISTORY];
    uint32_t                call_count;
    
} MockADC_t;

/**
 * @brief Initialize ADC mock
 * 
 * @param mock Pointer to mock state structure
 */
void Mock_ADC_Init(MockADC_t *mock);

/**
 * @brief Set simulated ADC channel value
 * 
 * @param mock Pointer to mock state structure
 * @param channel Channel number (0-15)
 * @param value ADC value (0-4095 for 12-bit)
 */
void Mock_ADC_SetChannelValue(MockADC_t *mock, uint8_t channel, uint16_t value);

/**
 * @brief Mock ADC read
 * 
 * @param mock Pointer to mock state structure
 * @param channel Channel to read
 * @param value Pointer to receive value
 * @return int32_t 0 on success, negative on error
 */
int32_t Mock_ADC_Read(MockADC_t *mock, uint8_t channel, uint16_t *value);

/*============================================================================*/
/*                          GLOBAL MOCK UTILITIES                              */
/*============================================================================*/

/**
 * @brief Global call sequence counter
 * 
 * Used for call order verification across different mock types.
 */
#if (MOCK_ENABLE_CALL_ORDER == 1)
extern uint32_t g_mock_global_call_order;
#endif

/**
 * @brief Reset global mock state
 * 
 * Call this between tests to ensure clean state.
 */
void Mock_GlobalReset(void);

/**
 * @brief Get current global call order
 * 
 * @return uint32_t Current call sequence number
 */
uint32_t Mock_GetGlobalCallOrder(void);

/**
 * @brief Verify call order matches expected sequence
 * 
 * @param actual Actual call order from record
 * @param expected Expected order
 * @return bool true if match
 */
bool Mock_VerifyCallOrder(uint32_t actual, uint32_t expected);

/*============================================================================*/
/*                          EXPECTATION-BASED TESTING                          */
/*============================================================================*/

/**
 * @brief Expectation types for mock behavior
 */
typedef enum {
    MOCK_EXPECT_RETURN_VALUE,       /**< Expect specific return value */
    MOCK_EXPECT_CALL_COUNT,         /**< Expect specific call count */
    MOCK_EXPECT_PARAM,              /**< Expect specific parameter value */
    MOCK_EXPECT_BUFFER,             /**< Expect specific buffer content */
} MockExpectationType_t;

/**
 * @brief Set expected return value for next mock call
 * 
 * @param mock Pointer to mock state (generic)
 * @param value Value to return
 */
void Mock_SetExpectedReturnValue(void *mock, int32_t value);

/**
 * @brief Verify all expectations were met
 * 
 * @return bool true if all expectations satisfied
 */
bool Mock_VerifyAllExpectations(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_HalH */
