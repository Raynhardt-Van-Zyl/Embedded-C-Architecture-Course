/**
 * @file hal_uart.h
 * @brief UART Hardware Abstraction Layer for STM32F401RE
 * 
 * This module provides polling-based UART communication using USART2,
 * which is connected to the ST-LINK Virtual COM Port (VCP) on the Nucleo board.
 * 
 * USART2 Pin Assignment (Nucleo-F401RE):
 *   PA2 - USART2_TX (connected to ST-LINK VCP RX)
 *   PA3 - USART2_RX (connected to ST-LINK VCP TX)
 * 
 * Clock Configuration:
 *   - USART2 is on APB1 bus
 *   - APB1 clock (PCLK1) = 42 MHz (when SYSCLK = 84 MHz)
 *   - Baud rate divisor calculated from PCLK1
 * 
 * Features:
 *   - 8 data bits, no parity, 1 stop bit (8N1)
 *   - Polling transmit and receive
 *   - No interrupts or DMA (simple polling mode)
 * 
 * @author Embedded Systems Course
 * @date 2024
 */

#ifndef HAL_UART_H
#define HAL_UART_H

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
 * USART Base Addresses
 * From STM32F401 Reference Manual (RM0368)
 */
#define USART1_BASE         (0x40011000UL)
#define USART2_BASE         (0x40004400UL)
#define USART6_BASE         (0x40011400UL)

/**
 * RCC Base Address
 */
#define RCC_BASE            (0x40023800UL)

/**
 * GPIO Base Addresses (for UART pin configuration)
 */
#define GPIOA_BASE          (0x40020000UL)

/*============================================================================*/
/*                          REGISTER DEFINITIONS                               */
/*============================================================================*/

/**
 * USART Register Structure
 * 
 * Register offsets from Reference Manual:
 *   SR:    0x00 - Status register (read-only flags)
 *   DR:    0x04 - Data register (read/write)
 *   BRR:   0x08 - Baud rate register
 *   CR1:   0x0C - Control register 1
 *   CR2:   0x10 - Control register 2
 *   CR3:   0x14 - Control register 3
 *   GTPR:  0x18 - Guard time and prescaler
 */
typedef struct {
    volatile uint32_t SR;       /* Offset 0x00: Status register */
    volatile uint32_t DR;       /* Offset 0x04: Data register */
    volatile uint32_t BRR;      /* Offset 0x08: Baud rate register */
    volatile uint32_t CR1;      /* Offset 0x0C: Control register 1 */
    volatile uint32_t CR2;      /* Offset 0x10: Control register 2 */
    volatile uint32_t CR3;      /* Offset 0x14: Control register 3 */
    volatile uint32_t GTPR;     /* Offset 0x18: Guard time and prescaler */
} USART_Regs_t;

/**
 * RCC Register Structure (partial - relevant registers only)
 */
typedef struct {
    volatile uint32_t CR;       /* Offset 0x00: Clock control */
    volatile uint32_t PLLCFGR;  /* Offset 0x04: PLL configuration */
    volatile uint32_t CFGR;     /* Offset 0x08: Clock configuration */
    volatile uint32_t CIR;      /* Offset 0x0C: Clock interrupt */
    uint8_t          reserved[0x24];
    volatile uint32_t AHB1ENR;  /* Offset 0x30: AHB1 clock enable */
    uint8_t          reserved2[0x08];
    volatile uint32_t APB1ENR;  /* Offset 0x40: APB1 clock enable */
    volatile uint32_t APB2ENR;  /* Offset 0x44: APB2 clock enable */
} RCC_Regs_t;

/**
 * GPIO Register Structure (minimal for UART pin setup)
 */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFRL;
    volatile uint32_t AFRH;
} GPIO_Regs_t;

/* Peripheral pointers */
#define USART1              ((USART_Regs_t *)USART1_BASE)
#define USART2              ((USART_Regs_t *)USART2_BASE)
#define USART6              ((USART_Regs_t *)USART6_BASE)
#define RCC                 ((RCC_Regs_t *)RCC_BASE)
#define GPIOA               ((GPIO_Regs_t *)GPIOA_BASE)

/*============================================================================*/
/*                          USART SR REGISTER BITS                             */
/*============================================================================*/

/**
 * Status Register (SR) Bit Definitions
 * 
 * These flags indicate USART status and events.
 * Most flags are cleared by specific sequences (see each flag).
 */
#define USART_SR_PE          (1U << 0)    /* Parity error flag */
#define USART_SR_FE          (1U << 1)    /* Framing error (stop bit not detected) */
#define USART_SR_NE          (1U << 2)    /* Noise error flag */
#define USART_SR_ORE         (1U << 3)    /* Overrun error (data lost) */
#define USART_SR_IDLE        (1U << 4)    /* Idle line detected */
#define USART_SR_RXNE        (1U << 5)    /* RX data register not empty (data available) */
#define USART_SR_TC          (1U << 6)    /* Transmission complete */
#define USART_SR_TXE         (1U << 7)    /* TX data register empty (ready for data) */
#define USART_SR_LBD         (1U << 8)    /* LIN break detection */
#define USART_SR_CTS         (1U << 9)    /* CTS flag */

/*
 * Key Status Flags Explained:
 * 
 * TXE (Transmit Data Register Empty):
 *   - Set when DR can accept new data
 *   - Cleared when DR is written
 *   - Use: Wait for TXE before writing to DR
 * 
 * TC (Transmission Complete):
 *   - Set when transmission of last frame completes
 *   - Use: Wait for TC before disabling USART or entering low power
 *   - Clear: Read SR then write DR, or write 0 to TC
 * 
 * RXNE (Receive Data Register Not Empty):
 *   - Set when DR contains received data
 *   - Cleared when DR is read
 *   - Use: Wait for RXNE before reading from DR
 */

/*============================================================================*/
/*                          USART CR1 REGISTER BITS                            */
/*============================================================================*/

/**
 * Control Register 1 (CR1) Bit Definitions
 */
#define USART_CR1_SBK        (1U << 0)    /* Send break */
#define USART_CR1_RWU        (1U << 1)    /* Receiver wakeup from mute mode */
#define USART_CR1_RE         (1U << 2)    /* Receiver enable */
#define USART_CR1_TE         (1U << 3)    /* Transmitter enable */
#define USART_CR1_IDLEIE     (1U << 4)    /* Idle line interrupt enable */
#define USART_CR1_RXNEIE     (1U << 5)    /* RXNE interrupt enable */
#define USART_CR1_TCIE       (1U << 6)    /* TC interrupt enable */
#define USART_CR1_TXEIE      (1U << 7)    /* TXE interrupt enable */
#define USART_CR1_PEIE       (1U << 8)    /* Parity error interrupt enable */
#define USART_CR1_PS         (1U << 9)    /* Parity selection (0=even, 1=odd) */
#define USART_CR1_PCE        (1U << 10)   /* Parity control enable */
#define USART_CR1_WAKE       (1U << 11)   /* Wakeup method */
#define USART_CR1_M          (1U << 12)   /* Word length (0=8bit, 1=9bit) */
#define USART_CR1_UE         (1U << 13)   /* USART enable */
#define USART_CR1_OVER8      (1U << 15)   /* Oversampling mode (0=16x, 1=8x) */

/*============================================================================*/
/*                          USART CR2 REGISTER BITS                            */
/*============================================================================*/

/**
 * Control Register 2 (CR2) Bit Definitions
 */
#define USART_CR2_LBDL       (1U << 5)    /* LIN break detection length */
#define USART_CR2_LBDIE      (1U << 6)    /* LIN break detection interrupt */
#define USART_CR2_LBCL       (1U << 8)    /* Last bit clock pulse */
#define USART_CR2_CPHA       (1U << 9)    /* Clock phase */
#define USART_CR2_CPOL       (1U << 10)   /* Clock polarity */
#define USART_CR2_CLKEN      (1U << 11)   /* Clock enable */
#define USART_CR2_STOP_1     (0U << 12)   /* 1 stop bit */
#define USART_CR2_STOP_0_5   (1U << 12)   /* 0.5 stop bits */
#define USART_CR2_STOP_2     (2U << 12)   /* 2 stop bits */
#define USART_CR2_STOP_1_5   (3U << 12)   /* 1.5 stop bits */
#define USART_CR2_LINEN      (1U << 14)   /* LIN mode enable */

/*============================================================================*/
/*                          RCC CLOCK ENABLE BITS                              */
/*============================================================================*/

/**
 * RCC APB1ENR USART Clock Enable Bits
 * USART2 is on APB1 bus (slower peripheral bus)
 */
#define RCC_APB1ENR_USART2EN    (1U << 17)

/**
 * RCC APB2ENR USART Clock Enable Bits
 * USART1 and USART6 are on APB2 bus (faster peripheral bus)
 */
#define RCC_APB2ENR_USART1EN    (1U << 4)
#define RCC_APB2ENR_USART6EN    (1U << 5)

/**
 * RCC AHB1ENR GPIO Clock Enable
 */
#define RCC_AHB1ENR_GPIOAEN     (1U << 0)

/*============================================================================*/
/*                          CLOCK CONFIGURATION                                */
/*============================================================================*/

/**
 * Clock frequencies for baud rate calculation
 * 
 * These values assume:
 *   - HSE = 8 MHz (Nucleo board external crystal)
 *   - PLL configuration: HSE/PLLM * PLLN / PLLP = 84 MHz SYSCLK
 *   - AHB prescaler = 1, so HCLK = 84 MHz
 *   - APB1 prescaler = 2, so PCLK1 = 42 MHz
 *   - APB2 prescaler = 1, so PCLK2 = 84 MHz
 * 
 * USART2 is on APB1, so its clock source is PCLK1 = 42 MHz
 */
#define UART_PCLK_FREQUENCY     42000000UL   /* APB1 clock: 42 MHz */
#define UART_PCLK_FREQUENCY_HZ  42000000UL   /* Alias for clarity */

/*============================================================================*/
/*                          PUBLIC FUNCTION PROTOTYPES                         */
/*============================================================================*/

/**
 * @brief Initialize USART2 for serial communication
 * 
 * Performs complete USART2 initialization:
 *   1. Enable GPIOA clock (for PA2/PA3)
 *   2. Enable USART2 clock (APB1)
 *   3. Configure PA2 as AF7 (TX), PA3 as AF7 (RX)
 *   4. Calculate and set baud rate divisor
 *   5. Configure: 8 data bits, no parity, 1 stop bit
 *   6. Enable transmitter and receiver
 *   7. Enable USART
 * 
 * Baud Rate Calculation:
 *   BRR = fPCLK / (16 * baud)
 *   For 115200 baud: BRR = 42000000 / (16 * 115200) = 22.79 ≈ 22
 *   
 *   With OVER8=0 (16x oversampling):
 *   - Mantissa = integer part of division
 *   - Fraction = (remainder / 16) * 16
 * 
 * @param baud_rate Desired baud rate (e.g., 9600, 115200)
 */
void HAL_UART_Init(uint32_t baud_rate);

/**
 * @brief Transmit a single byte via UART
 * 
 * Waits for TXE flag (transmit data register empty), then writes byte.
 * This is a blocking function - it waits until the transmit buffer
 * is ready to accept new data.
 * 
 * Timing:
 *   - At 115200 baud: ~87 µs per character (10 bits: 1 start + 8 data + 1 stop)
 *   - TXE typically set quickly if no ongoing transmission
 * 
 * @param byte Data byte to transmit
 */
void HAL_UART_PutChar(uint8_t byte);

/**
 * @brief Transmit a null-terminated string via UART
 * 
 * Iterates through string, calling PutChar for each character.
 * Does NOT append newline - caller must include \r\n if desired.
 * 
 * @param str Pointer to null-terminated string
 */
void HAL_UART_Puts(const char *str);

/**
 * @brief Receive a single byte via UART
 * 
 * Waits for RXNE flag (receive data register not empty), then reads byte.
 * This is a blocking function - it waits until data is received.
 * 
 * Warning: This function will block forever if no data is received.
 * For non-blocking operation, use HAL_UART_Available() first.
 * 
 * @return Received data byte
 */
uint8_t HAL_UART_GetChar(void);

/**
 * @brief Check if data is available to read
 * 
 * Non-blocking check for received data.
 * 
 * @return true if at least one byte is available, false otherwise
 */
bool HAL_UART_Available(void);

/**
 * @brief Transmit data buffer via UART
 * 
 * Transmits multiple bytes from buffer.
 * 
 * @param data   Pointer to data buffer
 * @param length Number of bytes to transmit
 */
void HAL_UART_Write(const uint8_t *data, size_t length);

/**
 * @brief Print formatted integer value
 * 
 * Prints an integer as decimal string.
 * 
 * @param value Integer value to print
 */
void HAL_UART_PrintInt(int32_t value);

/**
 * @brief Print hexadecimal value
 * 
 * Prints a value in hexadecimal format with "0x" prefix.
 * 
 * @param value  Value to print
 * @param digits Number of hex digits (1-8)
 */
void HAL_UART_PrintHex(uint32_t value, uint8_t digits);

#ifdef __cplusplus
}
#endif

#endif /* HAL_UART_H */
