/**
 * @file    main.c
 * @brief   UART Printf Example for STM32F401RE Nucleo
 * @details This example demonstrates:
 *          - UART initialization for virtual COM port
 *          - Sending strings over UART
 *          - Echoing received characters
 *          - Simple counter display
 * 
 * Hardware Configuration:
 *   - UART: USART2 connected to ST-LINK virtual COM port
 *   - TX:   PA2 (USART2_TX, alternate function AF7)
 *   - RX:   PA3 (USART2_RX, alternate function AF7)
 *   - Baud: 115200, 8N1 (8 data bits, no parity, 1 stop bit)
 * 
 * UART Initialization Sequence:
 *   1. GPIO pins configured by BSP_Init():
 *      - PA2: Alternate function mode, AF7, push-pull, high speed
 *      - PA3: Alternate function mode, AF7, no pull-up/down
 *   2. BSP_UART_Init() configures USART2:
 *      - Enable USART2 clock (APB1ENR.USART2EN)
 *      - Calculate BRR for 115200 baud at 42 MHz APB1:
 *        BRR = 42000000 / (16 * 115200) = 22.789
 *        DIV_Mantissa = 22, DIV_Fraction = 13 (0.8125 * 16)
 *        BRR = 0x16D
 *      - Set CR1: TE (transmit enable), RE (receive enable), UE (UART enable)
 * 
 * Communication:
 *   - Open a terminal program (PuTTY, Tera Term, etc.)
 *   - Connect to the ST-LINK virtual COM port
 *   - Settings: 115200 baud, 8 data bits, no parity, 1 stop bit
 * 
 * Expected Behavior:
 *   - Prints "Hello World!" and incrementing counter every 1 second
 *   - Any character sent to the UART is echoed back
 *   - Press 'r' to reset counter
 * 
 * @author  Embedded C Architecture Course
 * @version 1.0
 */

#include "bsp.h"
#include <stdio.h>
#include <string.h>

/*============================================================================*/
/*                          CONFIGURATION                                      */
/*============================================================================*/

#define PRINT_INTERVAL_MS   (1000U)
#define BUFFER_SIZE         (64U)

/*============================================================================*/
/*                          PRIVATE VARIABLES                                  */
/*============================================================================*/

static volatile uint32_t counter = 0;
static char buffer[BUFFER_SIZE];

/*============================================================================*/
/*                          PRIVATE FUNCTIONS                                  */
/*============================================================================*/

/**
 * @brief   Convert integer to string
 * @param   value: Integer value to convert
 * @param   str:   Output string buffer
 * @return  Pointer to output string
 */
static char* itoa_simple(uint32_t value, char *str) {
    char *ptr = str;
    char *start;
    char temp;
    
    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return str;
    }
    
    start = ptr;
    while (value > 0) {
        *ptr++ = (value % 10) + '0';
        value /= 10;
    }
    *ptr = '\0';
    
    ptr--;
    while (start < ptr) {
        temp = *start;
        *start = *ptr;
        *ptr = temp;
        start++;
        ptr--;
    }
    
    return str;
}

/**
 * @brief   Print formatted counter value
 * @param   count: Counter value to print
 */
static void print_counter(uint32_t count) {
    BSP_UART_PutString("Counter: ");
    itoa_simple(count, buffer);
    BSP_UART_PutString(buffer);
    BSP_UART_PutString("\r\n");
}

/*============================================================================*/
/*                          MAIN FUNCTION                                      */
/*============================================================================*/

int main(void) {
    BSP_Init();
    BSP_UART_Init();
    
    BSP_UART_PutString("\r\n");
    BSP_UART_PutString("================================\r\n");
    BSP_UART_PutString("  STM32F401RE UART Example\r\n");
    BSP_UART_PutString("================================\r\n");
    BSP_UART_PutString("\r\n");
    BSP_UART_PutString("Hello World!\r\n");
    BSP_UART_PutString("Send any character to echo.\r\n");
    BSP_UART_PutString("Press 'r' to reset counter.\r\n");
    BSP_UART_PutString("\r\n");
    
    while (1) {
        print_counter(counter);
        counter++;
        
        for (uint32_t i = 0; i < PRINT_INTERVAL_MS; i++) {
            if (BSP_UART_DataAvailable()) {
                uint8_t ch = BSP_UART_GetChar();
                
                BSP_UART_PutString("You sent: '");
                BSP_UART_PutChar(ch);
                BSP_UART_PutString("'\r\n");
                
                if (ch == 'r' || ch == 'R') {
                    counter = 0;
                    BSP_UART_PutString("Counter reset!\r\n");
                }
            }
            BSP_Delay(1);
        }
    }
    
    return 0;
}

/*============================================================================*/
/*                          INTERRUPT HANDLERS                                 */
/*============================================================================*/

void SysTick_Handler(void) {
    BSP_IncrementTick();
}
