/**
 * @file    main.c
 * @brief   Timer Interrupt Example for STM32F401RE Nucleo
 * @details This example demonstrates:
 *          - Hardware timer (TIM2) configuration
 *          - Timer interrupt setup and handling
 *          - Non-blocking LED control using timer counter
 *          - Proper ISR design (minimal work in interrupt context)
 * 
 * Hardware Configuration:
 *   - Timer: TIM2 (32-bit general-purpose timer)
 *   - Clock: 84 MHz (APB1 × 2 multiplier)
 *   - Interrupt: TIM2 global interrupt (IRQ 28)
 *   - LED: PA5 (LD2 - Green LED on Nucleo board)
 * 
 * Timer Configuration for 1ms tick:
 *   - Prescaler (PSC): 8399 (divides 84 MHz by 8400 = 10 kHz)
 *   - Auto-reload (ARR): 9 (counts 0-9, gives 1 kHz = 1ms period)
 *   - Update interrupt: Enabled (triggers on counter overflow)
 * 
 * Timing Math:
 *   f_out = f_in / ((PSC + 1) * (ARR + 1))
 *   f_out = 84,000,000 / ((8399 + 1) * (9 + 1))
 *   f_out = 84,000,000 / 84,000 = 1000 Hz (1ms period)
 * 
 * ISR Design Principles:
 *   1. Keep ISRs short - do minimal work in interrupt context
 *   2. Set flags for main loop to process
 *   3. Clear interrupt flags to prevent re-entry
 *   4. Avoid blocking operations (delays, waiting)
 *   5. Use volatile for shared variables
 * 
 * Expected Behavior:
 *   - LED toggles every 500ms based on timer counter
 *   - Millisecond counter displayed over UART
 *   - No blocking delays in main loop
 * 
 * @author  Embedded C Architecture Course
 * @version 1.0
 */

#include "bsp.h"
#include "../hal/hal_timer.h"

/*============================================================================*/
/*                          CONFIGURATION                                      */
/*============================================================================*/

#define LED_TOGGLE_INTERVAL_MS  (500U)
#define UART_UPDATE_INTERVAL_MS (1000U)

/*============================================================================*/
/*                          PRIVATE VARIABLES                                  */
/*============================================================================*/

static volatile uint32_t ms_counter = 0;
static volatile uint32_t last_led_toggle = 0;
static volatile uint32_t last_uart_update = 0;
static volatile uint8_t uart_update_flag = 0;

/*============================================================================*/
/*                          PRIVATE FUNCTIONS                                  */
/*============================================================================*/

/**
 * @brief   Timer callback function
 * @details Called from TIM2 interrupt every 1ms.
 *          This function demonstrates proper ISR design:
 *          - Minimal work done here
 *          - Only increments counters and sets flags
 *          - Main loop handles actual LED toggle and UART output
 * 
 * @note    This is called from interrupt context, so:
 *          - Keep it short
 *          - Don't use blocking functions
 *          - Use volatile for shared variables
 */
static void timer_callback(void) {
    ms_counter++;
    
    if ((ms_counter - last_led_toggle) >= LED_TOGGLE_INTERVAL_MS) {
        last_led_toggle = ms_counter;
        BSP_LED_Toggle();
    }
    
    if ((ms_counter - last_uart_update) >= UART_UPDATE_INTERVAL_MS) {
        last_uart_update = ms_counter;
        uart_update_flag = 1;
    }
}

/**
 * @brief   Convert integer to decimal string
 * @param   value: Value to convert
 * @param   buf:   Output buffer
 * @param   size:  Buffer size
 * @return  Number of characters written
 */
static int uint_to_str(uint32_t value, char *buf, int size) {
    char temp[12];
    int idx = 0;
    int out_idx = 0;
    
    if (value == 0) {
        if (size >= 2) {
            buf[0] = '0';
            buf[1] = '\0';
            return 1;
        }
        return 0;
    }
    
    while (value > 0 && idx < 11) {
        temp[idx++] = '0' + (value % 10);
        value /= 10;
    }
    
    while (idx > 0 && out_idx < size - 1) {
        buf[out_idx++] = temp[--idx];
    }
    buf[out_idx] = '\0';
    
    return out_idx;
}

/**
 * @brief   Print timer statistics over UART
 */
static void print_stats(void) {
    char num_buf[12];
    
    BSP_UART_PutString("Time: ");
    uint_to_str(ms_counter, num_buf, sizeof(num_buf));
    BSP_UART_PutString(num_buf);
    BSP_UART_PutString(" ms (");
    uint_to_str(ms_counter / 1000, num_buf, sizeof(num_buf));
    BSP_UART_PutString(num_buf);
    BSP_UART_PutString(" sec)\r\n");
}

/*============================================================================*/
/*                          MAIN FUNCTION                                      */
/*============================================================================*/

int main(void) {
    BSP_Init();
    BSP_UART_Init();
    
    BSP_UART_PutString("\r\n");
    BSP_UART_PutString("================================\r\n");
    BSP_UART_PutString("  Timer Interrupt Example\r\n");
    BSP_UART_PutString("================================\r\n");
    BSP_UART_PutString("\r\n");
    BSP_UART_PutString("TIM2 Configuration:\r\n");
    BSP_UART_PutString("  Prescaler: 8399\r\n");
    BSP_UART_PutString("  Auto-reload: 9\r\n");
    BSP_UART_PutString("  Interrupt: 1 kHz (1 ms)\r\n");
    BSP_UART_PutString("\r\n");
    BSP_UART_PutString("LED toggles every 500ms\r\n");
    BSP_UART_PutString("Time printed every 1000ms\r\n");
    BSP_UART_PutString("\r\n");
    
    HAL_Timer_SetCallback(timer_callback);
    HAL_Timer_Init();
    HAL_Timer_Start();
    
    BSP_UART_PutString("Timer started!\r\n\r\n");
    
    while (1) {
        if (uart_update_flag) {
            uart_update_flag = 0;
            print_stats();
        }
        
        if (BSP_UART_DataAvailable()) {
            uint8_t ch = BSP_UART_GetChar();
            if (ch == 'r' || ch == 'R') {
                ms_counter = 0;
                last_led_toggle = 0;
                last_uart_update = 0;
                BSP_UART_PutString("Counter reset!\r\n");
            }
        }
        
        __WFI();
    }
    
    return 0;
}

/*============================================================================*/
/*                          INTERRUPT HANDLERS                                 */
/*============================================================================*/

void SysTick_Handler(void) {
    BSP_IncrementTick();
}

void TIM2_IRQHandler(void) {
    HAL_Timer_IRQHandler();
}
