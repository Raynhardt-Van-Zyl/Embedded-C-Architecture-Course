/**
 * @file bsp_config.c
 * @brief Board Support Package Implementation
 * 
 * @details Implements the board-specific initialization and control functions.
 * This file adapts the generic HAL to the specific hardware configuration.
 * 
 * Implementation Notes:
 * - All board-specific initializations go here
 * - Pin configurations are defined using BSP header macros
 * - Peripheral clocks are enabled as needed
 * - External devices are initialized after GPIO setup
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#include "bsp_config.h"
#include <string.h>

/*============================================================================*/
/*                              PRIVATE VARIABLES                             */
/*============================================================================*/

/** @brief LED configuration table */
static const BSP_LED_Config_t s_ledConfig[BSP_LED_COUNT] = {
    [BSP_LED_1] = { BSP_LED_1_PORT, BSP_LED_1_PIN, BSP_LED_1_ACTIVE_HIGH },
    [BSP_LED_2] = { BSP_LED_2_PORT, BSP_LED_2_PIN, BSP_LED_2_ACTIVE_HIGH },
    [BSP_LED_3] = { BSP_LED_3_PORT, BSP_LED_3_PIN, BSP_LED_3_ACTIVE_HIGH },
    [BSP_LED_4] = { BSP_LED_4_PORT, BSP_LED_4_PIN, BSP_LED_4_ACTIVE_HIGH }
};

/** @brief Button configuration table */
static const BSP_Button_Config_t s_buttonConfig[BSP_BUTTON_COUNT] = {
    [BSP_BUTTON_1] = { BSP_BUTTON_1_PORT, BSP_BUTTON_1_PIN, BSP_BUTTON_1_ACTIVE_HIGH }
};

/** @brief BSP initialization flag */
static bool s_bspInitialized = false;

/*============================================================================*/
/*                              PRIVATE FUNCTIONS                             */
/*============================================================================*/

/**
 * @brief Initialize GPIO clocks
 * 
 * @details Enables clock for all GPIO ports used on the board.
 */
static void BSP_InitGPIOClocks(void)
{
}

/**
 * @brief Initialize LED GPIO pins
 * 
 * @details Configures all LED pins as outputs with initial state OFF.
 */
static void BSP_InitLEDs(void)
{
    for (uint8_t i = 0; i < BSP_LED_COUNT; i++) {
        HAL_GPIO_Config_t gpioConfig = {
            .port = s_ledConfig[i].port,
            .pin = s_ledConfig[i].pin,
            .mode = HAL_GPIO_MODE_OUTPUT_PP,
            .pull = HAL_GPIO_NOPULL,
            .speed = HAL_GPIO_SPEED_LOW,
            .alternate = 0
        };
        
        HAL_GPIO_Init(&gpioConfig);
        
        if (s_ledConfig[i].activeHigh) {
            HAL_GPIO_Write(s_ledConfig[i].port, s_ledConfig[i].pin, false);
        } else {
            HAL_GPIO_Write(s_ledConfig[i].port, s_ledConfig[i].pin, true);
        }
    }
}

/**
 * @brief Initialize button GPIO pins
 * 
 * @details Configures all button pins as inputs with appropriate pull resistors.
 */
static void BSP_InitButtons(void)
{
    for (uint8_t i = 0; i < BSP_BUTTON_COUNT; i++) {
        HAL_GPIO_Config_t gpioConfig = {
            .port = s_buttonConfig[i].port,
            .pin = s_buttonConfig[i].pin,
            .mode = HAL_GPIO_MODE_INPUT,
            .pull = s_buttonConfig[i].activeHigh ? HAL_GPIO_PULLDOWN : HAL_GPIO_PULLUP,
            .speed = HAL_GPIO_SPEED_LOW,
            .alternate = 0
        };
        
        HAL_GPIO_Init(&gpioConfig);
    }
}

/**
 * @brief Initialize UART peripherals
 * 
 * @details Configures all UART peripherals with their pin assignments.
 */
static void BSP_InitUARTs(void)
{
    HAL_UART_Config_t uartConfig = {
        .instance = BSP_UART_DEBUG_INSTANCE,
        .baudRate = BSP_UART_DEBUG_BAUDRATE,
        .wordLength = 8,
        .parity = HAL_UART_PARITY_NONE,
        .stopBits = HAL_UART_STOPBITS_1,
        .hwFlowControl = false,
        .oversampling = false
    };
    
    HAL_UART_Init(&uartConfig);
}

/**
 * @brief Initialize I2C peripherals
 * 
 * @details Configures I2C buses for sensors and EEPROM.
 */
static void BSP_InitI2Cs(void)
{
    HAL_I2C_Config_t i2cConfig = {
        .instance = BSP_I2C_SENSOR_INSTANCE,
        .addressingMode = HAL_I2C_ADDRESSINGMODE_7BIT,
        .clockSpeed = BSP_I2C_SENSOR_CLOCK_SPEED,
        .ownAddress = 0,
        .generalCall = false,
        .noStretch = false
    };
    
    HAL_I2C_Init(&i2cConfig);
    
    i2cConfig.instance = BSP_I2C_EEPROM_INSTANCE;
    i2cConfig.clockSpeed = BSP_I2C_EEPROM_CLOCK_SPEED;
    HAL_I2C_Init(&i2cConfig);
}

/**
 * @brief Initialize SPI peripherals
 * 
 * @details Configures SPI buses for flash and sensors.
 */
static void BSP_InitSPIs(void)
{
    HAL_SPI_Config_t spiConfig = {
        .instance = BSP_SPI_FLASH_INSTANCE,
        .mode = HAL_SPI_MODE_MASTER,
        .clockSpeed = BSP_SPI_FLASH_CLOCK_SPEED,
        .dataSize = 8,
        .clockPolarity = HAL_SPI_CPOL_LOW,
        .clockPhase = HAL_SPI_CPHA_1EDGE,
        .firstBitMSB = true,
        .ssOutput = true
    };
    
    HAL_SPI_Init(&spiConfig);
}

/**
 * @brief Initialize ADC peripheral
 * 
 * @details Configures ADC channels for analog inputs.
 */
static void BSP_InitADC(void)
{
    HAL_ADC_Config_t adcConfig = {
        .instance = BSP_ADC_INSTANCE,
        .resolution = 12,
        .channel = BSP_ADC_BATTERY_CHANNEL,
        .sampleTime = 480,
        .continuous = false,
        .dmaEnable = false
    };
    
    HAL_ADC_Init(&adcConfig);
}

/**
 * @brief Initialize timers
 * 
 * @details Configures timers for system functions.
 */
static void BSP_InitTimers(void)
{
    HAL_TIMER_Config_t timerConfig = {
        .instance = BSP_PWM_INSTANCE,
        .counterMode = HAL_TIMER_MODE_UP,
        .period = (BSP_APB1_CLOCK / BSP_PWM_FREQUENCY_HZ) - 1,
        .prescaler = 0,
        .clockDivision = 0,
        .autoReloadPreload = true
    };
    
    HAL_TIMER_Init(&timerConfig);
}

/*============================================================================*/
/*                              PUBLIC API IMPLEMENTATION                     */
/*============================================================================*/

/**
 * @brief Initialize the BSP
 */
HAL_Status_e BSP_Init(void)
{
    if (s_bspInitialized) {
        return HAL_OK;
    }
    
    BSP_InitGPIOClocks();
    
    BSP_InitLEDs();
    BSP_InitButtons();
    BSP_InitUARTs();
    BSP_InitI2Cs();
    BSP_InitSPIs();
    BSP_InitADC();
    BSP_InitTimers();
    
    s_bspInitialized = true;
    
    return HAL_OK;
}

/**
 * @brief Deinitialize the BSP
 */
void BSP_Deinit(void)
{
    if (!s_bspInitialized) {
        return;
    }
    
    for (uint8_t i = 0; i < BSP_LED_COUNT; i++) {
        HAL_GPIO_Deinit(s_ledConfig[i].port, s_ledConfig[i].pin);
    }
    
    for (uint8_t i = 0; i < BSP_BUTTON_COUNT; i++) {
        HAL_GPIO_Deinit(s_buttonConfig[i].port, s_buttonConfig[i].pin);
    }
    
    HAL_UART_Deinit(BSP_UART_DEBUG_INSTANCE);
    HAL_I2C_Deinit(BSP_I2C_SENSOR_INSTANCE);
    HAL_I2C_Deinit(BSP_I2C_EEPROM_INSTANCE);
    HAL_SPI_Deinit(BSP_SPI_FLASH_INSTANCE);
    HAL_ADC_Deinit(BSP_ADC_INSTANCE);
    HAL_TIMER_Deinit(BSP_PWM_INSTANCE);
    
    s_bspInitialized = false;
}

/**
 * @brief Turn LED on
 */
void BSP_LED_On(BSP_LED_e led)
{
    if (led >= BSP_LED_COUNT) {
        return;
    }
    
    bool state = s_ledConfig[led].activeHigh ? true : false;
    HAL_GPIO_Write(s_ledConfig[led].port, s_ledConfig[led].pin, state);
}

/**
 * @brief Turn LED off
 */
void BSP_LED_Off(BSP_LED_e led)
{
    if (led >= BSP_LED_COUNT) {
        return;
    }
    
    bool state = s_ledConfig[led].activeHigh ? false : true;
    HAL_GPIO_Write(s_ledConfig[led].port, s_ledConfig[led].pin, state);
}

/**
 * @brief Toggle LED state
 */
void BSP_LED_Toggle(BSP_LED_e led)
{
    if (led >= BSP_LED_COUNT) {
        return;
    }
    
    HAL_GPIO_Toggle(s_ledConfig[led].port, s_ledConfig[led].pin);
}

/**
 * @brief Get button state
 */
bool BSP_Button_Read(BSP_Button_e button)
{
    if (button >= BSP_BUTTON_COUNT) {
        return false;
    }
    
    bool rawState = HAL_GPIO_Read(s_buttonConfig[button].port, 
                                   s_buttonConfig[button].pin);
    
    if (s_buttonConfig[button].activeHigh) {
        return rawState;
    } else {
        return !rawState;
    }
}

/**
 * @brief Get board name string
 */
const char* BSP_GetBoardName(void)
{
    return BSP_BOARD_NAME;
}

/**
 * @brief Get board revision string
 */
const char* BSP_GetBoardRevision(void)
{
    return BSP_BOARD_REVISION;
}

/*============================================================================*/
/*                              BSP UTILITY FUNCTIONS                         */
/*============================================================================*/

/**
 * @brief Read temperature sensor
 * 
 * @param pTemperature Output pointer for temperature in Celsius
 * @return HAL_OK on success
 */
HAL_Status_e BSP_ReadTemperature(float *pTemperature)
{
    if (pTemperature == NULL) {
        return HAL_INVALID_PARAM;
    }
    
    uint8_t rawData[2];
    HAL_Status_e status;
    
    status = HAL_I2C_MasterReceive(BSP_I2C_SENSOR_INSTANCE, 
                                    BSP_TEMP_SENSOR_ADDR << 1,
                                    rawData, 2, 1000);
    
    if (status != HAL_OK) {
        return status;
    }
    
    int16_t rawTemp = ((int16_t)rawData[0] << 8) | rawData[1];
    *pTemperature = (float)rawTemp / 256.0f;
    
    return HAL_OK;
}

/**
 * @brief Read humidity sensor
 * 
 * @param pHumidity Output pointer for humidity percentage
 * @return HAL_OK on success
 */
HAL_Status_e BSP_ReadHumidity(float *pHumidity)
{
    if (pHumidity == NULL) {
        return HAL_INVALID_PARAM;
    }
    
    uint8_t cmd = 0xE5;
    uint8_t rawData[2];
    HAL_Status_e status;
    
    status = HAL_I2C_MasterTransmit(BSP_I2C_SENSOR_INSTANCE,
                                     BSP_HUMIDITY_SENSOR_ADDR << 1,
                                     &cmd, 1, 1000);
    if (status != HAL_OK) {
        return status;
    }
    
    status = HAL_I2C_MasterReceive(BSP_I2C_SENSOR_INSTANCE,
                                    BSP_HUMIDITY_SENSOR_ADDR << 1,
                                    rawData, 2, 1000);
    if (status != HAL_OK) {
        return status;
    }
    
    uint16_t rawHumidity = ((uint16_t)rawData[0] << 8) | rawData[1];
    *pHumidity = (125.0f * (float)rawHumidity / 65536.0f) - 6.0f;
    
    return HAL_OK;
}

/**
 * @brief Read battery voltage
 * 
 * @param pVoltage Output pointer for voltage in millivolts
 * @return HAL_OK on success
 */
HAL_Status_e BSP_ReadBatteryVoltage(uint16_t *pVoltage)
{
    if (pVoltage == NULL) {
        return HAL_INVALID_PARAM;
    }
    
    uint16_t adcValue;
    HAL_Status_e status = HAL_ADC_Read(BSP_ADC_INSTANCE, &adcValue, 100);
    
    if (status != HAL_OK) {
        return status;
    }
    
    *pVoltage = (uint16_t)((float)adcValue * 3300.0f / 4095.0f * 2.0f);
    
    return HAL_OK;
}

/**
 * @brief Set PWM duty cycle
 * 
 * @param channel   PWM channel (0-3)
 * @param dutyCycle Duty cycle in percent (0-100)
 * @return HAL_OK on success
 */
HAL_Status_e BSP_SetPWM(uint8_t channel, uint8_t dutyCycle)
{
    if (channel > 3 || dutyCycle > 100) {
        return HAL_INVALID_PARAM;
    }
    
    (void)channel;
    (void)dutyCycle;
    
    return HAL_OK;
}

/**
 * @brief Write to EEPROM
 * 
 * @param address Memory address to write
 * @param data    Data to write
 * @param length  Number of bytes
 * @return HAL_OK on success
 */
HAL_Status_e BSP_EEPROM_Write(uint16_t address, const uint8_t *data, uint16_t length)
{
    if (data == NULL || length == 0) {
        return HAL_INVALID_PARAM;
    }
    
    return HAL_I2C_MemWrite(BSP_I2C_EEPROM_INSTANCE,
                             BSP_EEPROM_ADDR << 1,
                             address, 2,
                             data, length, 1000);
}

/**
 * @brief Read from EEPROM
 * 
 * @param address Memory address to read
 * @param data    Buffer for read data
 * @param length  Number of bytes to read
 * @return HAL_OK on success
 */
HAL_Status_e BSP_EEPROM_Read(uint16_t address, uint8_t *data, uint16_t length)
{
    if (data == NULL || length == 0) {
        return HAL_INVALID_PARAM;
    }
    
    return HAL_I2C_MemRead(BSP_I2C_EEPROM_INSTANCE,
                            BSP_EEPROM_ADDR << 1,
                            address, 2,
                            data, length, 1000);
}

/**
 * @brief Debug printf output
 * 
 * @param format Format string
 * @param ...    Variable arguments
 * @return Number of characters written
 */
int BSP_Printf(const char *format, ...)
{
    (void)format;
    return 0;
}
