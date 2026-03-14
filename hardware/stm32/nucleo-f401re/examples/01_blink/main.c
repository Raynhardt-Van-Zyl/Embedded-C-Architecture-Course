/**
 * @file    main.c
 * @brief   LED Blink Example for STM32F401RE Nucleo
 * @details This example demonstrates:
 *          - Basic BSP initialization sequence
 *          - Non-blocking delay using SysTick timer
 *          - LED control (on/off/toggle)
 * 
 * Hardware Configuration:
 *   - LED: PA5 (LD2 - Green LED on Nucleo board)
 *   - SysTick: System timer for millisecond delays
 * 
 * Startup Sequence (what happens before main()):
 *   1. Power-on reset
 *   2. CPU reads initial SP from address 0x00000000 (stack top)
 *   3. CPU reads initial PC from address 0x00000004 (Reset_Handler)
 *   4. Reset_Handler executes (from startup code):
 *      a. Copy .data section from Flash to RAM
 *      b. Zero-fill .bss section in RAM
 *      c. Call SystemInit() (clock configuration, if defined)
 *      d. Call __main() / main()
 * 
 * BSP_Init() Sequence:
 *   1. BSP_SystemClock_Config():
 *      - Enable HSI (16 MHz internal RC) as fallback
 *      - Enable HSE (8 MHz from ST-LINK MCO)
 *      - Configure Flash latency for 84 MHz (2 wait states)
 *      - Configure PLL: HSE/8 * 336 / 4 = 84 MHz
 *      - Select PLL as system clock source
 *   2. BSP_SysTick_Init():
 *      - Configure SysTick for 1ms interrupts
 *      - Enable SysTick interrupt and counter
 *   3. BSP_GPIO_Init():
 *      - Enable GPIOA, GPIOB, GPIOC clocks
 *      - Configure PA5 (LED) as output, push-pull, high speed
 *      - Configure PC13 (Button) as input with pull-up
 *      - Configure PA2/PA3 (UART) as alternate function AF7
 *      - Configure PB6/PB7 (I2C) as alternate function AF4, open-drain
 * 
 * Expected Behavior:
 *   - Green LED (LD2) blinks at 1 Hz (500ms on, 500ms off)
 * 
 * @author  Embedded C Architecture Course
 * @version 1.0
 */

#include "bsp.h"

/*============================================================================*/
/*                          CONFIGURATION                                      */
/*============================================================================*/

#define BLINK_DELAY_MS      (500U)

/*============================================================================*/
/*                          MAIN FUNCTION                                      */
/*============================================================================*/

int main(void) {
    BSP_Init();
    
    while (1) {
        BSP_LED_On();
        BSP_Delay(BLINK_DELAY_MS);
        
        BSP_LED_Off();
        BSP_Delay(BLINK_DELAY_MS);
    }
    
    return 0;
}

/*============================================================================*/
/*                          INTERRUPT HANDLERS                                 */
/*============================================================================*/

/**
 * @brief   SysTick interrupt handler
 * @details Called every 1ms when SysTick counter reaches zero.
 *          Must call BSP_IncrementTick() to update the millisecond counter
 *          used by BSP_Delay().
 * 
 * @note    The SysTick timer is configured in BSP_SysTick_Init():
 *          - Reload value: 84000 (84 MHz / 1000 Hz = 84000)
 *          - Clock source: Processor clock (84 MHz)
 *          - Interrupt: Enabled
 *          - Counter: Enabled
 * 
 *          When the counter reaches zero:
 *          1. COUNTFLAG is set (can be read from SysTick->CTRL)
 *          2. Interrupt is triggered (this handler)
 *          3. Counter is reloaded from LOAD register
 */
void SysTick_Handler(void) {
    BSP_IncrementTick();
}
