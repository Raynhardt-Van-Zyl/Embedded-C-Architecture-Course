/**
 * @file Hallayer.h
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

#ifndef HalLAYER_H
#define HalLAYER_H

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
#define HalVERSION_MAJOR          (1U)
#define HalVERSION_MINOR          (0U)
#define HalVERSION_PATCH          (0U)

/** @brief Maximum number of GPIO ports */
#define HalGPIO_PORT_COUNT        (8U)

/** @brief Maximum pins per port */
#define HalGPIO_PINS_PER_PORT     (16U)

/** @brief Maximum number of UART instances */
#define HalUART_INSTANCE_COUNT    (8U)

/** @brief Maximum number of SPI instances */
#define HalSPI_INSTANCE_COUNT     (4U)

/** @brief Maximum number of I2C instances */
#define HalI2C_INSTANCE_COUNT     (4U)

/** @brief Maximum number of timer instances */
#define HalTIMER_INSTANCE_COUNT   (8U)

/** @brief Maximum number of ADC instances */
#define HalADC_INSTANCE_COUNT     (3U)

/** @brief Maximum number of DMA channels */
#define HalDMA_CHANNEL_COUNT      (16U)

/** @brief Set bit macro */
#define HalSET_BIT(reg, bit)      ((reg) |= (1U << (bit)))

/** @brief Clear bit macro */
#define HalCLEAR_BIT(reg, bit)    ((reg) &= ~(1U << (bit)))

/** @brief Read bit macro */
#define HalREAD_BIT(reg, bit)     (((reg) >> (bit)) & 1U)

/** @brief Toggle bit macro */
#define HalTOGGLE_BIT(reg, bit)   ((reg) ^= (1U << (bit)))

/** @brief Write value to bitfield macro */
#define HalWRITE_REG(reg, val)    ((reg) = (val))

/** @brief Read register macro */
#define HalREAD_REG(reg)          ((reg))

/*============================================================================*/
/*                              ENUMERATIONS                                  */
/*============================================================================*/

/**
 * @brief HAL status codes
 */
typedef enum {
    HalOK = 0,                     /**< Operation successful */
    HalERROR = -1,                 /**< Generic error */
    HalBUSY = -2,                  /**< Resource busy */
    HalTIMEOUT = -3,               /**< Operation timeout */
    HalINVALID_PARAM = -4,         /**< Invalid parameter */
    HalNOT_INITIALIZED = -5,       /**< Not initialized */
    HalNOT_SUPPORTED = -6          /**< Feature not supported */
} HalStatus_e;

/**
 * @brief GPIO port enumeration
 */
typedef enum {
    HalGPIO_PORT_A = 0,            /**< GPIO Port A */
    HalGPIO_PORT_B,                /**< GPIO Port B */
    HalGPIO_PORT_C,                /**< GPIO Port C */
    HalGPIO_PORT_D,                /**< GPIO Port D */
    HalGPIO_PORT_E,                /**< GPIO Port E */
    HalGPIO_PORT_F,                /**< GPIO Port F */
    HalGPIO_PORT_G,                /**< GPIO Port G */
    HalGPIO_PORT_H                 /**< GPIO Port H */
} HalGPIO_Port_e;

/**
 * @brief GPIO pin mode
 */
typedef enum {
    HalGPIO_MODE_INPUT = 0,        /**< Input mode (floating) */
    HalGPIO_MODE_OUTPUT_PP,        /**< Output push-pull */
    HalGPIO_MODE_OUTPUT_OD,        /**< Output open-drain */
    HalGPIO_MODE_AF_PP,            /**< Alternate function push-pull */
    HalGPIO_MODE_AF_OD,            /**< Alternate function open-drain */
    HalGPIO_MODE_ANALOG,           /**< Analog mode */
    HalGPIO_MODE_IT_RISING,        /**< External interrupt rising edge */
    HalGPIO_MODE_IT_FALLING,       /**< External interrupt falling edge */
    HalGPIO_MODE_IT_RISING_FALLING /**< External interrupt both edges */
} HalGPIO_Mode_e;

/**
 * @brief GPIO pull configuration
 */
typedef enum {
    HalGPIO_NOPULL = 0,            /**< No pull resistor */
    HalGPIO_PULLUP,                /**< Pull-up resistor */
    HalGPIO_PULLDOWN               /**< Pull-down resistor */
} HalGPIO_Pull_e;

/**
 * @brief GPIO speed configuration
 */
typedef enum {
    HalGPIO_SPEED_LOW = 0,         /**< Low speed */
    HalGPIO_SPEED_MEDIUM,          /**< Medium speed */
    HalGPIO_SPEED_HIGH,            /**< High speed */
    HalGPIO_SPEED_VERY_HIGH        /**< Very high speed */
} HalGPIO_Speed_e;

/**
 * @brief UART parity configuration
 */
typedef enum {
    HalUART_PARITY_NONE = 0,       /**< No parity */
    HalUART_PARITY_EVEN,           /**< Even parity */
    HalUART_PARITY_ODD             /**< Odd parity */
} HalUART_Parity_e;

/**
 * @brief UART stop bits configuration
 */
typedef enum {
    HalUART_STOPBITS_1 = 0,        /**< 1 stop bit */
    HalUART_STOPBITS_2,            /**< 2 stop bits */
    HalUART_STOPBITS_1_5           /**< 1.5 stop bits */
} HalUART_StopBits_e;

/**
 * @brief SPI clock polarity
 */
typedef enum {
    HalSPI_CPOL_LOW = 0,           /**< Clock polarity low when idle */
    HalSPI_CPOL_HIGH               /**< Clock polarity high when idle */
} HalSPI_CPOL_e;

/**
 * @brief SPI clock phase
 */
typedef enum {
    HalSPI_CPHA_1EDGE = 0,         /**< Data captured on first edge */
    HalSPI_CPHA_2EDGE              /**< Data captured on second edge */
} HalSPI_CPHA_e;

/**
 * @brief SPI mode (master/slave)
 */
typedef enum {
    HalSPI_MODE_SLAVE = 0,         /**< Slave mode */
    HalSPI_MODE_MASTER             /**< Master mode */
} HalSPI_Mode_e;

/**
 * @brief I2C addressing mode
 */
typedef enum {
    HalI2C_ADDRESSINGMODE_7BIT = 0,/**< 7-bit addressing */
    HalI2C_ADDRESSINGMODE_10BIT    /**< 10-bit addressing */
} HalI2C_AddressingMode_e;

/**
 * @brief Timer counter mode
 */
typedef enum {
    HalTIMER_MODE_UP = 0,          /**< Up counting */
    HalTIMER_MODE_DOWN,            /**< Down counting */
    HalTIMER_MODE_CENTERALIGNED1,  /**< Center aligned mode 1 */
    HalTIMER_MODE_CENTERALIGNED2,  /**< Center aligned mode 2 */
    HalTIMER_MODE_CENTERALIGNED3   /**< Center aligned mode 3 */
} HalTIMER_Mode_e;

/**
 * @brief Clock source selection
 */
typedef enum {
    HalCLKSOURCE_INTERNAL = 0,     /**< Internal oscillator */
    HalCLKSOURCE_EXTERNAL,         /**< External oscillator */
    HalCLKSOURCE_PLL,              /**< PLL output */
    HalCLKSOURCE_LSE,              /**< Low-speed external */
    HalCLKSOURCE_LSI               /**< Low-speed internal */
} HalClockSource_e;

/**
 * @brief Interrupt priority levels
 */
typedef enum {
    HalIRQ_PRIORITY_LOWEST = 15,   /**< Lowest priority */
    HalIRQ_PRIORITY_LOW = 10,      /**< Low priority */
    HalIRQ_PRIORITY_MEDIUM = 5,    /**< Medium priority */
    HalIRQ_PRIORITY_HIGH = 2,      /**< High priority */
    HalIRQ_PRIORITY_HIGHEST = 0    /**< Highest priority */
} HalIRQ_Priority_e;

/*============================================================================*/
/*                              STRUCTURES                                    */
/*============================================================================*/

/**
 * @brief GPIO pin configuration structure
 */
typedef struct {
    HalGPIO_Port_e     port;           /**< GPIO port */
    uint8_t             pin;            /**< Pin number (0-15) */
    HalGPIO_Mode_e     mode;           /**< Pin mode */
    HalGPIO_Pull_e     pull;           /**< Pull configuration */
    HalGPIO_Speed_e    speed;          /**< Output speed */
    uint8_t             alternate;      /**< Alternate function (0-15) */
} HalGPIO_Config_t;

/**
 * @brief UART configuration structure
 */
typedef struct {
    uint8_t             instance;       /**< UART instance (0-7) */
    uint32_t            baudRate;       /**< Baud rate */
    uint32_t            wordLength;     /**< Word length (8 or 9) */
    HalUART_Parity_e   parity;         /**< Parity setting */
    HalUART_StopBits_e stopBits;       /**< Stop bits */
    bool                hwFlowControl;  /**< Hardware flow control */
    bool                oversampling;   /**< Oversampling (8 or 16) */
} HalUART_Config_t;

/**
 * @brief SPI configuration structure
 */
typedef struct {
    uint8_t             instance;       /**< SPI instance (0-3) */
    HalSPI_Mode_e      mode;           /**< Master/Slave mode */
    uint32_t            clockSpeed;     /**< Clock speed in Hz */
    uint8_t             dataSize;       /**< Data frame size (4-16) */
    HalSPI_CPOL_e      clockPolarity;  /**< Clock polarity */
    HalSPI_CPHA_e      clockPhase;     /**< Clock phase */
    bool                firstBitMSB;    /**< MSB first if true */
    bool                ssOutput;       /**< SS output enable */
} HalSPI_Config_t;

/**
 * @brief I2C configuration structure
 */
typedef struct {
    uint8_t                 instance;       /**< I2C instance (0-3) */
    HalI2C_AddressingMode_e addressingMode; /**< Addressing mode */
    uint32_t                clockSpeed;     /**< Clock speed in Hz */
    uint16_t                ownAddress;     /**< Own slave address */
    bool                    generalCall;    /**< General call enable */
    bool                    noStretch;      /**< Clock stretching disable */
} HalI2C_Config_t;

/**
 * @brief Timer configuration structure
 */
typedef struct {
    uint8_t             instance;       /**< Timer instance (0-7) */
    HalTIMER_Mode_e    counterMode;    /**< Counter mode */
    uint32_t            period;         /**< Auto-reload period */
    uint32_t            prescaler;      /**< Prescaler value */
    uint32_t            clockDivision;  /**< Clock division */
    bool                autoReloadPreload; /**< Auto-reload preload */
} HalTIMER_Config_t;

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
} HalADC_Config_t;

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
} HalDMA_Config_t;

/**
 * @brief System clock configuration structure
 */
typedef struct {
    HalClockSource_e   source;         /**< Clock source */
    uint32_t            sysClock;       /**< System clock frequency (Hz) */
    uint32_t            ahbPrescaler;   /**< AHB prescaler */
    uint32_t            apb1Prescaler;  /**< APB1 prescaler */
    uint32_t            apb2Prescaler;  /**< APB2 prescaler */
    bool                usePLL;         /**< Use PLL */
    uint8_t             pllM;           /**< PLL M divider */
    uint8_t             pllN;           /**< PLL N multiplier */
    uint8_t             pllP;           /**< PLL P divider */
    uint8_t             pllQ;           /**< PLL Q divider */
} HalClockConfig_t;

/*============================================================================*/
/*                              CALLBACK TYPES                                */
/*============================================================================*/

/**
 * @brief GPIO interrupt callback
 */
typedef void (*HalGPIO_Callback_t)(HalGPIO_Port_e port, uint8_t pin);

/**
 * @brief UART receive callback
 */
typedef void (*HalUART_RxCallback_t)(uint8_t instance, uint8_t data);

/**
 * @brief UART transmit complete callback
 */
typedef void (*HalUART_TxCallback_t)(uint8_t instance);

/**
 * @brief Timer overflow callback
 */
typedef void (*HalTIMER_Callback_t)(uint8_t instance);

/*============================================================================*/
/*                              HAL INITIALIZATION                            */
/*============================================================================*/

/**
 * @brief Initialize the HAL layer
 * 
 * @return HalOK on success
 */
HalStatus_e HalInit(void);

/**
 * @brief Deinitialize the HAL layer
 */
void HalDeinit(void);

/**
 * @brief Configure system clocks
 * 
 * @param config Clock configuration
 * @return HalOK on success
 */
HalStatus_e HalClockConfig(const HalClockConfig_t *config);

/**
 * @brief Get system clock frequency
 * 
 * @return System clock frequency in Hz
 */
uint32_t HalGetSystemClock(void);

/**
 * @brief Enable global interrupts
 */
void HalEnableIRQ(void);

/**
 * @brief Disable global interrupts
 */
void HalDisableIRQ(void);

/**
 * @brief Get tick count (milliseconds since boot)
 * 
 * @return Tick count in milliseconds
 */
uint32_t HalGetTick(void);

/**
 * @brief Delay for specified milliseconds
 * 
 * @param ms Delay in milliseconds
 */
void HalDelay(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* HalLAYER_H */
