/**
 * @file uart_isr_example.c
 * @brief Complete UART ISR example using lock-free ring buffer
 * 
 * This example demonstrates proper ISR design:
 * 1. Minimal work in ISR
 * 2. Lock-free data transfer to main loop
 * 3. Volatile correctness
 */

#include "isr_safe.h"
#include <stdint.h>

/*============================================================================*/
/* PRIVATE VARIABLES                                                           */
/*============================================================================*/

static RingBuffer_SPSC_t s_uart_rx_buffer;
static uint8_t s_rx_storage[256];

volatile bool g_uart_rx_packet_ready = false;

/*============================================================================*/
/* PUBLIC FUNCTIONS                                                            */
/*============================================================================*/

void UART_ISR_Init(void)
{
    RingBuffer_SPSC_Init(&s_uart_rx_buffer, s_rx_storage, 256);
}

/**
 * @brief UART Receive ISR
 * 
 * CRITICAL: This ISR follows all company rules:
 * - Under 20 instructions
 * - Zero blocking calls
 * - Zero complex math
 * - Uses lock-free buffer
 */
void USART1_IRQHandler(void)
{
    /* Check if RX buffer has data */
    if (USART1->SR & USART_SR_RXNE) {
        /* Read byte (clears flag automatically) */
        uint8_t byte = (uint8_t)USART1->DR;
        
        /* Put into lock-free ring buffer */
        (void)RingBuffer_Put_Isr(&s_uart_rx_buffer, byte);
        
        /* Signal packet ready on newline */
        if (byte == '\n') {
            g_uart_rx_packet_ready = true;
        }
    }
}

/**
 * @brief Get received byte from main loop context
 */
bool UART_GetByte(uint8_t* byte)
{
    return RingBuffer_Get_Main(&s_uart_rx_buffer, byte);
}

/**
 * @brief Check if packet is ready
 */
bool UART_IsPacketReady(void)
{
    return g_uart_rx_packet_ready;
}

/**
 * @brief Clear packet ready flag
 */
void UART_ClearPacketReady(void)
{
    g_uart_rx_packet_ready = false;
}
