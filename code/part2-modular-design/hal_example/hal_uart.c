/**
 * @file hal_uart.c
 * @brief Hardware Abstraction Layer for UART - Implementation
 * 
 * This file implements the UART peripheral access for the STM32F401RE.
 * It * @note This is a register-level implementation demonstrating
 *        direct hardware manipulation without using ST's HAL library.
 * 
 * @author Embedded Architecture Team
 * @version 1.0.0
 */

#include "hal_uart.h"

/*============================================================================*/
/* PRIVATE DEFINES                                                              */
/*============================================================================*/

#define UART1_BASE        (0x40011000UL)
#define UART2_BASE        (0x40004400UL)

#define UART_SR_OFFSET   (0x00U)
#define UART_DR_OFFSET   (0x04U)
#define UART_BRR_OFFSET  (0x08U)
#define UART_CR1_OFFSET  (0x0CU)
#define UART_CR2_OFFSET  (0x10U)

/* Status Register bits */
#define UART_SR_RXNE        (1U << 5)
#define UART_SR_TC          (1U << 6)
#define UART_SR_TXE        (1U << 7)

/* Control Register 1 bits */
#define UART_CR1_UE          (1U << 0)

/* Control Register 2 bits */
#define UART_CR2_STOP       (1U << 0)

/*============================================================================*/
/* PRIVATE VARIABLES                                                            */
/*============================================================================*/

/* Static instance pool - no malloc */
static HalUart_t s_uart_pool[2];

/*============================================================================*/
/* PUBLIC FUNCTION IMPLEMENTATIONS                                               */
/*============================================================================*/

HalUart_t* Hal_Uart_Init(uint8_t uart_id, const HalUartConfig_t* config)
{
    if (config == NULL) return NULL;
    if (uart_id > 1) return NULL;
    
    HalUart_t* ctx = &s_uart_pool[uart_id];
    
    /* Enable UART clock */
    if (uart_id == 0) {
        RCC->APB1ENR |= (1U << 17);  /* USART1EN */
    } else {
        RCC->APB1ENR |= (1U << 18);  /* USART2EN */
    }
    
    /* Configure GPIO pins - alternate function mode */
    if (uart_id == 1) {
        /* USART2: PA2=TX, PA3=RX, AF7 */
        GPIOA->MODER &= ~(0x3U << (2 * 2));  /* Clear PA2 */
        GPIOA->MODER |= (0x2U << (2 * 2));  /* AF mode for PA2 */
        GPIOA->AFR[0] = 0x07U;       /* AF7 for PA2 */
        
        GPIOA->MODER &= ~(0x3U << (2 * 3));  /* Clear PA3 */
        GPIOA->MODER |= (0x2U << (2 * 3));  /* AF mode for PA3 */
        GPIOA->AFR[0] = 0x07U << (4 * 3); /* AF7 for PA3 */
    }
    
    /* Configure baud rate */
    uint32_t pclk = 84000000UL;  /* APB1 clock */
    uint32_t baud = config->baud_rate;
    volatile uint32_t* uart_base = (uart_id == 1) ? 
        (volatile uint32_t*)UART2_BASE : 
        (volatile uint32_t*)UART1_BASE;
    
    uint32_t brr = pclk / (16UL * baud);
    uart_base[UART_BRR_OFFSET / 4] = brr;
    
    /* Configure 8N1 format */
    uart_base[UART_CR1_OFFSET / 4] = 0;  /* 8 data bits */
    uart_base[UART_CR2_OFFSET / 4] = 0;  /* 1 stop bit */
    
    /* Enable UART */
    uart_base[UART_CR1_OFFSET / 4] |= UART_CR1_UE;
    
    ctx->is_initialized = true;
    return ctx;
}

HalUartError_t Hal_Uart_Deinit(HalUart_t* uart)
{
    if (uart == NULL || !uart->is_initialized) {
        return HAL_UART_ERROR_INVALID_PARAM;
    }
    
    uart->is_initialized = false;
    return HAL_UART_OK;
}

int32_t Hal_Uart_WriteBlocking(HalUart_t* uart, const uint8_t* data, 
                                uint32_t length, uint32_t timeout_ms)
{
    if (uart == NULL || !uart->is_initialized || data == NULL) {
        return -1;
    }
    
    volatile uint32_t* uart_base = uart->uart_id == 1 ? 
        (volatile uint32_t*)UART2_BASE : 
        (volatile uint32_t*)UART1_BASE;
    
    for (uint32_t i = 0; i < length; i++) {
        /* Wait for TXE (transmit empty) */
        uint32_t timeout = timeout_ms * 84000UL / 1000UL;
        while (!(uart_base[UART_SR_OFFSET / 4] & UART_SR_TXE)) {
            if (timeout-- == 0) return -2;  /* Timeout */
        }
        
        /* Write data */
        uart_base[UART_DR_OFFSET / 4] = data[i];
    }
    
    return (int32_t)length;
}

int32_t Hal_Uart_ReadBlocking(HalUart_t* uart, uint8_t* buffer, 
                               uint32_t length, uint32_t timeout_ms)
{
    if (uart == NULL || !uart->is_initialized || buffer == NULL) {
        return -1;
    }
    
    volatile uint32_t* uart_base = uart->uart_id == 1 ? 
        (volatile uint32_t*)UART2_BASE : 
        (volatile uint32_t*)UART1_BASE;
    
    for (uint32_t i = 0; i < length; i++) {
        /* Wait for RXNE (receive not empty) */
        uint32_t timeout = timeout_ms * 84000UL / 1000UL;
        while (!(uart_base[UART_SR_OFFSET / 4] & UART_SR_RXNE)) {
            if (timeout-- == 0) return -2;  /* Timeout */
        }
        
        /* Read data */
        buffer[i] = (uint8_t)uart_base[UART_DR_OFFSET / 4];
    }
    
    return (int32_t)length;
}

uint32_t Hal_Uart_GetRxAvailable(HalUart_t* uart)
{
    if (uart == NULL || !uart->is_initialized) {
        return 0;
    }
    
    return uart->rx_count;
}

const char* Hal_Uart_ErrorToString(HalUartError_t err)
{
    switch (err) {
        case HAL_UART_OK: return "OK";
        case HAL_UART_ERROR: return "General error";
        case HAL_UART_ERROR_INVALID_PARAM: return "Invalid parameter";
        case HAL_UART_ERROR_NULL_POINTER: return "NULL pointer";
        case HAL_UART_ERROR_NOT_INITIALIZED: return "Not initialized";
        default: return "Unknown error";
    }
}
