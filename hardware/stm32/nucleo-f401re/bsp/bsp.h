/**
 * @file    bsp.h
 * @brief   Board Support Package Header for STM32 Nucleo-F401RE
 * @details This header provides hardware abstractions and pin definitions
 *          for the Nucleo-F401RE development board. It defines:
 *          - LED pin (LD2 - Green LED on PA5)
 *          - User button pin (B1 - Blue button on PC13)
 *          - UART pins for USB virtual COM port (USART2)
 *          - I2C pins for external sensors (I2C1)
 * 
 * @note    All pin assignments match the Nucleo-F401RE board schematic.
 *          The virtual COM port is connected to USART2 via the ST-LINK.
 * 
 * @author  Embedded C Architecture Course
 * @version 1.0
 */

#ifndef BSP_H
#define BSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32f401xe.h"

/*============================================================================*/
/*                          CLOCK CONFIGURATION                                */
/*============================================================================*/

/**
 * @defgroup BSP_Clock BSP Clock Configuration
 * @brief    System clock configuration constants for STM32F401RE
 * @{
 */

/**
 * @brief   External High Speed Clock (HSE) frequency
 * @details The Nucleo-F401RE uses the ST-LINK MCO output as HSE source,
 *          which provides an 8 MHz clock signal.
 */
#define BSP_HSE_CLOCK_HZ            (8000000UL)

/**
 * @brief   Internal High Speed Clock (HSI) frequency
 * @details The HSI is an internal 16 MHz RC oscillator.
 *          Used as fallback when HSE is not available.
 */
#define BSP_HSI_CLOCK_HZ            (16000000UL)

/**
 * @brief   Main PLL output clock (System Clock)
 * @details PLL configuration for 84 MHz system clock:
 *          - PLLM = 8 (HSE/8 = 1 MHz, valid input 1-2 MHz)
 *          - PLLN = 336 (VCO = 1 MHz * 336 = 336 MHz, valid 100-432 MHz)
 *          - PLLP = 4 (System Clock = 336/4 = 84 MHz)
 *          - PLLQ = 7 (USB/SDIO = 336/7 = 48 MHz for USB)
 */
#define BSP_SYS_CLOCK_HZ            (84000000UL)

/**
 * @brief   AHB bus clock frequency
 * @details AHB is derived from SYSCLK with HPRE prescaler.
 *          HPRE = 1, so AHB = SYSCLK = 84 MHz.
 *          This clocks: GPIO, DMA, CRC, Flash memory interface
 */
#define BSP_AHB_CLOCK_HZ            (84000000UL)

/**
 * @brief   APB1 bus clock frequency
 * @details APB1 is derived from AHB with PPRE1 prescaler.
 *          PPRE1 = 2 (APB1 = AHB/2 = 42 MHz).
 *          Maximum APB1 frequency is 42 MHz for STM32F401.
 *          This clocks: TIM2-5, I2C1-3, USART2, etc.
 * @note    Timer clocks on APB1 are multiplied by 2 when APB1 prescaler > 1,
 *          so TIM2-5 run at 84 MHz.
 */
#define BSP_APB1_CLOCK_HZ           (42000000UL)

/**
 * @brief   APB2 bus clock frequency
 * @details APB2 is derived from AHB with PPRE2 prescaler.
 *          PPRE2 = 1 (APB2 = AHB = 84 MHz).
 *          Maximum APB2 frequency is 84 MHz for STM32F401.
 *          This clocks: TIM1, TIM9-11, USART1, USART6, SPI1, ADC, etc.
 * @note    Timer clocks on APB2 are NOT multiplied when APB2 prescaler = 1,
 *          so TIM1, TIM9-11 run at 84 MHz.
 */
#define BSP_APB2_CLOCK_HZ           (84000000UL)

/** @} */

/*============================================================================*/
/*                          LED CONFIGURATION                                  */
/*============================================================================*/

/**
 * @defgroup BSP_LED LED Configuration
 * @brief    On-board LED definitions for Nucleo-F401RE
 * @{
 */

/**
 * @brief   LED GPIO port
 * @details LD2 (green LED) is connected to GPIOA on the Nucleo-F401RE.
 *          The LED is active-high (writing 1 turns it ON).
 */
#define BSP_LED_PORT                GPIOA

/**
 * @brief   LED GPIO pin number
 * @details LD2 is connected to PA5 (pin 5 of GPIO port A).
 */
#define BSP_LED_PIN                 (5U)

/**
 * @brief   LED GPIO pin mask
 * @details Bit mask for bit-band or BSRR register access.
 */
#define BSP_LED_PIN_MASK            (1UL << BSP_LED_PIN)

/**
 * @brief   LED RCC clock enable bit
 * @details This bit in RCC_AHB1ENR enables the GPIOA peripheral clock.
 *          Must be set before accessing GPIOA registers.
 */
#define BSP_LED_RCC_ENABLE          RCC_AHB1ENR_GPIOAEN

/** @} */

/*============================================================================*/
/*                          USER BUTTON CONFIGURATION                          */
/*============================================================================*/

/**
 * @defgroup BSP_Button User Button Configuration
 * @brief    On-board user button definitions for Nucleo-F401RE
 * @{
 */

/**
 * @brief   User button GPIO port
 * @details B1 (blue user button) is connected to GPIOC on the Nucleo-F401RE.
 *          The button is active-low (pressed = logic 0).
 */
#define BSP_BUTTON_PORT             GPIOC

/**
 * @brief   User button GPIO pin number
 * @details B1 is connected to PC13 (pin 13 of GPIO port C).
 */
#define BSP_BUTTON_PIN              (13U)

/**
 * @brief   User button GPIO pin mask
 * @details Bit mask for reading button state.
 */
#define BSP_BUTTON_PIN_MASK         (1UL << BSP_BUTTON_PIN)

/**
 * @brief   User button RCC clock enable bit
 * @details This bit in RCC_AHB1ENR enables the GPIOC peripheral clock.
 */
#define BSP_BUTTON_RCC_ENABLE       RCC_AHB1ENR_GPIOCEN

/** @} */

/*============================================================================*/
/*                          UART CONFIGURATION                                 */
/*============================================================================*/

/**
 * @defgroup BSP_UART UART Configuration (Virtual COM Port)
 * @brief    UART pin definitions for USB virtual COM port via ST-LINK
 * @{
 */

/**
 * @brief   UART peripheral instance
 * @details USART2 is connected to the ST-LINK virtual COM port.
 *          Communication with PC via USB CDC (Virtual Serial Port).
 */
#define BSP_UART                    USART2

/**
 * @brief   UART TX GPIO port
 * @details USART2_TX is on GPIOA (PA2).
 */
#define BSP_UART_TX_PORT            GPIOA

/**
 * @brief   UART TX GPIO pin number
 * @details USART2_TX is on PA2 (pin 2 of GPIO port A).
 */
#define BSP_UART_TX_PIN             (2U)

/**
 * @brief   UART TX alternate function number
 * @details AF7 is the alternate function for USART1/2/3 on these pins.
 */
#define BSP_UART_TX_AF              (7U)

/**
 * @brief   UART RX GPIO port
 * @details USART2_RX is on GPIOA (PA3).
 */
#define BSP_UART_RX_PORT            GPIOA

/**
 * @brief   UART RX GPIO pin number
 * @details USART2_RX is on PA3 (pin 3 of GPIO port A).
 */
#define BSP_UART_RX_PIN             (3U)

/**
 * @brief   UART RX alternate function number
 * @details AF7 is the alternate function for USART1/2/3 on these pins.
 */
#define BSP_UART_RX_AF              (7U)

/**
 * @brief   UART RCC clock enable bit (peripheral)
 * @details This bit in RCC_APB1ENR enables the USART2 peripheral clock.
 */
#define BSP_UART_RCC_ENABLE         RCC_APB1ENR_USART2EN

/**
 * @brief   UART GPIO port RCC clock enable bit
 * @details This bit in RCC_AHB1ENR enables the GPIOA peripheral clock.
 */
#define BSP_UART_GPIO_RCC_ENABLE    RCC_AHB1ENR_GPIOAEN

/**
 * @brief   Default UART baud rate
 * @details 115200 baud is the standard baud rate for most applications.
 */
#define BSP_UART_BAUD_RATE          (115200UL)

/**
 * @brief   UART clock frequency for baud rate calculation
 * @details USART2 is on APB1 bus, so clock is APB1 frequency (42 MHz).
 */
#define BSP_UART_CLOCK_HZ           (BSP_APB1_CLOCK_HZ)

/** @} */

/*============================================================================*/
/*                          I2C CONFIGURATION                                  */
/*============================================================================*/

/**
 * @defgroup BSP_I2C I2C Configuration
 * @brief    I2C pin definitions for external sensor communication
 * @{
 */

/**
 * @brief   I2C peripheral instance
 * @details I2C1 is available on the Arduino header pins.
 */
#define BSP_I2C                     I2C1

/**
 * @brief   I2C SCL GPIO port
 * @details I2C1_SCL is on GPIOB (PB6) - matches Arduino SCL pin.
 */
#define BSP_I2C_SCL_PORT            GPIOB

/**
 * @brief   I2C SCL GPIO pin number
 * @details I2C1_SCL is on PB6 (pin 6 of GPIO port B).
 */
#define BSP_I2C_SCL_PIN             (6U)

/**
 * @brief   I2C SCL alternate function number
 * @details AF4 is the alternate function for I2C1/2/3 on these pins.
 */
#define BSP_I2C_SCL_AF              (4U)

/**
 * @brief   I2C SDA GPIO port
 * @details I2C1_SDA is on GPIOB (PB7) - matches Arduino SDA pin.
 */
#define BSP_I2C_SDA_PORT            GPIOB

/**
 * @brief   I2C SDA GPIO pin number
 * @details I2C1_SDA is on PB7 (pin 7 of GPIO port B).
 */
#define BSP_I2C_SDA_PIN             (7U)

/**
 * @brief   I2C SDA alternate function number
 * @details AF4 is the alternate function for I2C1/2/3 on these pins.
 */
#define BSP_I2C_SDA_AF              (4U)

/**
 * @brief   I2C RCC clock enable bit (peripheral)
 * @details This bit in RCC_APB1ENR enables the I2C1 peripheral clock.
 */
#define BSP_I2C_RCC_ENABLE          RCC_APB1ENR_I2C1EN

/**
 * @brief   I2C GPIO port RCC clock enable bit
 * @details This bit in RCC_AHB1ENR enables the GPIOB peripheral clock.
 */
#define BSP_I2C_GPIO_RCC_ENABLE     RCC_AHB1ENR_GPIOBEN

/**
 * @brief   I2C clock frequency
 * @details I2C1 is on APB1 bus, so clock is APB1 frequency (42 MHz).
 */
#define BSP_I2C_CLOCK_HZ            (BSP_APB1_CLOCK_HZ)

/**
 * @brief   Default I2C clock speed (100 kHz standard mode)
 */
#define BSP_I2C_SPEED_HZ            (100000UL)

/** @} */

/*============================================================================*/
/*                          SPI CONFIGURATION                                  */
/*============================================================================*/

/**
 * @defgroup BSP_SPI SPI Configuration
 * @brief    SPI pin definitions for external device communication
 * @{
 */

/**
 * @brief   SPI peripheral instance
 * @details SPI1 is available on the Arduino header pins.
 */
#define BSP_SPI                     SPI1

/**
 * @brief   SPI SCK GPIO port
 * @details SPI1_SCK is on GPIOA (PA5) - shares pin with LED!
 * @note    PA5 is also the LED pin, so using SPI1 conflicts with LED.
 *          Consider using SPI2 or SPI3 for applications requiring both.
 */
#define BSP_SPI_SCK_PORT            GPIOA

/**
 * @brief   SPI SCK GPIO pin number
 * @details SPI1_SCK is on PA5 (pin 5 of GPIO port A).
 */
#define BSP_SPI_SCK_PIN             (5U)

/**
 * @brief   SPI SCK alternate function number
 * @details AF5 is the alternate function for SPI1 on PA5.
 */
#define BSP_SPI_SCK_AF              (5U)

/**
 * @brief   SPI MISO GPIO port
 * @details SPI1_MISO is on GPIOA (PA6).
 */
#define BSP_SPI_MISO_PORT           GPIOA

/**
 * @brief   SPI MISO GPIO pin number
 * @details SPI1_MISO is on PA6 (pin 6 of GPIO port A).
 */
#define BSP_SPI_MISO_PIN            (6U)

/**
 * @brief   SPI MISO alternate function number
 * @details AF5 is the alternate function for SPI1 on PA6.
 */
#define BSP_SPI_MISO_AF             (5U)

/**
 * @brief   SPI MOSI GPIO port
 * @details SPI1_MOSI is on GPIOA (PA7).
 */
#define BSP_SPI_MOSI_PORT           GPIOA

/**
 * @brief   SPI MOSI GPIO pin number
 * @details SPI1_MOSI is on PA7 (pin 7 of GPIO port A).
 */
#define BSP_SPI_MOSI_PIN            (7U)

/**
 * @brief   SPI MOSI alternate function number
 * @details AF5 is the alternate function for SPI1 on PA7.
 */
#define BSP_SPI_MOSI_AF             (5U)

/**
 * @brief   SPI RCC clock enable bit (peripheral)
 * @details This bit in RCC_APB2ENR enables the SPI1 peripheral clock.
 */
#define BSP_SPI_RCC_ENABLE          RCC_APB2ENR_SPI1EN

/**
 * @brief   SPI GPIO port RCC clock enable bit
 * @details This bit in RCC_AHB1ENR enables the GPIOA peripheral clock.
 */
#define BSP_SPI_GPIO_RCC_ENABLE     RCC_AHB1ENR_GPIOAEN

/** @} */

/*============================================================================*/
/*                          TIMER CONFIGURATION                                */
/*============================================================================*/

/**
 * @defgroup BSP_Timer Timer Configuration
 * @brief    Timer peripheral definitions
 * @{
 */

/**
 * @brief   System tick timer instance
 * @details SysTick is used for RTOS tick or simple delay functions.
 */
#define BSP_SYSTICK                 SysTick

/**
 * @brief   General purpose timer for delays
 * @details TIM2 is a 32-bit general-purpose timer on APB1.
 */
#define BSP_DELAY_TIMER             TIM2

/**
 * @brief   Delay timer RCC clock enable bit
 * @details This bit in RCC_APB1ENR enables the TIM2 peripheral clock.
 */
#define BSP_DELAY_TIMER_RCC_ENABLE  RCC_APB1ENR_TIM2EN

/**
 * @brief   Delay timer clock frequency
 * @details TIM2 is on APB1, but when APB1 prescaler > 1,
 *          timer clock is multiplied by 2. So TIM2 runs at 84 MHz.
 */
#define BSP_DELAY_TIMER_CLOCK_HZ    (84000000UL)

/** @} */

/*============================================================================*/
/*                          FUNCTION PROTOTYPES                                */
/*============================================================================*/

/**
 * @defgroup BSP_Functions BSP Function Prototypes
 * @brief    Board initialization and control functions
 * @{
 */

/**
 * @brief   Initialize the Board Support Package
 * @details This function performs the following initialization:
 *          - Configures the system clock to 84 MHz using PLL
 *          - Enables clocks for GPIOA, GPIOB, GPIOC peripherals
 *          - Configures LED pin (PA5) as output
 *          - Configures User Button pin (PC13) as input with pull-up
 *          - Configures UART2 pins (PA2/PA3) for virtual COM port
 *          - Configures I2C1 pins (PB6/PB7) for external sensors
 * 
 * @note    This function must be called before any other BSP functions.
 *          It does NOT initialize the UART or I2C peripherals themselves,
 *          only the GPIO pins. Use BSP_UART_Init() and BSP_I2C_Init()
 *          separately to initialize the peripherals.
 */
void BSP_Init(void);

/**
 * @brief   Turn on the on-board LED (LD2)
 * @details Sets PA5 high, which illuminates the green LED.
 *          The LED is active-high on the Nucleo-F401RE.
 */
void BSP_LED_On(void);

/**
 * @brief   Turn off the on-board LED (LD2)
 * @details Sets PA5 low, which turns off the green LED.
 */
void BSP_LED_Off(void);

/**
 * @brief   Toggle the on-board LED (LD2)
 * @details Inverts the current state of PA5.
 *          If LED was on, it turns off. If off, it turns on.
 */
void BSP_LED_Toggle(void);

/**
 * @brief   Read the User Button state (B1)
 * @return  1 if button is pressed (active-low input reads 0)
 *          0 if button is not pressed
 * @note    The button is active-low, but this function returns
 *          logical 1 when pressed for convenience.
 */
uint8_t BSP_Button_Read(void);

/**
 * @brief   Initialize the UART peripheral for communication
 * @details Configures USART2 with:
 *          - 8 data bits
 *          - No parity
 *          - 1 stop bit
 *          - Baud rate as defined by BSP_UART_BAUD_RATE (default 115200)
 *          - TX and RX enabled
 * 
 * @note    Call BSP_Init() first to configure the GPIO pins.
 */
void BSP_UART_Init(void);

/**
 * @brief   Send a single byte over UART
 * @param   byte: The byte to transmit
 * @note    This function blocks until the transmit buffer is empty.
 */
void BSP_UART_PutChar(uint8_t byte);

/**
 * @brief   Receive a single byte from UART
 * @return  The received byte
 * @note    This function blocks until a byte is received.
 */
uint8_t BSP_UART_GetChar(void);

/**
 * @brief   Send a null-terminated string over UART
 * @param   str: Pointer to the null-terminated string to transmit
 */
void BSP_UART_PutString(const char *str);

/**
 * @brief   Check if UART data is available to read
 * @return  1 if data is available in receive buffer
 *          0 if no data is available
 */
uint8_t BSP_UART_DataAvailable(void);

/**
 * @brief   Initialize the I2C peripheral for communication
 * @details Configures I2C1 with:
 *          - Standard mode (100 kHz) or Fast mode (400 kHz)
 *          - 7-bit addressing mode
 *          - Clock stretching enabled
 * 
 * @note    Call BSP_Init() first to configure the GPIO pins.
 */
void BSP_I2C_Init(void);

/**
 * @brief   Simple blocking delay in milliseconds
 * @param   ms: Number of milliseconds to delay
 * @note    Uses a simple software loop. Accuracy depends on compiler
 *          optimization settings. For precise timing, use hardware timers.
 */
void BSP_Delay(uint32_t ms);

/** @} */

/*============================================================================*/
/*                          INLINE FUNCTIONS                                   */
/*============================================================================*/

/**
 * @brief   Get the current LED state
 * @return  1 if LED is on, 0 if LED is off
 */
static inline uint8_t BSP_LED_IsOn(void) {
    return (BSP_LED_PORT->ODR & BSP_LED_PIN_MASK) ? 1 : 0;
}

/**
 * @brief   Get the raw button GPIO state
 * @return  1 if button pin is high, 0 if low
 * @note    The button is active-low, so this returns 0 when pressed.
 */
static inline uint8_t BSP_Button_GetRaw(void) {
    return (BSP_BUTTON_PORT->IDR & BSP_BUTTON_PIN_MASK) ? 1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* BSP_H */
