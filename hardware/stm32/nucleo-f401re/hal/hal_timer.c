/**
 * @file hal_timer.c
 * @brief Timer Hardware Abstraction Layer Implementation for STM32F401RE
 * 
 * This file implements TIM2-based timing functions for the STM32F401RE.
 * 
 * Key Implementation Details:
 *   - TIM2 configured as 32-bit down-counter with auto-reload
 *   - 1 ms tick rate for system timing
 *   - NVIC interrupt enabled for update event
 *   - Millisecond counter maintained in ISR
 * 
 * Timer Clock Path:
 *   SYSCLK (84 MHz) → AHB (84 MHz) → APB1 (42 MHz) → TIM2 (84 MHz*)
 *   *Timer clock is doubled when APB1 prescaler ≠ 1
 * 
 * @author Embedded Systems Course
 * @date 2024
 */

#include "hal_timer.h"

/*============================================================================*/
/*                          PRIVATE VARIABLES                                  */
/*============================================================================*/

/**
 * Millisecond counter
 * Incremented in timer ISR, rolls over after ~49 days
 */
static volatile uint32_t g_millis_count = 0U;

/**
 * User callback function pointer
 * Called from timer ISR on each tick
 */
static Timer_Callback_t g_timer_callback = (Timer_Callback_t)0;

/*============================================================================*/
/*                          PUBLIC FUNCTION IMPLEMENTATIONS                    */
/*============================================================================*/

/**
 * @brief Initialize TIM2 for 1ms system tick
 * 
 * Complete initialization sequence with detailed register explanations:
 * 
 * INITIALIZATION ORDER (from Reference Manual):
 *   1. Enable TIM2 clock
 *   2. Configure prescaler (PSC)
 *   3. Configure auto-reload (ARR)
 *   4. Configure control registers (CR1)
 *   5. Enable update interrupt (DIER)
 *   6. Enable NVIC interrupt
 *   7. Generate update event to load registers
 *   8. Start counter
 */
void HAL_Timer_Init(void)
{
    /*
     * ==================== STEP 1: Enable TIM2 Clock ====================
     * 
     * TIM2 is on APB1 bus.
     * Set bit 0 in APB1ENR to enable TIM2 clock.
     */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    
    /*
     * ==================== STEP 2: Disable Counter ====================
     * 
     * Clear CEN bit to ensure timer is stopped during configuration.
     * This prevents unexpected behavior during setup.
     */
    TIM2->CR1 &= ~TIM_CR1_CEN;
    
    /*
     * ==================== STEP 3: Configure Prescaler (PSC) ====================
     * 
     * Prescaler divides the timer input clock.
     * PSC is a 16-bit register (0-65535).
     * 
     * Formula: f_after_psc = f_input / (PSC + 1)
     * 
     * With 84 MHz input and PSC = 83:
     *   f_after_psc = 84,000,000 / (83 + 1)
     *   f_after_psc = 84,000,000 / 84
     *   f_after_psc = 1,000,000 Hz = 1 MHz
     * 
     * This gives us 1 µs resolution before the auto-reload divider.
     */
    TIM2->PSC = 83U;  /* Divide 84 MHz by 84 → 1 MHz */
    
    /*
     * ==================== STEP 4: Configure Auto-Reload (ARR) ====================
     * 
     * ARR determines when the counter reloads and generates update event.
     * ARR is 32-bit for TIM2 (16-bit for most other timers).
     * 
     * Formula: f_out = f_after_psc / (ARR + 1)
     * 
     * For 1 kHz (1 ms period) with 1 MHz after prescaler:
     *   f_out = 1,000,000 / (ARR + 1) = 1000
     *   ARR + 1 = 1000
     *   ARR = 999
     * 
     * Counter counts from 0 to ARR (inclusive), then reloads.
     * Total counts per cycle = ARR + 1 = 1000
     * Period = 1000 × 1 µs = 1 ms
     */
    TIM2->ARR = 999U;  /* Count 0-999, period = 1000 × 1 µs = 1 ms */
    
    /*
     * ==================== STEP 5: Configure Control Register 1 ====================
     * 
     * CR1 Configuration:
     *   - ARPE = 1: Enable ARR preload buffer
     *     This ensures ARR changes take effect at next update event,
     *     preventing glitches if ARR is changed while counting.
     *   
     *   - URS = 1: Only counter overflow generates update interrupt
     *     This prevents software-generated updates (UG bit) from
     *     causing interrupts.
     *   
     *   - UDIS = 0: Update events enabled (default)
     *   
     *   - OPM = 0: Continuous mode (not one-pulse)
     *   
     *   - DIR = 0: Up-counting (counts 0 → ARR)
     */
    TIM2->CR1 = TIM_CR1_ARPE | TIM_CR1_URS;
    
    /*
     * ==================== STEP 6: Clear Status Flags ====================
     * 
     * Clear any pending interrupt flags before enabling interrupts.
     * This prevents immediate interrupt on enable.
     * 
     * Writing 0 to UIF clears the flag.
     */
    TIM2->SR = 0U;
    
    /*
     * ==================== STEP 7: Enable Update Interrupt ====================
     * 
     * Set UIE (Update Interrupt Enable) bit in DIER.
     * This enables interrupt generation on update events.
     * 
     * Update event occurs when counter overflows (reaches ARR).
     */
    TIM2->DIER = TIM_DIER_UIE;
    
    /*
     * ==================== STEP 8: Enable NVIC Interrupt ====================
     * 
     * Enable TIM2 interrupt in the Nested Vectored Interrupt Controller.
     * 
     * NVIC ISER (Interrupt Set Enable Register):
     *   - Each bit controls one interrupt
     *   - Writing 1 enables the interrupt
     *   - TIM2 is IRQ 28
     *   - IRQ 28 is in ISER[0] (handles IRQs 0-31)
     *   - Bit position in ISER[0] = 28
     */
    NVIC->ISER[TIM2_IRQn_REG_INDEX] = (1U << TIM2_IRQn);
    
    /*
     * ==================== STEP 9: Generate Update Event ====================
     * 
     * Set UG (Update Generation) bit in EGR.
     * This transfers PSC and ARR from shadow to active registers.
     * 
     * Note: This also sets UIF if URS=0, but with URS=1 it doesn't.
     * However, we cleared SR above to be safe.
     */
    TIM2->EGR = TIM_EGR_UG;
    
    /*
     * ==================== STEP 10: Clear UIF Flag Again ====================
     * 
     * The UG event may have set UIF (depending on URS).
     * Clear it to prevent immediate interrupt on start.
     */
    TIM2->SR = 0U;
    
    /*
     * Note: Counter is NOT started here.
     * Call HAL_Timer_Start() to begin counting.
     */
}

/**
 * @brief Set callback for timer interrupt
 * 
 * Stores the callback function pointer for use in ISR.
 * Pass NULL to disable callback.
 * 
 * @param callback Function pointer to call on timer tick
 */
void HAL_Timer_SetCallback(Timer_Callback_t callback)
{
    g_timer_callback = callback;
}

/**
 * @brief Start TIM2 counter
 * 
 * Sets CEN (Counter Enable) bit in CR1.
 * Timer begins counting from 0 toward ARR.
 */
void HAL_Timer_Start(void)
{
    /*
     * Reset counter to 0 for consistent timing.
     * Without this, first period may be shorter.
     */
    TIM2->CNT = 0U;
    
    /*
     * Set CEN bit to enable counting.
     * Counter now increments from 0 to ARR, then reloads.
     */
    TIM2->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief Stop TIM2 counter
 * 
 * Clears CEN bit in CR1.
 * Counter value is preserved (can be read with GetCounter).
 */
void HAL_Timer_Stop(void)
{
    /*
     * Clear CEN bit to disable counting.
     * Counter stops immediately at current value.
     */
    TIM2->CR1 &= ~TIM_CR1_CEN;
}

/**
 * @brief Get current counter value
 * 
 * Reads the CNT register.
 * Useful for measuring elapsed time within a tick.
 * 
 * @return Current counter value (0 to ARR)
 */
uint32_t HAL_Timer_GetCounter(void)
{
    return TIM2->CNT;
}

/**
 * @brief Set counter value
 * 
 * Writes to CNT register.
 * Can be used to reset or adjust counter.
 * 
 * @param value New counter value
 */
void HAL_Timer_SetCounter(uint32_t value)
{
    TIM2->CNT = value;
}

/**
 * @brief Get elapsed milliseconds since timer start
 * 
 * Returns the millisecond counter maintained in ISR.
 * This counter rolls over after ~49.7 days.
 * 
 * Thread Safety:
 *   Reading a 32-bit value is atomic on Cortex-M4.
 *   No critical section needed for read.
 * 
 * @return Milliseconds since timer start
 */
uint32_t HAL_Timer_GetMillis(void)
{
    return g_millis_count;
}

/**
 * @brief Blocking delay in milliseconds
 * 
 * Uses the millisecond counter for precise delay.
 * More accurate than busy-wait loops.
 * 
 * Note: This is a blocking function.
 * In RTOS environments, use OS delay functions instead.
 * 
 * @param ms Number of milliseconds to delay
 */
void HAL_Timer_Delay(uint32_t ms)
{
    uint32_t start = g_millis_count;
    uint32_t elapsed;
    
    /*
     * Wait until elapsed time reaches desired delay.
     * Handle counter rollover correctly using subtraction.
     * 
     * Example with rollover:
     *   start = 0xFFFFFFF0
     *   After 32 ms: g_millis_count = 0x00000010
     *   elapsed = 0x00000010 - 0xFFFFFFF0 = 32 (correct!)
     * 
     * This works because unsigned subtraction wraps correctly.
     */
    do
    {
        elapsed = g_millis_count - start;
    } while (elapsed < ms);
}

/**
 * @brief Timer interrupt handler
 * 
 * Called from TIM2_IRQHandler (which user must implement).
 * 
 * Actions:
 *   1. Check if update interrupt flag is set
 *   2. Clear the interrupt flag
 *   3. Increment millisecond counter
 *   4. Call user callback if registered
 * 
 * ISR Best Practices:
 *   - Keep ISRs short and fast
 *   - Clear flags before doing work (or after, consistently)
 *   - Avoid function calls that may block
 *   - Don't use floating point in ISR
 */
void HAL_Timer_IRQHandler(void)
{
    /*
     * Check if update interrupt flag is set.
     * UIF is set when counter overflows (reaches ARR + 1).
     */
    if ((TIM2->SR & TIM_SR_UIF) != 0U)
    {
        /*
         * Clear the interrupt flag by writing 0 to UIF.
         * 
         * IMPORTANT: Clear flag BEFORE incrementing counter and
         * calling callback. This prevents missing interrupts if
         * the callback takes too long.
         * 
         * If we cleared after, and callback took > 1ms, we might
         * miss the next tick.
         */
        TIM2->SR &= ~TIM_SR_UIF;
        
        /*
         * Increment millisecond counter.
         * This wraps naturally at 0xFFFFFFFF.
         */
        g_millis_count++;
        
        /*
         * Call user callback if registered.
         * 
         * WARNING: Keep callbacks short!
         * Long callbacks will delay other interrupts and
         * may cause system timing issues.
         */
        if (g_timer_callback != (Timer_Callback_t)0)
        {
            g_timer_callback();
        }
    }
}

/*============================================================================*/
/*                          EXAMPLE ISR IMPLEMENTATION                         */
/*============================================================================*/

/*
 * The user must implement the actual interrupt handler in their code:
 * 
 * // In startup file, ensure TIM2_IRQHandler is in vector table
 * 
 * void TIM2_IRQHandler(void)
 * {
 *     HAL_Timer_IRQHandler();
 * }
 * 
 * Or for more control:
 * 
 * void TIM2_IRQHandler(void)
 * {
 *     // Handle other timer interrupts here if needed
 *     
 *     if (TIM2->SR & TIM_SR_UIF)
 *     {
 *         TIM2->SR &= ~TIM_SR_UIF;
 *         // Custom handling...
 *     }
 * }
 */

/*============================================================================*/
/*                          UTILITY FUNCTIONS                                  */
/*============================================================================*/

/**
 * Example: Convert milliseconds to timer ticks
 * 
 * For applications needing sub-millisecond timing, use the counter directly.
 * 
 * uint32_t start = HAL_Timer_GetCounter();
 * // ... do something ...
 * uint32_t end = HAL_Timer_GetCounter();
 * uint32_t elapsed_us = (end - start); // In microseconds (1 MHz counter)
 */
