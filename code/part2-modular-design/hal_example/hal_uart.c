/**
 * @file Haluart.c
 * @brief Hardware Abstraction Layer for UART - Implementation
 * 
 * This file implements the UART peripheral access for the STM32F401RE
 * following the course naming conventions and architectural patterns.
 * 
 * @author Embedded C Architecture Course
 */

#include "Haluart.h"
#include "../../../hardware/stm32/nucleo-f401re/hal/Haluart.h" // Hardware register definitions
#include <stddef.h>
#include <string.h>

/*============================================================================*/
/*                          PRIVATE DEFINITIONS                                */
/*============================================================================*/

#define MAX_UART_INSTANCES 3

struct HalUart_s {
    bool is_allocated;
    HalUartConfig_t config;
    USART_Regs_t* regs;
    uint32_t irq_number;
};

static struct HalUart_s g_uart_pool[MAX_UART_INSTANCES] = {0};

/*============================================================================*/
/*                          PRIVATE HELPER FUNCTIONS                           */
/*============================================================================*/

static USART_Regs_t* get_uart_regs(uint8_t uart_id) {
    switch (uart_id) {
        case 1:  return USART1;
        case 2:  return USART2;
        case 6:  return USART6;
        default: return NULL;
    }
}

/*============================================================================*/
/*                          PUBLIC API IMPLEMENTATION                          */
/*============================================================================*/

HalUartHandle HalUart_Init(uint8_t uart_id, const HalUartConfig_t* config) {
    if (!config) return NULL;

    USART_Regs_t* regs = get_uart_regs(uart_id);
    if (!regs) return NULL;

    /* Allocate from pool */
    struct HalUart_s* inst = NULL;
    for (int i = 0; i < MAX_UART_INSTANCES; i++) {
        if (!g_uart_pool[i].is_allocated) {
            inst = &g_uart_pool[i];
            inst->is_allocated = true;
            break;
        }
    }

    if (!inst) return NULL;

    inst->config = *config;
    inst->regs = regs;

    /* 1. Enable Clock and GPIO AF (In a real system, this would call HalGPIO_Init) */
    /* Hardware-specific logic from hardware/hal/Haluart.c */
    if (uart_id == 2) {
        // Enable USART2 clock
        //RCC->APB1ENR |= (1U << 17);
        // This is handled by hardware-level HalUART_Init if available
    }

    /* 2. Configure Baud Rate, Parity, Stop bits using hardware registers */
    // Placeholder for actual register logic or calling hardware HAL
    // For this course, we demonstrate the mapping:
    // regs->BRR = ...
    // regs->CR1 = ...

    return (HalUartHandle)inst;
}

void HalUart_DeInit(HalUartHandle handle) {
    if (handle) {
        handle->is_allocated = false;
    }
}

int32_t HalUart_Write(HalUartHandle handle, const uint8_t* data, uint32_t length, uint32_t timeout_ms) {
    if (!handle || !data) return -1;
    
    USART_Regs_t* regs = handle->regs;
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < length; i++) {
        uint32_t timeout = timeout_ms * 1000; // Rough conversion
        while (!(regs->SR & USART_SR_TXE)) {
            if (timeout-- == 0) return (int32_t)count;
        }
        regs->DR = data[i];
        count++;
    }
    
    return (int32_t)count;
}

int32_t HalUart_Read(HalUartHandle handle, uint8_t* buffer, uint32_t length, uint32_t timeout_ms) {
    if (!handle || !buffer) return -1;
    
    USART_Regs_t* regs = handle->regs;
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < length; i++) {
        uint32_t timeout = timeout_ms * 1000;
        while (!(regs->SR & USART_SR_RXNE)) {
            if (timeout-- == 0) return (int32_t)count;
        }
        buffer[i] = (uint8_t)(regs->DR & 0xFF);
        count++;
    }
    
    return (int32_t)count;
}

bool HalUart_IsBusy(HalUartHandle handle) {
    if (!handle) return false;
    return !(handle->regs->SR & USART_SR_TC);
}
