/**
 * @file bsp_config.h
 * @brief Board Support Package Configuration Header
 * 
 * @details This header defines the board-specific configuration for a
 * target hardware platform. It contains pin mappings, peripheral assignments,
 * and hardware-specific constants that adapt the HAL to a specific board.
 * 
 * Architecture Overview:
 * ┌─────────────────────────────────────────────────────────────┐
 * │                    APPLICATION LAYER                        │
 * ├─────────────────────────────────────────────────────────────┤
 * │                    DRIVER INTERFACE                         │
 * ├─────────────────────────────────────────────────────────────┤
 * │                    HARDWARE ABSTRACTION LAYER               │
 * ├─────────────────────────────────────────────────────────────┤
 * │                    BSP (This File)                          │
 * │  ┌───────────────────────────────────────────────────────┐ │
 * │  │  Pin Mappings │ Peripheral Config │ Board Constants   │ │
 * │  └───────────────────────────────────────────────────────┘ │
 * ├─────────────────────────────────────────────────────────────┤
 * │                    PHYSICAL HARDWARE                        │
 * └─────────────────────────────────────────────────────────────┘
 * 
 * BSP Design Principles:
 * - All hardware-specific definitions in one place
 * - Clear separation between logical and physical names
 * - Easy to port to new hardware by changing this file
 * - Compile-time configuration for optimization
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#ifndef BSP_CONFIG_H
#define BSP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*                              INCLUDES                                      */
/*============================================================================*/

#include "Hallayer.h"

/*============================================================================*/
/*                              BOARD IDENTIFICATION                          */
/*============================================================================*/

/** @brief Board name string */
#define BSP_BOARD_NAME              "STM32F407-DISCO"

/** @brief Board revision */
#define BSP_BOARD_REVISION          "1.0"

/** @brief MCU part number */
#define BSP_MCU_PART_NUMBER         "STM32F407VGT6"

/** @brief BSP version information */
#define BSP_VERSION_MAJOR           (1U)
#define BSP_VERSION_MINOR           (0U)
#define BSP_VERSION_PATCH           (0U)

/*============================================================================*/
/*                              CLOCK CONFIGURATION                           */
/*============================================================================*/

/** @brief External oscillator frequency (Hz) */
#define BSP_HSE_FREQUENCY           (8000000U)

/** @brief System clock frequency (Hz) */
#define BSP_SYSTEM_CLOCK            (168000000U)

/** @brief AHB bus clock frequency (Hz) */
#define BSP_AHB_CLOCK               (168000000U)

/** @brief APB1 bus clock frequency (Hz) */
#define BSP_APB1_CLOCK              (42000000U)

/** @brief APB2 bus clock frequency (Hz) */
#define BSP_APB2_CLOCK              (84000000U)

/** @brief PLL configuration */
#define BSP_PLL_M                   (8U)
#define BSP_PLL_N                   (336U)
#define BSP_PLL_P                   (2U)
#define BSP_PLL_Q                   (7U)

/*============================================================================*/
/*                              LED DEFINITIONS                               */
/*============================================================================*/

/**
 * @brief LED enumeration
 * 
 * @details Logical LED names that map to physical pins.
 * Use these enums in application code for portability.
 */
typedef enum {
    BSP_LED_1 = 0,                  /**< LED 1 (Green) */
    BSP_LED_2,                      /**< LED 2 (Orange) */
    BSP_LED_3,                      /**< LED 3 (Red) */
    BSP_LED_4,                      /**< LED 4 (Blue) */
    BSP_LED_COUNT                   /**< Number of LEDs */
} BSP_LED_e;

/** @brief LED 1 GPIO configuration */
#define BSP_LED_1_PORT              HalGPIO_PORT_D
#define BSP_LED_1_PIN               (12U)
#define BSP_LED_1_ACTIVE_HIGH       (1U)

/** @brief LED 2 GPIO configuration */
#define BSP_LED_2_PORT              HalGPIO_PORT_D
#define BSP_LED_2_PIN               (13U)
#define BSP_LED_2_ACTIVE_HIGH       (1U)

/** @brief LED 3 GPIO configuration */
#define BSP_LED_3_PORT              HalGPIO_PORT_D
#define BSP_LED_3_PIN               (14U)
#define BSP_LED_3_ACTIVE_HIGH       (1U)

/** @brief LED 4 GPIO configuration */
#define BSP_LED_4_PORT              HalGPIO_PORT_D
#define BSP_LED_4_PIN               (15U)
#define BSP_LED_4_ACTIVE_HIGH       (1U)

/*============================================================================*/
/*                              BUTTON DEFINITIONS                            */
/*============================================================================*/

/**
 * @brief Button enumeration
 */
typedef enum {
    BSP_BUTTON_1 = 0,               /**< User button */
    BSP_BUTTON_RESET,               /**< Reset button (usually hardware) */
    BSP_BUTTON_COUNT                /**< Number of buttons */
} BSP_Button_e;

/** @brief User button GPIO configuration */
#define BSP_BUTTON_1_PORT           HalGPIO_PORT_A
#define BSP_BUTTON_1_PIN            (0U)
#define BSP_BUTTON_1_ACTIVE_HIGH    (1U)

/*============================================================================*/
/*                              UART DEFINITIONS                              */
/*============================================================================*/

/**
 * @brief UART peripheral enumeration
 */
typedef enum {
    BSP_UART_DEBUG = 0,             /**< Debug/console UART */
    BSP_UART_SENSOR,                /**< Sensor communication UART */
    BSP_UART_WIRELESS,              /**< Wireless module UART */
    BSP_UART_COUNT                  /**< Number of UARTs */
} BSP_UART_e;

/** @brief Debug UART configuration */
#define BSP_UART_DEBUG_INSTANCE     (2U)
#define BSP_UART_DEBUG_BAUDRATE     (115200U)
#define BSP_UART_DEBUG_TX_PORT      HalGPIO_PORT_D
#define BSP_UART_DEBUG_TX_PIN       (8U)
#define BSP_UART_DEBUG_TX_AF        (7U)
#define BSP_UART_DEBUG_RX_PORT      HalGPIO_PORT_D
#define BSP_UART_DEBUG_RX_PIN       (9U)
#define BSP_UART_DEBUG_RX_AF        (7U)

/** @brief Sensor UART configuration */
#define BSP_UART_SENSOR_INSTANCE    (3U)
#define BSP_UART_SENSOR_BAUDRATE    (9600U)
#define BSP_UART_SENSOR_TX_PORT     HalGPIO_PORT_D
#define BSP_UART_SENSOR_TX_PIN      (8U)
#define BSP_UART_SENSOR_TX_AF       (7U)
#define BSP_UART_SENSOR_RX_PORT     HalGPIO_PORT_D
#define BSP_UART_SENSOR_RX_PIN      (9U)
#define BSP_UART_SENSOR_RX_AF       (7U)

/*============================================================================*/
/*                              SPI DEFINITIONS                               */
/*============================================================================*/

/**
 * @brief SPI peripheral enumeration
 */
typedef enum {
    BSP_SPI_FLASH = 0,              /**< External flash SPI */
    BSP_SPI_SENSOR,                 /**< Sensor SPI bus */
    BSP_SPI_DISPLAY,                /**< Display SPI */
    BSP_SPI_COUNT                   /**< Number of SPI buses */
} BSP_SPI_e;

/** @brief SPI Flash configuration */
#define BSP_SPI_FLASH_INSTANCE      (1U)
#define BSP_SPI_FLASH_CLOCK_SPEED   (21000000U)
#define BSP_SPI_FLASH_CS_PORT       HalGPIO_PORT_A
#define BSP_SPI_FLASH_CS_PIN        (4U)
#define BSP_SPI_FLASH_SCK_PORT      HalGPIO_PORT_A
#define BSP_SPI_FLASH_SCK_PIN       (5U)
#define BSP_SPI_FLASH_MISO_PORT     HalGPIO_PORT_A
#define BSP_SPI_FLASH_MISO_PIN      (6U)
#define BSP_SPI_FLASH_MOSI_PORT     HalGPIO_PORT_A
#define BSP_SPI_FLASH_MOSI_PIN      (7U)

/*============================================================================*/
/*                              I2C DEFINITIONS                               */
/*============================================================================*/

/**
 * @brief I2C peripheral enumeration
 */
typedef enum {
    BSP_I2C_SENSOR = 0,             /**< Sensor I2C bus */
#define BSP_I2C_1                   BSP_I2C_SENSOR
    BSP_I2C_EEPROM,                 /**< EEPROM I2C bus */
#define BSP_I2C_2                   BSP_I2C_EEPROM
    BSP_I2C_COUNT                   /**< Number of I2C buses */
} BSP_I2C_e;

/** @brief I2C Sensor bus configuration */
#define BSP_I2C_SENSOR_INSTANCE     (1U)
#define BSP_I2C_SENSOR_CLOCK_SPEED  (100000U)
#define BSP_I2C_SENSOR_SCL_PORT     HalGPIO_PORT_B
#define BSP_I2C_SENSOR_SCL_PIN      (6U)
#define BSP_I2C_SENSOR_SDA_PORT     HalGPIO_PORT_B
#define BSP_I2C_SENSOR_SDA_PIN      (7U)

/** @brief I2C EEPROM bus configuration */
#define BSP_I2C_EEPROM_INSTANCE     (2U)
#define BSP_I2C_EEPROM_CLOCK_SPEED  (400000U)
#define BSP_I2C_EEPROM_SCL_PORT     HalGPIO_PORT_B
#define BSP_I2C_EEPROM_SCL_PIN      (10U)
#define BSP_I2C_EEPROM_SDA_PORT     HalGPIO_PORT_B
#define BSP_I2C_EEPROM_SDA_PIN      (11U)

/*============================================================================*/
/*                              I2C DEVICE ADDRESSES                          */
/*============================================================================*/

/** @brief Temperature sensor I2C address */
#define BSP_TEMP_SENSOR_ADDR        (0x48U)

/** @brief Humidity sensor I2C address */
#define BSP_HUMIDITY_SENSOR_ADDR    (0x40U)

/** @brief Accelerometer I2C address */
#define BSP_ACCEL_SENSOR_ADDR       (0x19U)

/** @brief EEPROM I2C address */
#define BSP_EEPROM_ADDR             (0x50U)

/** @brief RTC I2C address */
#define BSP_RTC_ADDR                (0x68U)

/*============================================================================*/
/*                              ADC DEFINITIONS                               */
/*============================================================================*/

/**
 * @brief ADC channel enumeration
 */
typedef enum {
    BSP_ADC_BATTERY = 0,            /**< Battery voltage */
    BSP_ADC_TEMPERATURE,            /**< Internal temperature */
    BSP_ADC_LIGHT,                  /**< Light sensor */
    BSP_ADC_POTENTIOMETER,          /**< Potentiometer */
    BSP_ADC_COUNT                   /**< Number of ADC channels */
} BSP_ADC_e;

/** @brief ADC configuration */
#define BSP_ADC_INSTANCE            (1U)
#define BSP_ADC_BATTERY_CHANNEL     (1U)
#define BSP_ADC_BATTERY_PORT        HalGPIO_PORT_A
#define BSP_ADC_BATTERY_PIN         (1U)

/*============================================================================*/
/*                              TIMER DEFINITIONS                             */
/*============================================================================*/

/**
 * @brief Timer enumeration
 */
typedef enum {
    BSP_TIMER_SYSTICK = 0,          /**< System tick timer */
    BSP_TIMER_DELAY,                /**< Microsecond delay timer */
    BSP_TIMER_PWM,                  /**< PWM timer */
    BSP_TIMER_ENCODER,              /**< Encoder interface timer */
    BSP_TIMER_COUNT                 /**< Number of timers */
} BSP_TIMER_e;

/** @brief System tick timer configuration */
#define BSP_SYSTICK_INSTANCE        (0U)
#define BSP_SYSTICK_FREQUENCY_HZ    (1000U)

/** @brief PWM timer configuration */
#define BSP_PWM_INSTANCE            (3U)
#define BSP_PWM_FREQUENCY_HZ        (10000U)

/*============================================================================*/
/*                              DMA DEFINITIONS                               */
/*============================================================================*/

/**
 * @brief DMA channel enumeration
 */
typedef enum {
    BSP_DMA_UART1_RX = 0,           /**< UART1 RX DMA channel */
    BSP_DMA_UART1_TX,               /**< UART1 TX DMA channel */
    BSP_DMA_SPI1_RX,                /**< SPI1 RX DMA channel */
    BSP_DMA_SPI1_TX,                /**< SPI1 TX DMA channel */
    BSP_DMA_ADC1,                   /**< ADC1 DMA channel */
    BSP_DMA_COUNT                   /**< Number of DMA channels */
} BSP_DMA_e;

/*============================================================================*/
/*                              PIN HELPER MACROS                             */
/*============================================================================*/

/**
 * @brief Get LED port
 * @param led LED enumeration
 */
#define BSP_LED_PORT(led)           (s_ledConfig[led].port)

/**
 * @brief Get LED pin
 * @param led LED enumeration
 */
#define BSP_LED_PIN(led)            (s_ledConfig[led].pin)

/*============================================================================*/
/*                              STRUCTURES                                    */
/*============================================================================*/

/**
 * @brief LED configuration structure
 */
typedef struct {
    HalGPIO_Port_e port;           /**< GPIO port */
    uint8_t         pin;            /**< Pin number */
    bool            activeHigh;     /**< Active high if true */
} BSP_LED_Config_t;

/**
 * @brief Button configuration structure
 */
typedef struct {
    HalGPIO_Port_e port;           /**< GPIO port */
    uint8_t         pin;            /**< Pin number */
    bool            activeHigh;     /**< Active high if true */
} BSP_Button_Config_t;

/**
 * @brief UART pin configuration structure
 */
typedef struct {
    uint8_t         instance;       /**< UART instance */
    HalGPIO_Port_e txPort;         /**< TX pin port */
    uint8_t         txPin;          /**< TX pin number */
    uint8_t         txAlternate;    /**< TX alternate function */
    HalGPIO_Port_e rxPort;         /**< RX pin port */
    uint8_t         rxPin;          /**< RX pin number */
    uint8_t         rxAlternate;    /**< RX alternate function */
} BSP_UART_Config_t;

/*============================================================================*/
/*                              PUBLIC API                                    */
/*============================================================================*/

/**
 * @brief Initialize the BSP
 * 
 * @details Initializes all board peripherals and GPIO configurations.
 * Must be called after HalInit().
 * 
 * @return HalOK on success
 */
HalStatus_e BSP_Init(void);

/**
 * @brief Deinitialize the BSP
 */
void BSP_Deinit(void);

/**
 * @brief Turn LED on
 * @param led LED to turn on
 */
void BSP_LED_On(BSP_LED_e led);

/**
 * @brief Turn LED off
 * @param led LED to turn off
 */
void BSP_LED_Off(BSP_LED_e led);

/**
 * @brief Toggle LED state
 * @param led LED to toggle
 */
void BSP_LED_Toggle(BSP_LED_e led);

/**
 * @brief Get button state
 * @param button Button to read
 * @return true if pressed, false if released
 */
bool BSP_Button_Read(BSP_Button_e button);

/**
 * @brief Get board name string
 * @return Board name
 */
const char* BSP_GetBoardName(void);

/**
 * @brief Get board revision string
 * @return Board revision
 */
const char* BSP_GetBoardRevision(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_CONFIG_H */
