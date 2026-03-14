/**
 * @file    bsp.c
 * @brief   Board Support Package Implementation for STM32 Nucleo-F401RE
 * @details This file implements the Board Support Package functions for
 *          bare-metal hardware access on the Nucleo-F401RE development board.
 *          
 *          All functions use direct register-level access with no HAL/LL library
 *          dependencies. This provides maximum control and minimum overhead.
 * 
 * @note    Every register write is commented to explain its purpose and the
 *          bit fields being configured. This serves as both documentation
 *          and an educational resource for bare-metal programming.
 * 
 * @author  Embedded C Architecture Course
 * @version 1.0
 */

#include "bsp.h"

/*============================================================================*/
/*                          PRIVATE MACROS                                     */
/*============================================================================*/

/**
 * @brief   Memory barrier for ARM Cortex-M
 * @details Ensures all memory accesses are completed before continuing.
 *          Used after peripheral register writes to ensure synchronization.
 */
#define BSP_MEMORY_BARRIER()        __asm__ volatile ("" ::: "memory")

/**
 * @brief   Wait for a register flag to be set
 * @param   REG:   Register to read
 * @param   FLAG:  Flag bit mask to check
 * @param   TIMEOUT: Loop counter for timeout (0 = no timeout)
 * @return  1 if flag was set, 0 if timeout occurred
 */
#define BSP_WAIT_FLAG_SET(REG, FLAG, TIMEOUT) \
    ({ \
        uint32_t _timeout = (TIMEOUT); \
        while (!((REG) & (FLAG))) { \
            if (_timeout > 0) { \
                if (--_timeout == 0) break; \
            } \
        } \
        ((REG) & (FLAG)) ? 1 : 0; \
    })

/*============================================================================*/
/*                          PRIVATE VARIABLES                                  */
/*============================================================================*/

/**
 * @brief   Millisecond counter for BSP_Delay
 * @details Updated by SysTick interrupt handler.
 *          Volatile because it's modified in interrupt context.
 */
static volatile uint32_t bsp_tick_count = 0;

/*============================================================================*/
/*                          PRIVATE FUNCTION PROTOTYPES                        */
/*============================================================================*/

static void BSP_SystemClock_Config(void);
static void BSP_GPIO_Init(void);
static void BSP_SysTick_Init(void);

/*============================================================================*/
/*                          PUBLIC FUNCTIONS                                   */
/*============================================================================*/

/**
 * @brief   Initialize the Board Support Package
 * @details This is the main initialization function that sets up:
 *          1. System clock (84 MHz from PLL with HSE as source)
 *          2. SysTick timer for millisecond delays
 *          3. GPIO pins for LED, Button, UART, and I2C
 * 
 * @note    Call this function at the start of main() before using any
 *          other BSP functions. The initialization order is important:
 *          - Clock must be configured first (affects peripheral timing)
 *          - SysTick depends on system clock
 *          - GPIO configuration depends on GPIO clocks being enabled
 */
void BSP_Init(void) {
    BSP_SystemClock_Config();
    BSP_SysTick_Init();
    BSP_GPIO_Init();
}

/*============================================================================*/
/*                          LED FUNCTIONS                                      */
/*============================================================================*/

/**
 * @brief   Turn on the on-board LED (LD2 - Green LED on PA5)
 * @details Uses the GPIO BSRR (Bit Set/Reset Register) for atomic operation.
 *          Writing to the lower 16 bits sets the corresponding ODR bits.
 *          This is more efficient than read-modify-write on ODR.
 */
void BSP_LED_On(void) {
    BSP_LED_PORT->BSRR = BSP_LED_PIN_MASK;
}

/**
 * @brief   Turn off the on-board LED (LD2 - Green LED on PA5)
 * @details Uses the GPIO BSRR (Bit Set/Reset Register) for atomic operation.
 *          Writing to the upper 16 bits resets the corresponding ODR bits.
 *          BSRR[31:16] controls reset, BSRR[15:0] controls set.
 */
void BSP_LED_Off(void) {
    BSP_LED_PORT->BSRR = (BSP_LED_PIN_MASK << 16);
}

/**
 * @brief   Toggle the on-board LED (LD2 - Green LED on PA5)
 * @details Reads current ODR value, XORs with LED mask, writes back.
 *          This toggles the LED state: on->off or off->on.
 * @note    This is a read-modify-write operation, not atomic like BSRR.
 *          In an interrupt context, you may want to disable interrupts
 *          briefly if the same GPIO port is used elsewhere.
 */
void BSP_LED_Toggle(void) {
    BSP_LED_PORT->ODR ^= BSP_LED_PIN_MASK;
}

/*============================================================================*/
/*                          BUTTON FUNCTIONS                                   */
/*============================================================================*/

/**
 * @brief   Read the User Button state (B1 - Blue button on PC13)
 * @return  1 if button is pressed, 0 if not pressed
 * @details The button is wired active-low (pulls to ground when pressed).
 *          The GPIO is configured with internal pull-up resistor.
 *          When pressed, IDR bit reads 0. We invert this for logical 1 = pressed.
 */
uint8_t BSP_Button_Read(void) {
    return (BSP_BUTTON_PORT->IDR & BSP_BUTTON_PIN_MASK) ? 0 : 1;
}

/*============================================================================*/
/*                          UART FUNCTIONS                                     */
/*============================================================================*/

/**
 * @brief   Initialize the UART peripheral for serial communication
 * @details Configures USART2 with:
 *          - Baud rate: 115200 (configurable via BSP_UART_BAUD_RATE)
 *          - Data bits: 8 (M bit = 0 in CR1)
 *          - Parity: None (PCE bit = 0 in CR1)
 *          - Stop bits: 1 (STOP bits = 00 in CR2)
 *          - Mode: TX and RX enabled (TE and RE bits in CR1)
 * 
 * @note    GPIO pins must be configured first (done in BSP_GPIO_Init).
 *          Baud rate calculation: BRR = fPCLK / (16 x BaudRate)
 *          For 42 MHz APB1 clock and 115200 baud:
 *          BRR = 42000000 / (16 x 115200) = 22.789 ≈ 22.8125
 *          DIV_Mantissa = 22 (0x16), DIV_Fraction = 13 (0.8125 * 16 ≈ 13)
 *          BRR = (22 << 4) | 13 = 0x16D
 */
void BSP_UART_Init(void) {
    uint32_t brr_value;
    
    brr_value = (BSP_UART_CLOCK_HZ + (BSP_UART_BAUD_RATE / 2)) / BSP_UART_BAUD_RATE;
    brr_value = brr_value / 16;
    
    BSP_UART->BRR = (brr_value << USART_BRR_DIV_MANTISSA_Pos) |
                    (((BSP_UART_CLOCK_HZ / BSP_UART_BAUD_RATE) % 16) << USART_BRR_DIV_FRACTION_Pos);
    
    BSP_UART->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

/**
 * @brief   Send a single byte over UART
 * @param   byte: The byte to transmit
 * @details Waits for TXE (Transmit Data Register Empty) flag, then writes
 *          the byte to the Data Register (DR). The UART hardware automatically
 *          shifts out the data on the TX line.
 */
void BSP_UART_PutChar(uint8_t byte) {
    while (!(BSP_UART->SR & USART_SR_TXE)) {
    }
    BSP_UART->DR = byte;
}

/**
 * @brief   Receive a single byte from UART
 * @return  The received byte
 * @details Waits for RXNE (Receive Data Register Not Empty) flag, then reads
 *          the byte from the Data Register (DR). Reading DR clears RXNE.
 */
uint8_t BSP_UART_GetChar(void) {
    while (!(BSP_UART->SR & USART_SR_RXNE)) {
    }
    return (uint8_t)(BSP_UART->DR & 0xFF);
}

/**
 * @brief   Send a null-terminated string over UART
 * @param   str: Pointer to the null-terminated string to transmit
 * @details Iterates through the string, sending each character via
 *          BSP_UART_PutChar(). Stops when null terminator is reached.
 */
void BSP_UART_PutString(const char *str) {
    while (*str != '\0') {
        if (*str == '\n') {
            BSP_UART_PutChar('\r');
        }
        BSP_UART_PutChar(*str++);
    }
}

/**
 * @brief   Check if UART data is available to read
 * @return  1 if data is available, 0 otherwise
 * @details Non-blocking check of RXNE flag. Useful for polled I/O
 *          where you don't want to wait for data.
 */
uint8_t BSP_UART_DataAvailable(void) {
    return (BSP_UART->SR & USART_SR_RXNE) ? 1 : 0;
}

/*============================================================================*/
/*                          I2C FUNCTIONS                                      */
/*============================================================================*/

/**
 * @brief   Initialize the I2C peripheral for communication
 * @details Configures I2C1 with:
 *          - Standard mode (100 kHz) or Fast mode (400 kHz)
 *          - 7-bit addressing
 *          - Clock stretching enabled
 * 
 *          I2C timing calculation for Standard Mode (100 kHz):
 *          - Thigh = CCR x TPCLK1
 *          - Tlow  = CCR x TPCLK1
 *          - For 100 kHz: T = 10 us, Thigh = Tlow = 5 us
 *          - CCR = 5 us / (1/42 MHz) = 210 (0xD2)
 *          
 *          TRISE calculation:
 *          - Maximum rise time for Standard Mode = 1000 ns
 *          - TRISE = (1000 ns / TPCLK1) + 1 = (1000 / 23.8) + 1 ≈ 43 (0x2B)
 */
void BSP_I2C_Init(void) {
    uint32_t ccr_value;
    uint32_t trise_value;
    
    RCC->APB1RSTR |= RCC_APB1ENR_I2C1EN;
    RCC->APB1RSTR &= ~RCC_APB1ENR_I2C1EN;
    
    BSP_I2C->CR1 &= ~I2C_CR1_PE;
    
    ccr_value = (BSP_I2C_CLOCK_HZ / (BSP_I2C_SPEED_HZ * 2));
    if (ccr_value < 4) {
        ccr_value = 4;
    }
    BSP_I2C->CCR = ccr_value;
    
    trise_value = (BSP_I2C_CLOCK_HZ / 1000000UL) + 1;
    BSP_I2C->TRISE = trise_value;
    
    BSP_I2C->CR1 |= I2C_CR1_PE;
}

/*============================================================================*/
/*                          DELAY FUNCTIONS                                    */
/*============================================================================*/

/**
 * @brief   Simple blocking delay in milliseconds
 * @param   ms: Number of milliseconds to delay
 * @details Uses the SysTick counter for timing. The counter is incremented
 *          by the SysTick interrupt handler every millisecond.
 * 
 * @note    This function is accurate only if SysTick interrupt has
 *          higher priority than the calling context. For delays in
 *          critical sections or ISRs, use a hardware timer instead.
 */
void BSP_Delay(uint32_t ms) {
    uint32_t start_tick = bsp_tick_count;
    while ((bsp_tick_count - start_tick) < ms) {
    }
}

/**
 * @brief   Get current tick count
 * @return  Current millisecond tick count
 * @note    This can be used for non-blocking timing patterns.
 */
uint32_t BSP_GetTick(void) {
    return bsp_tick_count;
}

/**
 * @brief   Increment tick count (called from SysTick ISR)
 * @note    This should be called from SysTick_Handler in your application.
 */
void BSP_IncrementTick(void) {
    bsp_tick_count++;
}

/*============================================================================*/
/*                          PRIVATE FUNCTIONS                                  */
/*============================================================================*/

/**
 * @brief   Configure the system clock to 84 MHz using PLL
 * @details This function configures the STM32F401RE clock tree:
 *          
 *          Clock Source: HSE (8 MHz from ST-LINK MCO)
 *          PLL Configuration:
 *            - PLLM = 8:  HSE/8 = 1 MHz PLL input
 *            - PLLN = 336: VCO = 1 MHz * 336 = 336 MHz
 *            - PLLP = 4:   SYSCLK = 336/4 = 84 MHz
 *            - PLLQ = 7:   USB/SDIO = 336/7 = 48 MHz
 *          
 *          Bus Clocks:
 *            - AHB  = SYSCLK = 84 MHz (HPRE = 1)
 *            - APB1 = AHB/2 = 42 MHz (PPRE1 = 4)
 *            - APB2 = AHB = 84 MHz (PPRE2 = 0)
 * 
 * @note    The flash latency must be set before increasing the clock.
 *          For 84 MHz, we need 2 wait states (see STM32F401 datasheet).
 */
static void BSP_SystemClock_Config(void) {
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) {
    }
    
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) {
    }
    
    FLASH->ACR = FLASH_ACR_LATENCY_2WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN;
    
    RCC->PLLCFGR = (8U << RCC_PLLCFGR_PLLM_Pos) |
                   (336U << RCC_PLLCFGR_PLLN_Pos) |
                   (0U << RCC_PLLCFGR_PLLP_Pos) |
                   (RCC_PLLCFGR_PLLSRC_HSE) |
                   (7U << RCC_PLLCFGR_PLLQ_Pos);
    
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {
    }
    
    RCC->CFGR = (0U << RCC_CFGR_HPRE_Pos) |
                (5U << RCC_CFGR_PPRE1_Pos) |
                (0U << RCC_CFGR_PPRE2_Pos);
    
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL) {
    }
    
    RCC->CR &= ~RCC_CR_HSION;
}

/**
 * @brief   Initialize SysTick timer for millisecond timing
 * @details Configures SysTick to generate an interrupt every 1 ms.
 *          - Reload value = SystemCoreClock / 1000 = 84000
 *          - Uses processor clock (not external reference)
 *          - Enables SysTick interrupt and counter
 */
static void BSP_SysTick_Init(void) {
    SysTick->LOAD = (BSP_SYS_CLOCK_HZ / 1000UL) - 1UL;
    SysTick->VAL = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
}

/**
 * @brief   Initialize GPIO pins for LED, Button, UART, and I2C
 * @details This function configures:
 *          
 *          LED (PA5):
 *            - Mode: General purpose output
 *            - Output type: Push-pull
 *            - Speed: High speed
 *            - Pull-up/down: None
 *          
 *          Button (PC13):
 *            - Mode: Input
 *            - Pull-up/down: Pull-up (button is active-low)
 *          
 *          UART2 TX (PA2) and RX (PA3):
 *            - Mode: Alternate function
 *            - Alternate function: AF7 (USART2)
 *            - Output type: Push-pull (TX), no pull (both)
 *            - Speed: High speed
 *          
 *          I2C1 SCL (PB6) and SDA (PB7):
 *            - Mode: Alternate function
 *            - Alternate function: AF4 (I2C1)
 *            - Output type: Open-drain (I2C requires this)
 *            - Speed: High speed
 *            - Pull-up: None (external pull-ups required for I2C)
 */
static void BSP_GPIO_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN;
    
    BSP_MEMORY_BARRIER();
    
    GPIOA->MODER &= ~(0x3U << (BSP_LED_PIN * 2));
    GPIOA->MODER |= (0x1U << (BSP_LED_PIN * 2));
    GPIOA->OTYPER &= ~(0x1U << BSP_LED_PIN);
    GPIOA->OSPEEDR |= (0x2U << (BSP_LED_PIN * 2));
    GPIOA->PUPDR &= ~(0x3U << (BSP_LED_PIN * 2));
    
    GPIOC->MODER &= ~(0x3U << (BSP_BUTTON_PIN * 2));
    GPIOC->PUPDR &= ~(0x3U << (BSP_BUTTON_PIN * 2));
    GPIOC->PUPDR |= (0x1U << (BSP_BUTTON_PIN * 2));
    
    GPIOA->MODER &= ~(0x3U << (BSP_UART_TX_PIN * 2));
    GPIOA->MODER |= (0x2U << (BSP_UART_TX_PIN * 2));
    GPIOA->OTYPER &= ~(0x1U << BSP_UART_TX_PIN);
    GPIOA->OSPEEDR |= (0x2U << (BSP_UART_TX_PIN * 2));
    GPIOA->PUPDR &= ~(0x3U << (BSP_UART_TX_PIN * 2));
    GPIOA->AFRL &= ~(0xFU << (BSP_UART_TX_PIN * 4));
    GPIOA->AFRL |= ((uint32_t)BSP_UART_TX_AF << (BSP_UART_TX_PIN * 4));
    
    GPIOA->MODER &= ~(0x3U << (BSP_UART_RX_PIN * 2));
    GPIOA->MODER |= (0x2U << (BSP_UART_RX_PIN * 2));
    GPIOA->OTYPER &= ~(0x1U << BSP_UART_RX_PIN);
    GPIOA->OSPEEDR |= (0x2U << (BSP_UART_RX_PIN * 2));
    GPIOA->PUPDR &= ~(0x3U << (BSP_UART_RX_PIN * 2));
    GPIOA->AFRL &= ~(0xFU << (BSP_UART_RX_PIN * 4));
    GPIOA->AFRL |= ((uint32_t)BSP_UART_RX_AF << (BSP_UART_RX_PIN * 4));
    
    GPIOB->MODER &= ~(0x3U << (BSP_I2C_SCL_PIN * 2));
    GPIOB->MODER |= (0x2U << (BSP_I2C_SCL_PIN * 2));
    GPIOB->OTYPER |= (0x1U << BSP_I2C_SCL_PIN);
    GPIOB->OSPEEDR |= (0x2U << (BSP_I2C_SCL_PIN * 2));
    GPIOB->PUPDR &= ~(0x3U << (BSP_I2C_SCL_PIN * 2));
    GPIOB->AFRL &= ~(0xFU << (BSP_I2C_SCL_PIN * 4));
    GPIOB->AFRL |= ((uint32_t)BSP_I2C_SCL_AF << (BSP_I2C_SCL_PIN * 4));
    
    GPIOB->MODER &= ~(0x3U << (BSP_I2C_SDA_PIN * 2));
    GPIOB->MODER |= (0x2U << (BSP_I2C_SDA_PIN * 2));
    GPIOB->OTYPER |= (0x1U << BSP_I2C_SDA_PIN);
    GPIOB->OSPEEDR |= (0x2U << (BSP_I2C_SDA_PIN * 2));
    GPIOB->PUPDR &= ~(0x3U << (BSP_I2C_SDA_PIN * 2));
    GPIOB->AFRL &= ~(0xFU << (BSP_I2C_SDA_PIN * 4));
    GPIOB->AFRL |= ((uint32_t)BSP_I2C_SDA_AF << (BSP_I2C_SDA_PIN * 4));
}
