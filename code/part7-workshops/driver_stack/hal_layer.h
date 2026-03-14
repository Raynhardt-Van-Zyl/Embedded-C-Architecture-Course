/**
 * @file hal_layer.h
 * @brief Hardware Abstraction Layer Definitions
 * 
 * @details This header defines the HAL interface that provides a portable
 * abstraction layer between the driver layer and the actual hardware.
 * The HAL encapsulates all register-level operations.
 * 
 * Architecture Layers:
 * ┌─────────────────────────────────────────────────────────────┐
 * │                    APPLICATION LAYER                        │
 * ├─────────────────────────────────────────────────────────────┤
 * │                    DRIVER INTERFACE                         │
 * ├─────────────────────────────────────────────────────────────┤
 * │                    HAL (This File)                          │
 * │  ┌───────────────────────────────────────────────────────┐ │
 * │  │  GPIO HAL │ UART HAL │ SPI HAL │ I2C HAL │ Timer HAL  │ │
 * │  └───────────────────────────────────────────────────────┘ │
 * ├─────────────────────────────────────────────────────────────┤
 * │                    MCU REGISTERS                            │
 * └─────────────────────────────────────────────────────────────┘
 * 
 * Key Features:
 * - Portable across different MCUs
 * - Register access abstraction
 * - Interrupt management
 * - Clock configuration
 * - Power management hooks
 * 
 * @author Embedded C Architecture Course
 * @version 1.0.0
 * @date 2024
 */

#ifndef HAL_LAYER_H
#define HAL_LAYER_H

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

/** @brief HAL version information */
#define HAL_VERSION_MAJOR          (1U)
#define HAL_VERSION_MINOR          (0U)
#define HAL_VERSION_PATCH          (0U)

/** @brief Maximum number of GPIO ports */
#define HAL_GPIO_PORT_COUNT        (8U)

/** @brief Maximum pins per port */
#define HAL_GPIO_PINS_PER_PORT     (16U)

/** @brief Maximum number of UART instances */
#define HAL_UART_INSTANCE_COUNT    (8U)

/** @brief Maximum number of SPI instances */
#define HAL_SPI_INSTANCE_COUNT     (4U)

/** @brief Maximum number of I2C instances */
#define HAL_I2C_INSTANCE_COUNT     (4U)

/** @brief Maximum number of timer instances */
#define HAL_TIMER_INSTANCE_COUNT   (8U)

/** @brief Maximum number of ADC instances */
#define HAL_ADC_INSTANCE_COUNT     (3U)

/** @brief Maximum number of DMA channels */
#define HAL_DMA_CHANNEL_COUNT      (16U)

/** @brief Set bit macro */
#define HAL_SET_BIT(reg, bit)      ((reg) |= (1U << (bit)))

/** @brief Clear bit macro */
#define HAL_CLEAR_BIT(reg, bit)    ((reg) &= ~(1U << (bit)))

/** @brief Read bit macro */
#define HAL_READ_BIT(reg, bit)     (((reg) >> (bit)) & 1U)

/** @brief Toggle bit macro */
#define HAL_TOGGLE_BIT(reg, bit)   ((reg) ^= (1U << (bit)))

/** @brief Write value to bitfield macro */
#define HAL_WRITE_REG(reg, val)    ((reg) = (val))

/** @brief Read register macro */
#define HAL_READ_REG(reg)          ((reg))

/*============================================================================*/
/*                              ENUMERATIONS                                  */
/*============================================================================*/

/**
 * @brief HAL status codes
 */
typedef enum {
    HAL_OK = 0,                     /**< Operation successful */
    HAL_ERROR = -1,                 /**< Generic error */
    HAL_BUSY = -2,                  /**< Resource busy */
    HAL_TIMEOUT = -3,               /**< Operation timeout */
    HAL_INVALID_PARAM = -4,         /**< Invalid parameter */
    HAL_NOT_INITIALIZED = -5,       /**< Not initialized */
    HAL_NOT_SUPPORTED = -6          /**< Feature not supported */
} HAL_Status_e;

/**
 * @brief GPIO port enumeration
 */
typedef enum {
    HAL_GPIO_PORT_A = 0,            /**< GPIO Port A */
    HAL_GPIO_PORT_B,                /**< GPIO Port B */
    HAL_GPIO_PORT_C,                /**< GPIO Port C */
    HAL_GPIO_PORT_D,                /**< GPIO Port D */
    HAL_GPIO_PORT_E,                /**< GPIO Port E */
    HAL_GPIO_PORT_F,                /**< GPIO Port F */
    HAL_GPIO_PORT_G,                /**< GPIO Port G */
    HAL_GPIO_PORT_H                 /**< GPIO Port H */
} HAL_GPIO_Port_e;

/**
 * @brief GPIO pin mode
 */
typedef enum {
    HAL_GPIO_MODE_INPUT = 0,        /**< Input mode (floating) */
    HAL_GPIO_MODE_OUTPUT_PP,        /**< Output push-pull */
    HAL_GPIO_MODE_OUTPUT_OD,        /**< Output open-drain */
    HAL_GPIO_MODE_AF_PP,            /**< Alternate function push-pull */
    HAL_GPIO_MODE_AF_OD,            /**< Alternate function open-drain */
    HAL_GPIO_MODE_ANALOG,           /**< Analog mode */
    HAL_GPIO_MODE_IT_RISING,        /**< External interrupt rising edge */
    HAL_GPIO_MODE_IT_FALLING,       /**< External interrupt falling edge */
    HAL_GPIO_MODE_IT_RISING_FALLING /**< External interrupt both edges */
} HAL_GPIO_Mode_e;

/**
 * @brief GPIO pull configuration
 */
typedef enum {
    HAL_GPIO_NOPULL = 0,            /**< No pull resistor */
    HAL_GPIO_PULLUP,                /**< Pull-up resistor */
    HAL_GPIO_PULLDOWN               /**< Pull-down resistor */
} HAL_GPIO_Pull_e;

/**
 * @brief GPIO speed configuration
 */
typedef enum {
    HAL_GPIO_SPEED_LOW = 0,         /**< Low speed */
    HAL_GPIO_SPEED_MEDIUM,          /**< Medium speed */
    HAL_GPIO_SPEED_HIGH,            /**< High speed */
    HAL_GPIO_SPEED_VERY_HIGH        /**< Very high speed */
} HAL_GPIO_Speed_e;

/**
 * @brief UART parity configuration
 */
typedef enum {
    HAL_UART_PARITY_NONE = 0,       /**< No parity */
    HAL_UART_PARITY_EVEN,           /**< Even parity */
    HAL_UART_PARITY_ODD             /**< Odd parity */
} HAL_UART_Parity_e;

/**
 * @brief UART stop bits configuration
 */
typedef enum {
    HAL_UART_STOPBITS_1 = 0,        /**< 1 stop bit */
    HAL_UART_STOPBITS_2,            /**< 2 stop bits */
    HAL_UART_STOPBITS_1_5           /**< 1.5 stop bits */
} HAL_UART_StopBits_e;

/**
 * @brief SPI clock polarity
 */
typedef enum {
    HAL_SPI_CPOL_LOW = 0,           /**< Clock polarity low when idle */
    HAL_SPI_CPOL_HIGH               /**< Clock polarity high when idle */
} HAL_SPI_CPOL_e;

/**
 * @brief SPI clock phase
 */
typedef enum {
    HAL_SPI_CPHA_1EDGE = 0,         /**< Data captured on first edge */
    HAL_SPI_CPHA_2EDGE              /**< Data captured on second edge */
} HAL_SPI_CPHA_e;

/**
 * @brief SPI mode (master/slave)
 */
typedef enum {
    HAL_SPI_MODE_SLAVE = 0,         /**< Slave mode */
    HAL_SPI_MODE_MASTER             /**< Master mode */
} HAL_SPI_Mode_e;

/**
 * @brief I2C addressing mode
 */
typedef enum {
    HAL_I2C_ADDRESSINGMODE_7BIT = 0,/**< 7-bit addressing */
    HAL_I2C_ADDRESSINGMODE_10BIT    /**< 10-bit addressing */
} HAL_I2C_AddressingMode_e;

/**
 * @brief Timer counter mode
 */
typedef enum {
    HAL_TIMER_MODE_UP = 0,          /**< Up counting */
    HAL_TIMER_MODE_DOWN,            /**< Down counting */
    HAL_TIMER_MODE_CENTERALIGNED1,  /**< Center aligned mode 1 */
    HAL_TIMER_MODE_CENTERALIGNED2,  /**< Center aligned mode 2 */
    HAL_TIMER_MODE_CENTERALIGNED3   /**< Center aligned mode 3 */
} HAL_TIMER_Mode_e;

/**
 * @brief Clock source selection
 */
typedef enum {
    HAL_CLKSOURCE_INTERNAL = 0,     /**< Internal oscillator */
    HAL_CLKSOURCE_EXTERNAL,         /**< External oscillator */
    HAL_CLKSOURCE_PLL,              /**< PLL output */
    HAL_CLKSOURCE_LSE,              /**< Low-speed external */
    HAL_CLKSOURCE_LSI               /**< Low-speed internal */
} HAL_ClockSource_e;

/**
 * @brief Interrupt priority levels
 */
typedef enum {
    HAL_IRQ_PRIORITY_LOWEST = 15,   /**< Lowest priority */
    HAL_IRQ_PRIORITY_LOW = 10,      /**< Low priority */
    HAL_IRQ_PRIORITY_MEDIUM = 5,    /**< Medium priority */
    HAL_IRQ_PRIORITY_HIGH = 2,      /**< High priority */
    HAL_IRQ_PRIORITY_HIGHEST = 0    /**< Highest priority */
} HAL_IRQ_Priority_e;

/*============================================================================*/
/*                              STRUCTURES                                    */
/*============================================================================*/

/**
 * @brief GPIO pin configuration structure
 */
typedef struct {
    HAL_GPIO_Port_e     port;           /**< GPIO port */
    uint8_t             pin;            /**< Pin number (0-15) */
    HAL_GPIO_Mode_e     mode;           /**< Pin mode */
    HAL_GPIO_Pull_e     pull;           /**< Pull configuration */
    HAL_GPIO_Speed_e    speed;          /**< Output speed */
    uint8_t             alternate;      /**< Alternate function (0-15) */
} HAL_GPIO_Config_t;

/**
 * @brief UART configuration structure
 */
typedef struct {
    uint8_t             instance;       /**< UART instance (0-7) */
    uint32_t            baudRate;       /**< Baud rate */
    uint32_t            wordLength;     /**< Word length (8 or 9) */
    HAL_UART_Parity_e   parity;         /**< Parity setting */
    HAL_UART_StopBits_e stopBits;       /**< Stop bits */
    bool                hwFlowControl;  /**< Hardware flow control */
    bool                oversampling;   /**< Oversampling (8 or 16) */
} HAL_UART_Config_t;

/**
 * @brief SPI configuration structure
 */
typedef struct {
    uint8_t             instance;       /**< SPI instance (0-3) */
    HAL_SPI_Mode_e      mode;           /**< Master/Slave mode */
    uint32_t            clockSpeed;     /**< Clock speed in Hz */
    uint8_t             dataSize;       /**< Data frame size (4-16) */
    HAL_SPI_CPOL_e      clockPolarity;  /**< Clock polarity */
    HAL_SPI_CPHA_e      clockPhase;     /**< Clock phase */
    bool                firstBitMSB;    /**< MSB first if true */
    bool                ssOutput;       /**< SS output enable */
} HAL_SPI_Config_t;

/**
 * @brief I2C configuration structure
 */
typedef struct {
    uint8_t                 instance;       /**< I2C instance (0-3) */
    HAL_I2C_AddressingMode_e addressingMode; /**< Addressing mode */
    uint32_t                clockSpeed;     /**< Clock speed in Hz */
    uint16_t                ownAddress;     /**< Own slave address */
    bool                    generalCall;    /**< General call enable */
    bool                    noStretch;      /**< Clock stretching disable */
} HAL_I2C_Config_t;

/**
 * @brief Timer configuration structure
 */
typedef struct {
    uint8_t             instance;       /**< Timer instance (0-7) */
    HAL_TIMER_Mode_e    counterMode;    /**< Counter mode */
    uint32_t            period;         /**< Auto-reload period */
    uint32_t            prescaler;      /**< Prescaler value */
    uint32_t            clockDivision;  /**< Clock division */
    bool                autoReloadPreload; /**< Auto-reload preload */
} HAL_TIMER_Config_t;

/**
 * @brief ADC configuration structure
 */
typedef struct {
    uint8_t             instance;       /**< ADC instance (0-2) */
    uint8_t             resolution;     /**< Resolution (8, 10, 12 bits) */
    uint8_t             channel;        /**< Channel number */
    uint32_t            sampleTime;     /**< Sampling time cycles */
    bool                continuous;     /**< Continuous conversion mode */
    bool                dmaEnable;      /**< DMA enable */
} HAL_ADC_Config_t;

/**
 * @brief DMA configuration structure
 */
typedef struct {
    uint8_t             channel;        /**< DMA channel (0-15) */
    uint32_t            direction;      /**< Transfer direction */
    uint16_t            bufferSize;     /**< Buffer size */
    bool                memoryIncrement;/**< Memory address increment */
    bool                peripheralInc;  /**< Peripheral address increment */
    uint8_t             memorySize;     /**< Memory data size */
    uint8_t             peripheralSize; /**< Peripheral data size */
    bool                circular;       /**< Circular mode */
    uint8_t             priority;       /**< Channel priority */
} HAL_DMA_Config_t;

/**
 * @brief System clock configuration structure
 */
typedef struct {
    HAL_ClockSource_e   source;         /**< Clock source */
    uint32_t            sysClock;       /**< System clock frequency (Hz) */
    uint32_t            ahbPrescaler;   /**< AHB prescaler */
    uint32_t            apb1Prescaler;  /**< APB1 prescaler */
    uint32_t            apb2Prescaler;  /**< APB2 prescaler */
    bool                usePLL;         /**< Use PLL */
    uint8_t             pllM;           /**< PLL M divider */
    uint8_t             pllN;           /**< PLL N multiplier */
    uint8_t             pllP;           /**< PLL P divider */
    uint8_t             pllQ;           /**< PLL Q divider */
} HAL_ClockConfig_t;

/*============================================================================*/
/*                              CALLBACK TYPES                                */
/*============================================================================*/

/**
 * @brief GPIO interrupt callback
 */
typedef void (*HAL_GPIO_Callback_t)(HAL_GPIO_Port_e port, uint8_t pin);

/**
 * @brief UART receive callback
 */
typedef void (*HAL_UART_RxCallback_t)(uint8_t instance, uint8_t data);

/**
 * @brief UART transmit complete callback
 */
typedef void (*HAL_UART_TxCallback_t)(uint8_t instance);

/**
 * @brief Timer overflow callback
 */
typedef void (*HAL_TIMER_Callback_t)(uint8_t instance);

/*============================================================================*/
/*                              HAL INITIALIZATION                            */
/*============================================================================*/

/**
 * @brief Initialize the HAL layer
 * 
 * @return HAL_OK on success
 */
HAL_Status_e HAL_Init(void);

/**
 * @brief Deinitialize the HAL layer
 */
void HAL_Deinit(void);

/**
 * @brief Configure system clocks
 * 
 * @param config Clock configuration
 * @return HAL_OK on success
 */
HAL_Status_e HAL_ClockConfig(const HAL_ClockConfig_t *config);

/**
 * @brief Get system clock frequency
 * 
 * @return System clock frequency in Hz
 */
uint32_t HAL_GetSystemClock(void);

/**
 * @brief Enable global interrupts
 */
void HAL_EnableIRQ(void);

/**
 * @brief Disable global interrupts
 */
void HAL_DisableIRQ(void);

/**
 * @brief Get tick count (milliseconds since boot)
 * 
 * @return Tick count in milliseconds
 */
uint32_t HAL_GetTick(void);

/**
 * @brief Delay for specified milliseconds
 * 
 * @param ms Delay in milliseconds
 */
void HAL_Delay(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* HAL_LAYER_H */
