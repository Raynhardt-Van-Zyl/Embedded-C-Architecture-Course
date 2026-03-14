/**
 * @file hal_gpio.h
 * @brief GPIO Hardware Abstraction Layer for STM32F401RE
 * 
 * This module provides direct register-level access to GPIO peripherals.
 * All operations use atomic BSRR register for thread-safe pin manipulation.
 * 
 * GPIO Register Map (per port):
 *   MODER     - Mode register (input/output/alternate/analog)
 *   OTYPER    - Output type register (push-pull/open-drain)
 *   OSPEEDR   - Output speed register (low/medium/high/very-high)
 *   PUPDR     - Pull-up/pull-down register
 *   IDR       - Input data register (read only)
 *   ODR       - Output data register (read/write)
 *   BSRR      - Bit set/reset register (atomic, write only)
 *   LCKR      - Configuration lock register
 *   AFRL/AFRH - Alternate function registers (low/high pins)
 * 
 * Clock Domains:
 *   - All GPIO ports are on AHB1 bus
 *   - GPIOA-E clock enable bits in RCC->AHB1ENR
 * 
 * @author Embedded Systems Course
 * @date 2024
 */

#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/*                          CONFIGURATION CONSTANTS                           */
/*============================================================================*/

/**
 * GPIO Port Base Addresses
 * Each GPIO port has a 1KB address space in the peripheral region
 * Base addresses from STM32F401 Reference Manual (RM0368)
 */
#define GPIOA_BASE          (0x40020000UL)
#define GPIOB_BASE          (0x40020400UL)
#define GPIOC_BASE          (0x40020800UL)
#define GPIOD_BASE          (0x40020C00UL)
#define GPIOE_BASE          (0x40021000UL)
#define GPIOH_BASE          (0x40021C00UL)

/**
 * RCC Base Address for clock control
 */
#define RCC_BASE            (0x40023800UL)

/*============================================================================*/
/*                          REGISTER DEFINITIONS                               */
/*============================================================================*/

/**
 * GPIO Register Structure
 * Offsets from Reference Manual:
 *   MODER:   0x00 - Port mode register
 *   OTYPER:  0x04 - Port output type register
 *   OSPEEDR: 0x08 - Port output speed register
 *   PUPDR:   0x0C - Port pull-up/pull-down register
 *   IDR:     0x10 - Port input data register
 *   ODR:     0x14 - Port output data register
 *   BSRR:    0x18 - Port bit set/reset register
 *   LCKR:    0x1C - Port configuration lock register
 *   AFRL:    0x20 - Alternate function low register (pins 0-7)
 *   AFRH:    0x24 - Alternate function high register (pins 8-15)
 */
typedef struct {
    volatile uint32_t MODER;        /* Offset 0x00: Mode register, 2 bits per pin */
    volatile uint32_t OTYPER;       /* Offset 0x04: Output type, 1 bit per pin */
    volatile uint32_t OSPEEDR;      /* Offset 0x08: Output speed, 2 bits per pin */
    volatile uint32_t PUPDR;        /* Offset 0x0C: Pull-up/pull-down, 2 bits per pin */
    volatile uint32_t IDR;          /* Offset 0x10: Input data, read-only */
    volatile uint32_t ODR;          /* Offset 0x14: Output data, read/write */
    volatile uint32_t BSRR;         /* Offset 0x18: Bit set/reset, write-only */
    volatile uint32_t LCKR;         /* Offset 0x1C: Configuration lock */
    volatile uint32_t AFRL;         /* Offset 0x20: Alternate function low (pins 0-7) */
    volatile uint32_t AFRH;         /* Offset 0x24: Alternate function high (pins 8-15) */
} GPIO_Regs_t;

/**
 * RCC Register Structure (partial - only AHB1ENR for GPIO clocks)
 */
typedef struct {
    volatile uint32_t CR;           /* Offset 0x00: Clock control register */
    volatile uint32_t PLLCFGR;      /* Offset 0x04: PLL configuration */
    volatile uint32_t CFGR;         /* Offset 0x08: Clock configuration */
    volatile uint32_t CIR;          /* Offset 0x0C: Clock interrupt */
    uint8_t          reserved[0x24];
    volatile uint32_t AHB1ENR;      /* Offset 0x30: AHB1 peripheral clock enable */
    volatile uint32_t AHB2ENR;      /* Offset 0x34: AHB2 peripheral clock enable */
    volatile uint32_t AHB3ENR;      /* Offset 0x38: AHB3 peripheral clock enable */
    uint8_t          reserved2[0x04];
    volatile uint32_t APB1ENR;      /* Offset 0x40: APB1 peripheral clock enable */
    volatile uint32_t APB2ENR;      /* Offset 0x44: APB2 peripheral clock enable */
} RCC_Regs_t;

/* Peripheral pointer definitions */
#define GPIOA               ((GPIO_Regs_t *)GPIOA_BASE)
#define GPIOB               ((GPIO_Regs_t *)GPIOB_BASE)
#define GPIOC               ((GPIO_Regs_t *)GPIOC_BASE)
#define GPIOD               ((GPIO_Regs_t *)GPIOD_BASE)
#define GPIOE               ((GPIO_Regs_t *)GPIOE_BASE)
#define GPIOH               ((GPIO_Regs_t *)GPIOH_BASE)
#define RCC                 ((RCC_Regs_t *)RCC_BASE)

/*============================================================================*/
/*                          ENUMERATED TYPES                                   */
/*============================================================================*/

/**
 * GPIO Pin Mode Configuration
 * MODER register bits [2n+1:2n] for pin n
 * 
 * 00: Input mode (reset state after peripheral reset)
 * 01: General purpose output mode
 * 10: Alternate function mode (selected via AFRL/AFRH)
 * 11: Analog mode (lowest power, Schmitt trigger disabled)
 */
typedef enum {
    GPIO_MODE_INPUT     = 0x00,     /* Input floating (external pull required) */
    GPIO_MODE_OUTPUT    = 0x01,     /* General purpose output */
    GPIO_MODE_ALTERNATE = 0x02,     /* Alternate function (UART, SPI, I2C, etc.) */
    GPIO_MODE_ANALOG    = 0x03      /* Analog input/output or ADC/DAC */
} GPIO_Mode_t;

/**
 * GPIO Output Type Configuration
 * OTYPER register bit n for pin n
 * 
 * 0: Push-pull output (can drive high and low)
 *    - Use for most digital outputs
 *    - Strong drive in both directions
 * 
 * 1: Open-drain output (can only drive low)
 *    - Requires external pull-up resistor
 *    - Use for I2C, shared lines, level shifting
 */
typedef enum {
    GPIO_OTYPE_PUSHPULL  = 0x00,    /* Push-pull output (default) */
    GPIO_OTYPE_OPENDRAIN = 0x01     /* Open-drain output */
} GPIO_OType_t;

/**
 * GPIO Output Speed Configuration
 * OSPEEDR register bits [2n+1:2n] for pin n
 * 
 * Speed affects slew rate and EMI:
 *   - Lower speed = less EMI, lower power
 *   - Higher speed = faster edges, more EMI
 * 
 * Choose based on required toggle frequency:
 *   - Low (2MHz): Good for LED, slow signals
 *   - Medium (10MHz): General purpose
 *   - High (25MHz): SPI, fast GPIO
 *   - Very High (50MHz): High-speed interfaces
 */
typedef enum {
    GPIO_SPEED_LOW       = 0x00,    /* Low speed (2 MHz) */
    GPIO_SPEED_MEDIUM    = 0x01,    /* Medium speed (10 MHz) */
    GPIO_SPEED_HIGH      = 0x02,    /* High speed (25 MHz) */
    GPIO_SPEED_VERYHIGH  = 0x03     /* Very high speed (50 MHz) */
} GPIO_Speed_t;

/**
 * GPIO Pull-up/Pull-down Configuration
 * PUPDR register bits [2n+1:2n] for pin n
 * 
 * Internal pull resistors are approximately 40kΩ (typical)
 * 
 * Use cases:
 *   - Input with button: PULLUP (active low button)
 *   - Floating input: NONE (external circuit drives)
 *   - Unused pins: PULLDOWN or ANALOG mode (lowest power)
 */
typedef enum {
    GPIO_PULL_NONE  = 0x00,         /* No pull-up or pull-down */
    GPIO_PULL_UP    = 0x01,         /* Pull-up resistor enabled */
    GPIO_PULL_DOWN  = 0x02          /* Pull-down resistor enabled */
} GPIO_Pull_t;

/**
 * GPIO Alternate Function Selection
 * Values for AFRL/AFRH registers (4 bits per pin)
 * 
 * AF0:  System (JTAG/SWD, RTC)
 * AF1:  TIM1/TIM2
 * AF2:  TIM3/TIM4/TIM5
 * AF4:  I2C1/I2C2/I2C3
 * AF5:  SPI1/SPI2
 * AF6:  SPI3
 * AF7:  USART1/USART2/USART3
 * AF8:  UART4/UART5
 * 
 * See datasheet pinout and alternate function mapping table
 */
typedef enum {
    GPIO_AF0  = 0x00,   /* System (JTAG/SWD on PA13-15, PB3-4) */
    GPIO_AF1  = 0x01,   /* TIM1, TIM2 */
    GPIO_AF2  = 0x02,   /* TIM3, TIM4, TIM5 */
    GPIO_AF3  = 0x03,   /* TIM9, TIM10, TIM11 */
    GPIO_AF4  = 0x04,   /* I2C1, I2C2, I2C3 */
    GPIO_AF5  = 0x05,   /* SPI1, SPI2 */
    GPIO_AF6  = 0x06,   /* SPI3 */
    GPIO_AF7  = 0x07,   /* USART1, USART2, USART3 */
    GPIO_AF8  = 0x08,   /* UART4, UART5 */
    GPIO_AF9  = 0x09,   /* Reserved */
    GPIO_AF10 = 0x0A,   /* Reserved */
    GPIO_AF11 = 0x0B,   /* Reserved */
    GPIO_AF12 = 0x0C,   /* Reserved */
    GPIO_AF13 = 0x0D,   /* Reserved */
    GPIO_AF14 = 0x0E,   /* Reserved */
    GPIO_AF15 = 0x0F    /* Eventout (Cortex-M4 EVENT signal) */
} GPIO_AF_t;

/**
 * GPIO Pin State
 * Logical high/low for readability
 */
typedef enum {
    GPIO_PIN_RESET = 0x00U,         /* Pin logic low (0V) */
    GPIO_PIN_SET   = 0x01U          /* Pin logic high (VDD) */
} GPIO_PinState_t;

/*============================================================================*/
/*                          PIN NUMBER DEFINITIONS                             */
/*============================================================================*/

/**
 * Single Pin Bit Masks
 * Use these for pin parameter in functions
 * Can be OR'd together for multi-pin operations on same port
 */
#define GPIO_PIN_0      (1U << 0)
#define GPIO_PIN_1      (1U << 1)
#define GPIO_PIN_2      (1U << 2)
#define GPIO_PIN_3      (1U << 3)
#define GPIO_PIN_4      (1U << 4)
#define GPIO_PIN_5      (1U << 5)
#define GPIO_PIN_6      (1U << 6)
#define GPIO_PIN_7      (1U << 7)
#define GPIO_PIN_8      (1U << 8)
#define GPIO_PIN_9      (1U << 9)
#define GPIO_PIN_10     (1U << 10)
#define GPIO_PIN_11     (1U << 11)
#define GPIO_PIN_12     (1U << 12)
#define GPIO_PIN_13     (1U << 13)
#define GPIO_PIN_14     (1U << 14)
#define GPIO_PIN_15     (1U << 15)
#define GPIO_PIN_ALL    (0xFFFFU)

/*============================================================================*/
/*                          RCC CLOCK ENABLE BITS                              */
/*============================================================================*/

/**
 * RCC AHB1ENR GPIO Clock Enable Bits
 * Each GPIO port must have its clock enabled before use
 * 
 * Bit positions in AHB1ENR:
 *   Bit 0: GPIOAEN
 *   Bit 1: GPIOBEN
 *   Bit 2: GPIOCEN
 *   Bit 3: GPIODEN
 *   Bit 4: GPIOEEN
 *   Bit 7: GPIOHEN
 */
#define RCC_AHB1ENR_GPIOAEN    (1U << 0)
#define RCC_AHB1ENR_GPIOBEN    (1U << 1)
#define RCC_AHB1ENR_GPIOCEN    (1U << 2)
#define RCC_AHB1ENR_GPIODEN    (1U << 3)
#define RCC_AHB1ENR_GPIOEEN    (1U << 4)
#define RCC_AHB1ENR_GPIOHEN    (1U << 7)

/*============================================================================*/
/*                          PUBLIC FUNCTION PROTOTYPES                         */
/*============================================================================*/

/**
 * @brief Enable GPIO port clock
 * 
 * Must be called before any GPIO operations on the port.
 * Accessing GPIO registers without clock enabled causes HardFault.
 * 
 * @param port Pointer to GPIO port (GPIOA, GPIOB, etc.)
 */
void HAL_GPIO_ClockEnable(GPIO_Regs_t *port);

/**
 * @brief Initialize GPIO pin with full configuration
 * 
 * Configures all aspects of a GPIO pin:
 *   1. Enables port clock if not already enabled
 *   2. Sets mode (input/output/alternate/analog)
 *   3. Sets output type (push-pull/open-drain)
 *   4. Sets speed (affects slew rate)
 *   5. Sets pull resistors
 *   6. Sets alternate function if mode == GPIO_MODE_ALTERNATE
 * 
 * Register modifications:
 *   - MODER[2n+1:2n]   = mode
 *   - OTYPER[n]        = otype
 *   - OSPEEDR[2n+1:2n] = speed
 *   - PUPDR[2n+1:2n]   = pull
 *   - AFRL/AFRH        = af (if alternate mode)
 * 
 * @param port   Pointer to GPIO port (GPIOA, GPIOB, etc.)
 * @param pin    Pin number bitmask (GPIO_PIN_0 to GPIO_PIN_15)
 * @param mode   Pin mode (input/output/alternate/analog)
 * @param otype  Output type (push-pull/open-drain)
 * @param speed  Output speed (low/medium/high/very-high)
 * @param pull   Pull resistor configuration
 * @param af     Alternate function (only used if mode == GPIO_MODE_ALTERNATE)
 */
void HAL_GPIO_Init(GPIO_Regs_t *port, uint16_t pin, GPIO_Mode_t mode,
                   GPIO_OType_t otype, GPIO_Speed_t speed, 
                   GPIO_Pull_t pull, GPIO_AF_t af);

/**
 * @brief Set GPIO pin high (VDD)
 * 
 * Uses BSRR register for atomic operation - no read-modify-write.
 * BSRR bits [15:0] set corresponding ODR bits.
 * This is interrupt-safe and can be used in ISRs.
 * 
 * @param port Pointer to GPIO port
 * @param pin  Pin number bitmask
 */
void HAL_GPIO_Set(GPIO_Regs_t *port, uint16_t pin);

/**
 * @brief Set GPIO pin low (GND)
 * 
 * Uses BSRR register for atomic operation.
 * BSRR bits [31:16] reset corresponding ODR bits.
 * Writing 1 to bit 16+n clears ODR bit n.
 * 
 * @param port Pointer to GPIO port
 * @param pin  Pin number bitmask
 */
void HAL_GPIO_Reset(GPIO_Regs_t *port, uint16_t pin);

/**
 * @brief Toggle GPIO pin output
 * 
 * Performs XOR on ODR register to toggle pin state.
 * Note: This is NOT atomic - use with caution in interrupts.
 * For atomic toggle in ISR context, read IDR and use BSRR.
 * 
 * @param port Pointer to GPIO port
 * @param pin  Pin number bitmask
 */
void HAL_GPIO_Toggle(GPIO_Regs_t *port, uint16_t pin);

/**
 * @brief Read GPIO pin input state
 * 
 * Reads the IDR register which reflects the logic level on the pin.
 * For output pins, this reads the actual pin state (not ODR).
 * 
 * @param port Pointer to GPIO port
 * @param pin  Pin number bitmask
 * @return GPIO_PIN_SET if pin is high, GPIO_PIN_RESET if low
 */
GPIO_PinState_t HAL_GPIO_Read(GPIO_Regs_t *port, uint16_t pin);

/**
 * @brief Write GPIO pin output state
 * 
 * Convenience function that calls Set or Reset based on state.
 * 
 * @param port  Pointer to GPIO port
 * @param pin   Pin number bitmask
 * @param state Desired pin state (GPIO_PIN_SET or GPIO_PIN_RESET)
 */
void HAL_GPIO_Write(GPIO_Regs_t *port, uint16_t pin, GPIO_PinState_t state);

#ifdef __cplusplus
}
#endif

#endif /* HAL_GPIO_H */
