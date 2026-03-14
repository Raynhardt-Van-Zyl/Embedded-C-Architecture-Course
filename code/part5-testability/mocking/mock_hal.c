/**
 * @file mock_hal.c
 * @brief Mock HAL Implementation for Unit Testing
 * 
 * This file implements mock hardware abstraction layer functions
 * for testing embedded code without real hardware.
 * 
 * Key Features:
 * - Stateful mock implementations
 * - Call history tracking
 * - Data capture and verification
 * - Configurable error injection
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 */

#include "mock_hal.h"
#include <string.h>

/*============================================================================*/
/*                          GLOBAL MOCK STATE                                  */
/*============================================================================*/

#if (MOCK_ENABLE_CALL_ORDER == 1)
uint32_t g_mock_global_call_order = 0;
#endif

/*============================================================================*/
/*                          PRIVATE HELPER FUNCTIONS                           */
/*============================================================================*/

/**
 * @brief Record a mock call
 * 
 * @param history Call history array
 * @param count Pointer to call counter
 * @param name Function name
 * @return MockCallRecord_t* Pointer to record, or NULL if full
 */
static MockCallRecord_t *record_call(MockCallRecord_t *history, 
                                      uint32_t *count,
                                      const char *name)
{
    if (*count >= MOCK_MAX_CALL_HISTORY) {
        return NULL;
    }
    
    MockCallRecord_t *record = &history[*count];
    memset(record, 0, sizeof(MockCallRecord_t));
    
    record->function_name = name;
    record->call_count = (*count) + 1;
    
#if (MOCK_ENABLE_CALL_ORDER == 1)
    record->global_order = ++g_mock_global_call_order;
#endif
    
    (*count)++;
    return record;
}

/**
 * @brief Copy buffer data to record
 * 
 * @param record Call record to populate
 * @param data Data to copy
 * @param len Length of data
 */
static void copy_buffer_to_record(MockCallRecord_t *record, 
                                   const uint8_t *data, 
                                   uint16_t len)
{
    if ((data != NULL) && (len > 0) && (len <= MOCK_MAX_BUFFER_SIZE)) {
        memcpy(record->buffer_data, data, len);
        record->buffer_size = len;
    }
}

/*============================================================================*/
/*                          UART MOCK IMPLEMENTATION                           */
/*============================================================================*/

/**
 * @brief Initialize UART mock
 */
void Mock_UART_Init(MockUART_t *mock)
{
    if (mock == NULL) {
        return;
    }
    
    memset(mock, 0, sizeof(MockUART_t));
    mock->config.capture_tx_data = true;
    mock->config.provide_rx_data = true;
}

/**
 * @brief Configure UART mock behavior
 */
void Mock_UART_Configure(MockUART_t *mock, const MockUART_Config_t *config)
{
    if ((mock == NULL) || (config == NULL)) {
        return;
    }
    
    mock->config = *config;
}

/**
 * @brief Queue data to be "received" by UART
 */
void Mock_UART_QueueRxData(MockUART_t *mock, const uint8_t *data, uint16_t len)
{
    if ((mock == NULL) || (data == NULL) || (len == 0)) {
        return;
    }
    
    uint16_t copy_len = (len > MOCK_MAX_BUFFER_SIZE) ? 
                         MOCK_MAX_BUFFER_SIZE : len;
    
    memcpy(mock->rx_buffer, data, copy_len);
    mock->rx_buffer_len = copy_len;
    mock->rx_buffer_pos = 0;
}

/**
 * @brief Get data that was "transmitted" by UART
 */
uint16_t Mock_UART_GetTxData(MockUART_t *mock, uint8_t *data, uint16_t max_len)
{
    if ((mock == NULL) || (data == NULL) || (max_len == 0)) {
        return 0;
    }
    
    uint16_t copy_len = (mock->tx_buffer_len > max_len) ? 
                         max_len : mock->tx_buffer_len;
    
    memcpy(data, mock->tx_buffer, copy_len);
    return copy_len;
}

/**
 * @brief Mock UART transmit function
 */
int32_t Mock_UART_Transmit(MockUART_t *mock, const uint8_t *data, uint16_t len)
{
    if (mock == NULL) {
        return MOCK_ERROR_NOT_EXPECTED;
    }
    
    MockCallRecord_t *record = record_call(mock->tx_history, 
                                            &mock->tx_count, 
                                            "UART_Transmit");
    if (record == NULL) {
        return MOCK_ERROR_BUFFER_FULL;
    }
    
    record->params[0] = (uint32_t)data;
    record->params[1] = len;
    
    if ((mock->config.capture_tx_data) && (data != NULL) && (len > 0)) {
        copy_buffer_to_record(record, data, len);
        
        uint16_t remaining = MOCK_MAX_BUFFER_SIZE - mock->tx_buffer_len;
        if (len <= remaining) {
            memcpy(&mock->tx_buffer[mock->tx_buffer_len], data, len);
            mock->tx_buffer_len += len;
        }
    }
    
    if (mock->config.return_error) {
        mock->config.return_error = false;
        return (int32_t)mock->config.error_code;
    }
    
    return 0;
}

/**
 * @brief Mock UART receive function
 */
int32_t Mock_UART_Receive(MockUART_t *mock, uint8_t *data, uint16_t max_len)
{
    if (mock == NULL) {
        return MOCK_ERROR_NOT_EXPECTED;
    }
    
    MockCallRecord_t *record = record_call(mock->rx_history, 
                                            &mock->rx_count, 
                                            "UART_Receive");
    if (record == NULL) {
        return MOCK_ERROR_BUFFER_FULL;
    }
    
    record->params[0] = (uint32_t)data;
    record->params[1] = max_len;
    
    if (mock->config.return_error) {
        mock->config.return_error = false;
        return (int32_t)mock->config.error_code;
    }
    
    if (!mock->config.provide_rx_data) {
        return 0;
    }
    
    uint16_t available = mock->rx_buffer_len - mock->rx_buffer_pos;
    if (available == 0) {
        return MOCK_ERROR_NO_DATA;
    }
    
    uint16_t copy_len = (available > max_len) ? max_len : available;
    
    if (data != NULL) {
        memcpy(data, &mock->rx_buffer[mock->rx_buffer_pos], copy_len);
        mock->rx_buffer_pos += copy_len;
    }
    
    copy_buffer_to_record(record, data, copy_len);
    
    return (int32_t)copy_len;
}

/**
 * @brief Verify UART transmit was called with specific data
 */
bool Mock_UART_VerifyTxData(MockUART_t *mock, const uint8_t *expected, uint16_t len)
{
    if ((mock == NULL) || (expected == NULL) || (len == 0)) {
        return false;
    }
    
    for (uint32_t i = 0; i < mock->tx_count; i++) {
        MockCallRecord_t *record = &mock->tx_history[i];
        
        if ((record->buffer_size == len) &&
            (memcmp(record->buffer_data, expected, len) == 0)) {
            return true;
        }
    }
    
    return false;
}

/*============================================================================*/
/*                          GPIO MOCK IMPLEMENTATION                           */
/*============================================================================*/

/**
 * @brief Initialize GPIO mock
 */
void Mock_GPIO_Init(MockGPIO_t *mock)
{
    if (mock == NULL) {
        return;
    }
    
    memset(mock, 0, sizeof(MockGPIO_t));
}

/**
 * @brief Mock GPIO initialization
 */
int32_t Mock_GPIO_InitPin(MockGPIO_t *mock, uint8_t pin, MockGPIO_Mode_t mode)
{
    if ((mock == NULL) || (pin >= 32)) {
        return MOCK_ERROR_NOT_EXPECTED;
    }
    
    MockCallRecord_t *record = record_call(mock->history, 
                                            &mock->call_count, 
                                            "GPIO_InitPin");
    if (record == NULL) {
        return MOCK_ERROR_BUFFER_FULL;
    }
    
    record->params[0] = pin;
    record->params[1] = (uint32_t)mode;
    
    mock->pins[pin].mode = mode;
    mock->pins[pin].initialized = true;
    
    return 0;
}

/**
 * @brief Mock GPIO write
 */
int32_t Mock_GPIO_Write(MockGPIO_t *mock, uint8_t pin, MockGPIO_State_t state)
{
    if ((mock == NULL) || (pin >= 32)) {
        return MOCK_ERROR_NOT_EXPECTED;
    }
    
    MockCallRecord_t *record = record_call(mock->history, 
                                            &mock->call_count, 
                                            "GPIO_Write");
    if (record == NULL) {
        return MOCK_ERROR_BUFFER_FULL;
    }
    
    record->params[0] = pin;
    record->params[1] = (uint32_t)state;
    
    mock->pins[pin].state = state;
    
    return 0;
}

/**
 * @brief Mock GPIO read
 */
MockGPIO_State_t Mock_GPIO_Read(MockGPIO_t *mock, uint8_t pin)
{
    if ((mock == NULL) || (pin >= 32)) {
        return MOCK_GPIO_STATE_LOW;
    }
    
    MockCallRecord_t *record = record_call(mock->history, 
                                            &mock->call_count, 
                                            "GPIO_Read");
    if (record != NULL) {
        record->params[0] = pin;
        record->params[1] = mock->pins[pin].state;
    }
    
    return mock->pins[pin].state;
}

/**
 * @brief Set GPIO pin state for read simulation
 */
void Mock_GPIO_SetInputState(MockGPIO_t *mock, uint8_t pin, MockGPIO_State_t state)
{
    if ((mock == NULL) || (pin >= 32)) {
        return;
    }
    
    mock->pins[pin].state = state;
    mock->pins[pin].mode = MOCK_GPIO_MODE_INPUT;
}

/**
 * @brief Verify GPIO write was called with specific state
 */
bool Mock_GPIO_VerifyWrite(MockGPIO_t *mock, uint8_t pin, MockGPIO_State_t expected_state)
{
    if ((mock == NULL) || (pin >= 32)) {
        return false;
    }
    
    for (uint32_t i = 0; i < mock->call_count; i++) {
        MockCallRecord_t *record = &mock->history[i];
        
        if ((record->params[0] == pin) && 
            (record->params[1] == (uint32_t)expected_state)) {
            return true;
        }
    }
    
    return false;
}

/*============================================================================*/
/*                          TIMER MOCK IMPLEMENTATION                          */
/*============================================================================*/

/**
 * @brief Initialize timer mock
 */
void Mock_Timer_Init(MockTimer_t *mock)
{
    if (mock == NULL) {
        return;
    }
    
    memset(mock, 0, sizeof(MockTimer_t));
    mock->period = 0xFFFFFFFF;
}

/**
 * @brief Mock timer start
 */
int32_t Mock_Timer_Start(MockTimer_t *mock)
{
    if (mock == NULL) {
        return MOCK_ERROR_NOT_EXPECTED;
    }
    
    MockCallRecord_t *record = record_call(mock->history, 
                                            &mock->call_count, 
                                            "Timer_Start");
    if (record == NULL) {
        return MOCK_ERROR_BUFFER_FULL;
    }
    
    mock->running = true;
    
    return 0;
}

/**
 * @brief Mock timer stop
 */
int32_t Mock_Timer_Stop(MockTimer_t *mock)
{
    if (mock == NULL) {
        return MOCK_ERROR_NOT_EXPECTED;
    }
    
    MockCallRecord_t *record = record_call(mock->history, 
                                            &mock->call_count, 
                                            "Timer_Stop");
    if (record == NULL) {
        return MOCK_ERROR_BUFFER_FULL;
    }
    
    mock->running = false;
    
    return 0;
}

/**
 * @brief Mock timer get tick
 */
uint32_t Mock_Timer_GetTick(MockTimer_t *mock)
{
    if (mock == NULL) {
        return 0;
    }
    
    MockCallRecord_t *record = record_call(mock->history, 
                                            &mock->call_count, 
                                            "Timer_GetTick");
    if (record != NULL) {
        record->params[0] = mock->current_tick;
    }
    
    return mock->current_tick;
}

/**
 * @brief Advance simulated time
 */
void Mock_Timer_Advance(MockTimer_t *mock, uint32_t ticks)
{
    if (mock == NULL) {
        return;
    }
    
    uint32_t new_tick = mock->current_tick + ticks;
    
    if (new_tick >= mock->period) {
        mock->overflow = true;
        new_tick %= mock->period;
    }
    
    mock->current_tick = new_tick;
}

/**
 * @brief Set timer period for overflow simulation
 */
void Mock_Timer_SetPeriod(MockTimer_t *mock, uint32_t period)
{
    if (mock == NULL) {
        return;
    }
    
    mock->period = period;
}

/*============================================================================*/
/*                          SPI MOCK IMPLEMENTATION                            */
/*============================================================================*/

/**
 * @brief Initialize SPI mock
 */
void Mock_SPI_Init(MockSPI_t *mock)
{
    if (mock == NULL) {
        return;
    }
    
    memset(mock, 0, sizeof(MockSPI_t));
}

/**
 * @brief Queue data to be received during SPI transfer
 */
void Mock_SPI_QueueRxData(MockSPI_t *mock, const uint8_t *data, uint16_t len)
{
    if ((mock == NULL) || (data == NULL) || (len == 0)) {
        return;
    }
    
    uint16_t copy_len = (len > MOCK_MAX_BUFFER_SIZE) ? 
                         MOCK_MAX_BUFFER_SIZE : len;
    
    memcpy(mock->rx_buffer, data, copy_len);
    mock->rx_buffer_len = copy_len;
    mock->rx_buffer_pos = 0;
}

/**
 * @brief Mock SPI transfer (simultaneous TX/RX)
 */
int32_t Mock_SPI_Transfer(MockSPI_t *mock, const uint8_t *tx_data, 
                          uint8_t *rx_data, uint16_t len)
{
    if (mock == NULL) {
        return MOCK_ERROR_NOT_EXPECTED;
    }
    
    MockCallRecord_t *record = record_call(mock->transfer_history, 
                                            &mock->transfer_count, 
                                            "SPI_Transfer");
    if (record == NULL) {
        return MOCK_ERROR_BUFFER_FULL;
    }
    
    record->params[0] = len;
    
    if ((tx_data != NULL) && (len > 0)) {
        copy_buffer_to_record(record, tx_data, len);
        
        uint16_t remaining = MOCK_MAX_BUFFER_SIZE - mock->tx_buffer_len;
        if (len <= remaining) {
            memcpy(&mock->tx_buffer[mock->tx_buffer_len], tx_data, len);
            mock->tx_buffer_len += len;
        }
    }
    
    if (mock->return_error) {
        mock->return_error = false;
        return -1;
    }
    
    if ((rx_data != NULL) && (len > 0)) {
        uint16_t available = mock->rx_buffer_len - mock->rx_buffer_pos;
        uint16_t copy_len = (len > available) ? available : len;
        
        if (copy_len > 0) {
            memcpy(rx_data, &mock->rx_buffer[mock->rx_buffer_pos], copy_len);
            mock->rx_buffer_pos += copy_len;
        }
    }
    
    return 0;
}

/**
 * @brief Get data transmitted in SPI transfers
 */
uint16_t Mock_SPI_GetTxData(MockSPI_t *mock, uint8_t *data, uint16_t max_len)
{
    if ((mock == NULL) || (data == NULL) || (max_len == 0)) {
        return 0;
    }
    
    uint16_t copy_len = (mock->tx_buffer_len > max_len) ? 
                         max_len : mock->tx_buffer_len;
    
    memcpy(data, mock->tx_buffer, copy_len);
    return copy_len;
}

/*============================================================================*/
/*                          ADC MOCK IMPLEMENTATION                            */
/*============================================================================*/

/**
 * @brief Initialize ADC mock
 */
void Mock_ADC_Init(MockADC_t *mock)
{
    if (mock == NULL) {
        return;
    }
    
    memset(mock, 0, sizeof(MockADC_t));
}

/**
 * @brief Set simulated ADC channel value
 */
void Mock_ADC_SetChannelValue(MockADC_t *mock, uint8_t channel, uint16_t value)
{
    if ((mock == NULL) || (channel >= 16)) {
        return;
    }
    
    mock->channels[channel] = value;
    mock->channel_valid[channel] = true;
}

/**
 * @brief Mock ADC read
 */
int32_t Mock_ADC_Read(MockADC_t *mock, uint8_t channel, uint16_t *value)
{
    if ((mock == NULL) || (channel >= 16) || (value == NULL)) {
        return MOCK_ERROR_NOT_EXPECTED;
    }
    
    MockCallRecord_t *record = record_call(mock->history, 
                                            &mock->call_count, 
                                            "ADC_Read");
    if (record == NULL) {
        return MOCK_ERROR_BUFFER_FULL;
    }
    
    record->params[0] = channel;
    
    if (mock->return_error) {
        mock->return_error = false;
        return -1;
    }
    
    if (!mock->channel_valid[channel]) {
        return MOCK_ERROR_NO_DATA;
    }
    
    *value = mock->channels[channel];
    record->params[1] = *value;
    
    return 0;
}

/*============================================================================*/
/*                          GLOBAL MOCK UTILITIES                              */
/*============================================================================*/

/**
 * @brief Reset global mock state
 */
void Mock_GlobalReset(void)
{
#if (MOCK_ENABLE_CALL_ORDER == 1)
    g_mock_global_call_order = 0;
#endif
}

/**
 * @brief Get current global call order
 */
uint32_t Mock_GetGlobalCallOrder(void)
{
#if (MOCK_ENABLE_CALL_ORDER == 1)
    return g_mock_global_call_order;
#else
    return 0;
#endif
}

/**
 * @brief Verify call order matches expected sequence
 */
bool Mock_VerifyCallOrder(uint32_t actual, uint32_t expected)
{
    return (actual == expected);
}

/*============================================================================*/
/*                          EXPECTATION-BASED TESTING                          */
/*============================================================================*/

/**
 * @brief Storage for expected return value
 */
static int32_t s_expected_return_value = 0;
static bool s_has_expected_return = false;

/**
 * @brief Set expected return value for next mock call
 */
void Mock_SetExpectedReturnValue(void *mock, int32_t value)
{
    (void)mock;
    s_expected_return_value = value;
    s_has_expected_return = true;
}

/**
 * @brief Verify all expectations were met
 */
bool Mock_VerifyAllExpectations(void)
{
    bool result = !s_has_expected_return;
    s_has_expected_return = false;
    s_expected_return_value = 0;
    return result;
}

/*============================================================================*/
/*                          USAGE EXAMPLES                                     */
/*============================================================================*/

/*
 * Example test using mock HAL:
 * 
 * void test_uart_transmit_sends_correct_data(void)
 * {
 *     MockUART_t uart_mock;
 *     Mock_UART_Init(&uart_mock);
 *     
 *     uint8_t expected[] = {0x01, 0x02, 0x03};
 *     
 *     MyModule_SendCommand(0x01, 0x02, 0x03);
 *     
 *     TEST_ASSERT_TRUE(Mock_UART_VerifyTxData(&uart_mock, expected, 3));
 * }
 * 
 * void test_gpio_blinks_led(void)
 * {
 *     MockGPIO_t gpio_mock;
 *     Mock_GPIO_Init(&gpio_mock);
 *     
 *     MyModule_TurnOnLED();
 *     TEST_ASSERT_TRUE(Mock_GPIO_VerifyWrite(&gpio_mock, LED_PIN, MOCK_GPIO_STATE_HIGH));
 *     
 *     MyModule_TurnOffLED();
 *     TEST_ASSERT_TRUE(Mock_GPIO_VerifyWrite(&gpio_mock, LED_PIN, MOCK_GPIO_STATE_LOW));
 * }
 * 
 * void test_timer_timeout(void)
 * {
 *     MockTimer_t timer_mock;
 *     Mock_Timer_Init(&timer_mock);
 *     Mock_Timer_SetPeriod(&timer_mock, 1000);
 *     
 *     MyModule_StartTimeout();
 *     TEST_ASSERT_TRUE(timer_mock.running);
 *     
 *     Mock_Timer_Advance(&timer_mock, 1000);
 *     MyModule_CheckTimeout();
 *     TEST_ASSERT_TRUE(timeout_occurred);
 * }
 */
