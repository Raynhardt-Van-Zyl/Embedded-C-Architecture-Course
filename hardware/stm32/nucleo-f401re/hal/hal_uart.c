/**
 * @file hal_uart.c
 * @brief UART Hardware Abstraction Layer Implementation for STM32F401RE
 * 
 * This file implements polling-based UART communication for USART2,
 * which is connected to the ST-LINK Virtual COM Port on Nucleo boards.
 * 
 * Key Implementation Details:
 *   - Uses polling mode (no interrupts)
 *   - 8N1 format (8 data bits, no parity, 1 stop bit)
 *   - 16x oversampling for reliable reception
 *   - Baud rate calculated from APB1 clock (42 MHz)
 * 
 * @author Embedded Systems Course
 * @date 2024
 */

#include "hal_uart.h"

/*============================================================================*/
/*                          GPIO CONFIGURATION FOR USART2                     */
/*============================================================================*/

/**
 * GPIO Mode and Alternate Function Values
 * Used for configuring PA2 (TX) and PA3 (RX)
 */
#define GPIO_MODE_ALTERNATE     0x02    /* Alternate function mode */
#define GPIO_SPEED_HIGH         0x02    /* High speed */
#define GPIO_PULL_UP            0x01    /* Pull-up (good for idle-high UART) */
#define GPIO_AF7_USART2         0x07    /* AF7 maps to USART1/2/3 */

/*============================================================================*/
/*                          PUBLIC FUNCTION IMPLEMENTATIONS                    */
/*============================================================================*/

/**
 * @brief Initialize USART2 for serial communication
 * 
 * Complete initialization sequence with detailed register explanations:
 * 
 * INITIALIZATION ORDER (from Reference Manual):
 *   1. Enable peripheral clocks (GPIO and USART)
 *   2. Configure GPIO pins for alternate function
 *   3. Program BRR for desired baud rate
 *   4. Program CR1/CR2 for desired format
 *   5. Enable USART (UE bit)
 *   6. Enable TE and RE (order matters after UE)
 * 
 * @param baud_rate Desired baud rate (e.g., 9600, 115200)
 */
void HAL_UART_Init(uint32_t baud_rate)
{
    uint32_t temp;
    uint32_t brr_value;
    uint32_t mantissa;
    uint32_t fraction;
    
    /*
     * ==================== STEP 1: Enable Clocks ====================
     * 
     * GPIOA Clock (AHB1ENR):
     *   USART2 uses PA2 (TX) and PA3 (RX) on the Nucleo board.
     *   GPIOA must be clocked before configuring these pins.
     * 
     * USART2 Clock (APB1ENR):
     *   USART2 is on the APB1 bus (slower peripheral bus).
     *   APB1 clock = 42 MHz when SYSCLK = 84 MHz.
     */
    
    /* Enable GPIOA clock (bit 0 in AHB1ENR) */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    
    /* Enable USART2 clock (bit 17 in APB1ENR) */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    
    /*
     * ==================== STEP 2: Configure GPIO Pins ====================
     * 
     * PA2: USART2_TX (Alternate Function 7)
     * PA3: USART2_RX (Alternate Function 7)
     * 
     * GPIO Configuration for UART:
     *   - Mode: Alternate Function (10 binary)
     *   - Speed: High (good for 115200+ baud)
     *   - Pull: Pull-up (UART idle state is high)
     *   - AF: AF7 for USART2
     */
    
    /* Configure PA2 (TX) - Pin 2 */
    /* MODER[5:4] = 10 (alternate function) */
    temp = GPIOA->MODER;
    temp &= ~(0x03U << 4);              /* Clear bits 5:4 */
    temp |= (GPIO_MODE_ALTERNATE << 4); /* Set AF mode */
    GPIOA->MODER = temp;
    
    /* OSPEEDR[5:4] = 10 (high speed) */
    temp = GPIOA->OSPEEDR;
    temp &= ~(0x03U << 4);
    temp |= (GPIO_SPEED_HIGH << 4);
    GPIOA->OSPEEDR = temp;
    
    /* PUPDR[5:4] = 01 (pull-up) */
    temp = GPIOA->PUPDR;
    temp &= ~(0x03U << 4);
    temp |= (GPIO_PULL_UP << 4);
    GPIOA->PUPDR = temp;
    
    /* AFRL[11:8] = 0111 (AF7) - Pin 2 uses bits [11:8] */
    temp = GPIOA->AFRL;
    temp &= ~(0x0FU << 8);              /* Clear AF bits for pin 2 */
    temp |= (GPIO_AF7_USART2 << 8);     /* Set AF7 */
    GPIOA->AFRL = temp;
    
    /* Configure PA3 (RX) - Pin 3 */
    /* MODER[7:6] = 10 (alternate function) */
    temp = GPIOA->MODER;
    temp &= ~(0x03U << 6);              /* Clear bits 7:6 */
    temp |= (GPIO_MODE_ALTERNATE << 6); /* Set AF mode */
    GPIOA->MODER = temp;
    
    /* OSPEEDR[7:6] = 10 (high speed) */
    temp = GPIOA->OSPEEDR;
    temp &= ~(0x03U << 6);
    temp |= (GPIO_SPEED_HIGH << 6);
    GPIOA->OSPEEDR = temp;
    
    /* PUPDR[7:6] = 01 (pull-up) */
    temp = GPIOA->PUPDR;
    temp &= ~(0x03U << 6);
    temp |= (GPIO_PULL_UP << 6);
    GPIOA->PUPDR = temp;
    
    /* AFRL[15:12] = 0111 (AF7) - Pin 3 uses bits [15:12] */
    temp = GPIOA->AFRL;
    temp &= ~(0x0FU << 12);             /* Clear AF bits for pin 3 */
    temp |= (GPIO_AF7_USART2 << 12);    /* Set AF7 */
    GPIOA->AFRL = temp;
    
    /*
     * ==================== STEP 3: Calculate Baud Rate ====================
     * 
     * BRR (Baud Rate Register) Calculation:
     * 
     * With OVER8 = 0 (16x oversampling, default):
     *   BRR = fPCLK / (16 × baud)
     *   Where fPCLK = 42 MHz (APB1 clock)
     * 
     * BRR Register Layout:
     *   Bits [15:4] - DIV_Mantissa (integer part)
     *   Bits [3:0]  - DIV_Fraction (fractional part, /16)
     * 
     * Example: 115200 baud
     *   DIV = 42000000 / (16 × 115200) = 22.786
     *   Mantissa = 22 (integer part)
     *   Fraction = 0.786 × 16 = 12.58 ≈ 13
     *   BRR = (22 << 4) | 13 = 0x16D
     *   
     *   Actual baud = 42000000 / (16 × 22.8125) = 115,129 bps
     *   Error = (115129 - 115200) / 115200 = -0.06% (excellent)
     * 
     * Example: 9600 baud
     *   DIV = 42000000 / (16 × 9600) = 273.4375
     *   Mantissa = 273
     *   Fraction = 0.4375 × 16 = 7
     *   BRR = (273 << 4) | 7 = 0x1107
     */
    
    /* Calculate baud rate divisor */
    /*
     * UART_DIV = PCLK / (16 × baud)
     * 
     * Using 64-bit arithmetic to avoid overflow:
     *   42000000 × 16 = 672,000,000 (fits in 32-bit)
     *   But for safety with other clock frequencies, use 64-bit
     */
    uint64_t div = (uint64_t)UART_PCLK_FREQUENCY_HZ * 100U;
    div = div / (16U * baud_rate);
    
    /* Extract mantissa and fraction */
    mantissa = (uint32_t)(div / 100U);
    fraction = (uint32_t)((div % 100U) * 16U / 100U);
    
    /* Combine into BRR value */
    brr_value = (mantissa << 4) | (fraction & 0x0FU);
    
    /* Write BRR register */
    USART2->BRR = brr_value;
    
    /*
     * ==================== STEP 4: Configure Control Registers ====================
     */
    
    /* CR2: Configure stop bits (default is 1 stop bit, bits [13:12] = 00) */
    USART2->CR2 = USART_CR2_STOP_1;  /* 1 stop bit */
    
    /*
     * CR1 Configuration:
     *   - M = 0: 8 data bits (default)
     *   - PCE = 0: No parity (default)
     *   - OVER8 = 0: 16x oversampling (default, better noise immunity)
     *   - TE = 1: Transmitter enable
     *   - RE = 1: Receiver enable
     *   - UE = 1: USART enable
     * 
     * Note: UE should be set before TE/RE according to reference manual
     */
    USART2->CR1 = 0;  /* Clear all bits first */
    
    /* Enable USART first, then TE and RE */
    USART2->CR1 |= USART_CR1_UE;   /* USART enable */
    
    /* Small delay to ensure USART is ready (not strictly necessary but safe) */
    for (volatile int i = 0; i < 100; i++);
    
    USART2->CR1 |= USART_CR1_TE;   /* Transmitter enable */
    USART2->CR1 |= USART_CR1_RE;   /* Receiver enable */
    
    /*
     * ==================== STEP 5: Clear Any Pending Flags ====================
     * 
     * Read DR to clear RXNE if set (from any garbage data during init)
     * This prevents false data available on first read
     */
    temp = USART2->DR;
    (void)temp;  /* Suppress unused variable warning */
}

/**
 * @brief Transmit a single byte via UART
 * 
 * Polling transmit sequence:
 *   1. Wait for TXE flag (TX data register empty)
 *   2. Write data to DR
 *   3. TXE is automatically cleared by the write
 * 
 * TXE Flag:
 *   - Set when TDR (transmit data register) is empty
 *   - Cleared when DR is written
 *   - If TC (transmission complete) is also set, the shift register is empty
 * 
 * @param byte Data byte to transmit
 */
void HAL_UART_PutChar(uint8_t byte)
{
    /*
     * Wait for TXE flag to be set.
     * TXE = 1 means the transmit data register is empty and can accept new data.
     * 
     * This loop will block until the flag is set.
     * At 115200 baud, max wait is ~87 µs (time to shift out one character).
     */
    while ((USART2->SR & USART_SR_TXE) == 0U)
    {
        /* Busy wait - could add timeout for production code */
    }
    
    /*
     * Write data to DR register.
     * This automatically clears the TXE flag.
     * The data is transferred to the shift register and transmission begins.
     */
    USART2->DR = (uint32_t)byte;
}

/**
 * @brief Transmit a null-terminated string via UART
 * 
 * Iterates through string calling PutChar for each character.
 * 
 * @param str Pointer to null-terminated string
 */
void HAL_UART_Puts(const char *str)
{
    /* Transmit each character until null terminator */
    while (*str != '\0')
    {
        HAL_UART_PutChar((uint8_t)*str);
        str++;
    }
}

/**
 * @brief Receive a single byte via UART
 * 
 * Polling receive sequence:
 *   1. Wait for RXNE flag (RX data register not empty)
 *   2. Read data from DR
 *   3. RXNE is automatically cleared by the read
 * 
 * RXNE Flag:
 *   - Set when RDR (receive data register) contains received data
 *   - Cleared when DR is read
 * 
 * Warning: This function blocks indefinitely if no data is received!
 * 
 * @return Received data byte
 */
uint8_t HAL_UART_GetChar(void)
{
    /*
     * Wait for RXNE flag to be set.
     * RXNE = 1 means data has been received and is available in DR.
     * 
     * WARNING: This loop will block forever if no data arrives!
     * For non-blocking operation, check HAL_UART_Available() first.
     */
    while ((USART2->SR & USART_SR_RXNE) == 0U)
    {
        /* Busy wait - add timeout for production code */
    }
    
    /*
     * Read data from DR register.
     * This automatically clears the RXNE flag.
     * 
     * Cast to uint8_t because DR is 32-bit but only lower 8 bits contain data.
     */
    return (uint8_t)(USART2->DR & 0xFFU);
}

/**
 * @brief Check if data is available to read
 * 
 * Non-blocking check for received data.
 * Use this before calling GetChar to avoid blocking.
 * 
 * @return true if at least one byte is available, false otherwise
 */
bool HAL_UART_Available(void)
{
    /*
     * Check RXNE flag without blocking.
     * Returns true if data is waiting in the receive buffer.
     */
    return ((USART2->SR & USART_SR_RXNE) != 0U);
}

/**
 * @brief Transmit data buffer via UART
 * 
 * Transmits multiple bytes from buffer.
 * 
 * @param data   Pointer to data buffer
 * @param length Number of bytes to transmit
 */
void HAL_UART_Write(const uint8_t *data, size_t length)
{
    /* Transmit each byte in the buffer */
    for (size_t i = 0; i < length; i++)
    {
        HAL_UART_PutChar(data[i]);
    }
}

/**
 * @brief Print formatted integer value
 * 
 * Converts integer to ASCII and prints.
 * Handles negative numbers.
 * 
 * @param value Integer value to print
 */
void HAL_UART_PrintInt(int32_t value)
{
    char buffer[12];  /* Enough for -2147483648 + null */
    char *ptr = buffer + 11;
    int32_t abs_value;
    bool negative = false;
    
    /* Null terminate */
    *ptr = '\0';
    
    /* Handle negative numbers */
    if (value < 0)
    {
        negative = true;
        abs_value = -value;
    }
    else
    {
        abs_value = value;
    }
    
    /* Handle zero case */
    if (abs_value == 0)
    {
        HAL_UART_PutChar('0');
        return;
    }
    
    /* Convert to ASCII (reverse order) */
    while (abs_value > 0)
    {
        ptr--;
        *ptr = '0' + (abs_value % 10);
        abs_value /= 10;
    }
    
    /* Add minus sign if negative */
    if (negative)
    {
        ptr--;
        *ptr = '-';
    }
    
    /* Print the string */
    HAL_UART_Puts(ptr);
}

/**
 * @brief Print hexadecimal value
 * 
 * Prints a value in hexadecimal format with "0x" prefix.
 * 
 * @param value  Value to print
 * @param digits Number of hex digits (1-8)
 */
void HAL_UART_PrintHex(uint32_t value, uint8_t digits)
{
    static const char hex_chars[] = "0123456789ABCDEF";
    
    /* Print prefix */
    HAL_UART_Puts("0x");
    
    /* Clamp digits to valid range */
    if (digits > 8)
    {
        digits = 8;
    }
    
    /* Print hex digits from MSB to LSB */
    for (int i = digits - 1; i >= 0; i--)
    {
        uint8_t nibble = (value >> (i * 4)) & 0x0FU;
        HAL_UART_PutChar((uint8_t)hex_chars[nibble]);
    }
}
