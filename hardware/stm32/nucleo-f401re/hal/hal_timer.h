/**
 * @file hal_timer.h
 * @brief Timer Hardware Abstraction Layer for STM32F401RE
 * 
 * This module provides basic timer functionality using TIM2.
 * TIM2 is a 32-bit general-purpose timer with interrupt support.
 * 
 * TIM2 Features:
 *   - 32-bit auto-reload counter (can be used as 16-bit)
 *   - Programmable prescaler (16-bit, divide by 1-65536)
 *   - Multiple interrupt sources (update, capture/compare)
 *   - One-pulse mode support
 * 
 * Clock Configuration:
 *   - TIM2 is on APB1 bus
 *   - Timer clock = APB1 clock × multiplier
 *   - If APB1 prescaler ≠ 1, timer clock = APB1 × 2
 *   - With APB1 = 42 MHz (prescaler = 2), timer clock = 84 MHz
 * 
 * Timing Calculation for 1ms tick:
 *   Target frequency: 1000 Hz (1 ms period)
 *   Timer clock: 84 MHz
 *   
 *   Formula: f_timer = f_clock / ((PSC + 1) × (ARR + 1))
 *   
 *   For 1 ms with 84 MHz:
 *   PSC = 8399 (divide by 8400 → 10 kHz)
 *   ARR = 9 (count 0-9 → 1 kHz / 1000 Hz / 1 ms)
 *   
 *   Or:
 *   PSC = 83 (divide by 84 → 1 MHz)
 *   ARR = 999 (count 0-999 → 1 kHz / 1000 Hz / 1 ms)
 * 
 * @author Embedded Systems Course
 * @date 2024
 */

#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/*                          CONFIGURATION CONSTANTS                           */
/*============================================================================*/

/**
 * Timer Base Addresses
 * From STM32F401 Reference Manual (RM0368)
 * 
 * TIM2:  32-bit general-purpose timer
 * TIM3:  16-bit general-purpose timer
 * TIM4:  16-bit general-purpose timer
 * TIM5:  32-bit general-purpose timer
 * TIM9:  16-bit general-purpose timer
 * TIM10: 16-bit general-purpose timer
 * TIM11: 16-bit general-purpose timer
 */
#define TIM1_BASE           (0x40010000UL)
#define TIM2_BASE           (0x40000000UL)
#define TIM3_BASE           (0x40000400UL)
#define TIM4_BASE           (0x40000800UL)
#define TIM5_BASE           (0x40000C00UL)
#define TIM9_BASE           (0x40014000UL)
#define TIM10_BASE          (0x40014400UL)
#define TIM11_BASE          (0x40014800UL)

/**
 * RCC Base Address
 */
#define RCC_BASE            (0x40023800UL)

/**
 * NVIC (Nested Vectored Interrupt Controller) Base Address
 */
#define NVIC_BASE           (0xE000E100UL)

/*============================================================================*/
/*                          REGISTER DEFINITIONS                               */
/*============================================================================*/

/**
 * TIM Register Structure
 * 
 * Register offsets from Reference Manual:
 *   CR1:    0x00 - Control register 1
 *   CR2:    0x04 - Control register 2
 *   DIER:   0x0C - DMA/Interrupt enable register
 *   SR:     0x10 - Status register
 *   EGR:    0x14 - Event generation register
 *   CCMR1:  0x18 - Capture/compare mode register 1
 *   CCMR2:  0x1C - Capture/compare mode register 2
 *   CCER:   0x20 - Capture/compare enable register
 *   CNT:    0x24 - Counter register
 *   PSC:    0x28 - Prescaler register
 *   ARR:    0x2C - Auto-reload register
 *   CCR1-4: 0x34-0x40 - Capture/compare registers
 *   DCR:    0x48 - DMA control register
 *   DMAR:   0x4C - DMA address register
 */
typedef struct {
    volatile uint32_t CR1;      /* Offset 0x00: Control register 1 */
    volatile uint32_t CR2;      /* Offset 0x04: Control register 2 */
    volatile uint32_t SMCR;     /* Offset 0x08: Slave mode control register */
    volatile uint32_t DIER;     /* Offset 0x0C: DMA/Interrupt enable register */
    volatile uint32_t SR;       /* Offset 0x10: Status register */
    volatile uint32_t EGR;      /* Offset 0x14: Event generation register */
    volatile uint32_t CCMR1;    /* Offset 0x18: Capture/compare mode register 1 */
    volatile uint32_t CCMR2;    /* Offset 0x1C: Capture/compare mode register 2 */
    volatile uint32_t CCER;     /* Offset 0x20: Capture/compare enable register */
    volatile uint32_t CNT;      /* Offset 0x24: Counter register */
    volatile uint32_t PSC;      /* Offset 0x28: Prescaler register */
    volatile uint32_t ARR;      /* Offset 0x2C: Auto-reload register */
    volatile uint32_t reserved1;
    volatile uint32_t CCR1;     /* Offset 0x34: Capture/compare register 1 */
    volatile uint32_t CCR2;     /* Offset 0x38: Capture/compare register 2 */
    volatile uint32_t CCR3;     /* Offset 0x3C: Capture/compare register 3 */
    volatile uint32_t CCR4;     /* Offset 0x40: Capture/compare register 4 */
    volatile uint32_t reserved2;
    volatile uint32_t DCR;      /* Offset 0x48: DMA control register */
    volatile uint32_t DMAR;     /* Offset 0x4C: DMA address register */
} TIM_Regs_t;

/**
 * RCC Register Structure (partial)
 */
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    uint8_t          reserved[0x24];
    volatile uint32_t AHB1ENR;
    uint8_t          reserved2[0x08];
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
} RCC_Regs_t;

/**
 * NVIC Register Structure (partial - ISER and ICER only)
 * 
 * NVIC registers are used for enabling/disabling interrupts.
 */
typedef struct {
    volatile uint32_t ISER[8];  /* Interrupt Set Enable Registers */
    uint8_t          reserved[0x60];
    volatile uint32_t ICER[8];  /* Interrupt Clear Enable Registers */
} NVIC_Regs_t;

/* Peripheral pointers */
#define TIM1                ((TIM_Regs_t *)TIM1_BASE)
#define TIM2                ((TIM_Regs_t *)TIM2_BASE)
#define TIM3                ((TIM_Regs_t *)TIM3_BASE)
#define TIM4                ((TIM_Regs_t *)TIM4_BASE)
#define TIM5                ((TIM_Regs_t *)TIM5_BASE)
#define RCC                 ((RCC_Regs_t *)RCC_BASE)
#define NVIC                ((NVIC_Regs_t *)NVIC_BASE)

/*============================================================================*/
/*                          TIM CR1 REGISTER BITS                              */
/*============================================================================*/

/**
 * Control Register 1 (CR1) Bit Definitions
 */
#define TIM_CR1_CEN         (1U << 0)    /* Counter enable */
#define TIM_CR1_UDIS        (1U << 1)    /* Update disable */
#define TIM_CR1_URS         (1U << 2)    /* Update request source */
#define TIM_CR1_OPM         (1U << 3)    /* One-pulse mode */
#define TIM_CR1_DIR         (1U << 4)    /* Direction (0=up, 1=down) */
#define TIM_CR1_CMS_1       (1U << 5)    /* Center-aligned mode bit 0 */
#define TIM_CR1_CMS_2       (1U << 6)    /* Center-aligned mode bit 1 */
#define TIM_CR1_ARPE        (1U << 7)    /* Auto-reload preload enable */
#define TIM_CR1_CKD_1       (1U << 8)    /* Clock division bit 0 */
#define TIM_CR1_CKD_2       (1U << 9)    /* Clock division bit 1 */

/*
 * Key CR1 Bits Explained:
 * 
 * CEN (Counter Enable):
 *   - 0: Counter disabled
 *   - 1: Counter enabled
 *   - Must be set to start counting
 * 
 * UDIS (Update Disable):
 *   - 0: Update events enabled (UEV)
 *   - 1: Update events disabled
 *   - When disabled, ARR shadow not updated
 * 
 * URS (Update Request Source):
 *   - 0: Any update event generates interrupt
 *   - 1: Only counter overflow/underflow generates interrupt
 * 
 * OPM (One-Pulse Mode):
 *   - 0: Counter not stopped at update event
 *   - 1: Counter stops at next update event
 * 
 * ARPE (Auto-Reload Preload Enable):
 *   - 0: ARR register not buffered
 *   - 1: ARR register buffered (shadow)
 *   - Enable for glitch-free ARR changes
 */

/*============================================================================*/
/*                          TIM DIER REGISTER BITS                             */
/*============================================================================*/

/**
 * DMA/Interrupt Enable Register (DIER) Bit Definitions
 */
#define TIM_DIER_UIE        (1U << 0)    /* Update interrupt enable */
#define TIM_DIER_CC1IE      (1U << 1)    /* Capture/compare 1 interrupt enable */
#define TIM_DIER_CC2IE      (1U << 2)    /* Capture/compare 2 interrupt enable */
#define TIM_DIER_CC3IE      (1U << 3)    /* Capture/compare 3 interrupt enable */
#define TIM_DIER_CC4IE      (1U << 4)    /* Capture/compare 4 interrupt enable */
#define TIM_DIER_TIE        (1U << 6)    /* Trigger interrupt enable */
#define TIM_DIER_UDE        (1U << 8)    /* Update DMA request enable */

/*
 * DIER Controls:
 *   - Setting a bit enables the corresponding interrupt/DMA request
 *   - UIE: Update interrupt (triggered on counter overflow/reload)
 *   - CCxIE: Capture/compare interrupt (triggered on capture/compare event)
 */

/*============================================================================*/
/*                          TIM SR REGISTER BITS                               */
/*============================================================================*/

/**
 * Status Register (SR) Bit Definitions
 */
#define TIM_SR_UIF          (1U << 0)    /* Update interrupt flag */
#define TIM_SR_CC1IF        (1U << 1)    /* Capture/compare 1 interrupt flag */
#define TIM_SR_CC2IF        (1U << 2)    /* Capture/compare 2 interrupt flag */
#define TIM_SR_CC3IF        (1U << 3)    /* Capture/compare 3 interrupt flag */
#define TIM_SR_CC4IF        (1U << 4)    /* Capture/compare 4 interrupt flag */
#define TIM_SR_TIF          (1U << 6)    /* Trigger interrupt flag */
#define TIM_SR_CC1OF        (1U << 9)    /* Capture/compare 1 overcapture flag */

/*
 * Status Flags:
 * 
 * UIF (Update Interrupt Flag):
 *   - Set on counter overflow/underflow
 *   - Also set on UG bit set in EGR
 *   - Cleared by software writing 0
 *   - Must be cleared in ISR to prevent re-entry
 */

/*============================================================================*/
/*                          TIM EGR REGISTER BITS                              */
/*============================================================================*/

/**
 * Event Generation Register (EGR) Bit Definitions
 */
#define TIM_EGR_UG          (1U << 0)    /* Update generation */
#define TIM_EGR_CC1G        (1U << 1)    /* Capture/compare 1 generation */
#define TIM_EGR_CC2G        (1U << 2)    /* Capture/compare 2 generation */
#define TIM_EGR_CC3G        (1U << 3)    /* Capture/compare 3 generation */
#define TIM_EGR_CC4G        (1U << 4)    /* Capture/compare 4 generation */
#define TIM_EGR_TG          (1U << 6)    /* Trigger generation */

/*
 * EGR Usage:
 *   - Writing 1 to UG generates an update event
 *   - This loads PSC and ARR shadow registers
 *   - Also sets UIF if UIE is enabled
 */

/*============================================================================*/
/*                          RCC CLOCK ENABLE BITS                              */
/*============================================================================*/

/**
 * RCC APB1ENR Timer Clock Enable Bits
 */
#define RCC_APB1ENR_TIM2EN      (1U << 0)
#define RCC_APB1ENR_TIM3EN      (1U << 1)
#define RCC_APB1ENR_TIM4EN      (1U << 2)
#define RCC_APB1ENR_TIM5EN      (1U << 3)

/**
 * RCC APB2ENR Timer Clock Enable Bits
 */
#define RCC_APB2ENR_TIM1EN      (1U << 0)
#define RCC_APB2ENR_TIM9EN      (1U << 16)
#define RCC_APB2ENR_TIM10EN     (1U << 17)
#define RCC_APB2ENR_TIM11EN     (1U << 18)

/*============================================================================*/
/*                          NVIC INTERRUPT NUMBERS                             */
/*============================================================================*/

/**
 * TIM2 Interrupt Number
 * From STM32F401 vector table
 * TIM2 global interrupt is IRQ 28
 */
#define TIM2_IRQn               28

/**
 * NVIC ISER/ICER register index for TIM2
 * Each ISER/ICER register handles 32 interrupts
 * IRQ 28 is in register 0 (IRQs 0-31)
 */
#define TIM2_IRQn_REG_INDEX     0

/*============================================================================*/
/*                          CLOCK CONFIGURATION                                */
/*============================================================================*/

/**
 * Timer clock frequency
 * 
 * For STM32F401RE with SYSCLK = 84 MHz:
 *   - APB1 prescaler = 2, so APB1 = 42 MHz
 *   - Timer clock multiplier = 2 (because APB1 prescaler ≠ 1)
 *   - Timer clock = 42 × 2 = 84 MHz
 * 
 * This is the input frequency to the timer prescaler.
 */
#define TIMER_CLOCK_FREQUENCY   84000000UL   /* 84 MHz */

/**
 * Target tick frequency for system tick
 * 1000 Hz = 1 ms per tick
 */
#define TIMER_TICK_FREQUENCY    1000UL       /* 1 kHz = 1 ms */

/*============================================================================*/
/*                          TYPE DEFINITIONS                                   */
/*============================================================================*/

/**
 * Timer callback function type
 * Called from timer interrupt
 */
typedef void (*Timer_Callback_t)(void);

/*============================================================================*/
/*                          PUBLIC FUNCTION PROTOTYPES                         */
/*============================================================================*/

/**
 * @brief Initialize TIM2 for 1ms system tick
 * 
 * Configures TIM2 to generate periodic interrupts at 1ms intervals.
 * 
 * Configuration:
 *   - Clock: 84 MHz (from APB1 × 2 multiplier)
 *   - Prescaler: 8399 (divide by 8400 → 10 kHz)
 *   - Auto-reload: 9 (count 0-9 → 1 kHz)
 *   - Update interrupt enabled
 * 
 * Timing Math:
 *   f_out = f_in / ((PSC + 1) × (ARR + 1))
 *   f_out = 84,000,000 / ((8399 + 1) × (9 + 1))
 *   f_out = 84,000,000 / (8400 × 10)
 *   f_out = 84,000,000 / 84,000
 *   f_out = 1000 Hz (1 ms period)
 * 
 * Alternative calculation for ARR = 999, PSC = 83:
 *   f_out = 84,000,000 / ((83 + 1) × (999 + 1))
 *   f_out = 84,000,000 / (84 × 1000)
 *   f_out = 1000 Hz
 */
void HAL_Timer_Init(void);

/**
 * @brief Set callback for timer interrupt
 * 
 * The callback function is called from the timer ISR.
 * Keep callback functions short to avoid blocking other interrupts.
 * 
 * @param callback Function pointer to call on timer tick
 */
void HAL_Timer_SetCallback(Timer_Callback_t callback);

/**
 * @brief Start TIM2 counter
 * 
 * Sets CEN bit to enable counting.
 * Timer will generate interrupts at configured rate.
 */
void HAL_Timer_Start(void);

/**
 * @brief Stop TIM2 counter
 * 
 * Clears CEN bit to disable counting.
 * Counter value is preserved.
 */
void HAL_Timer_Stop(void);

/**
 * @brief Get current counter value
 * 
 * Returns the current CNT register value.
 * Useful for timing measurements.
 * 
 * @return Current counter value (0 to ARR)
 */
uint32_t HAL_Timer_GetCounter(void);

/**
 * @brief Set counter value
 * 
 * Sets the CNT register to a specific value.
 * 
 * @param value New counter value
 */
void HAL_Timer_SetCounter(uint32_t value);

/**
 * @brief Get elapsed milliseconds since timer start
 * 
 * Returns millisecond counter, incremented in ISR.
 * Rolls over after ~49 days (32-bit).
 * 
 * @return Milliseconds since timer start
 */
uint32_t HAL_Timer_GetMillis(void);

/**
 * @brief Blocking delay in milliseconds
 * 
 * Uses timer counter for precise delay.
 * Not suitable for delays > 10 ms in ISR context.
 * 
 * @param ms Number of milliseconds to delay
 */
void HAL_Timer_Delay(uint32_t ms);

/**
 * @brief Timer interrupt handler
 * 
 * This function should be called from the TIM2 ISR.
 * Clears interrupt flag and calls user callback.
 * 
 * Note: User must implement the actual ISR that calls this:
 *   void TIM2_IRQHandler(void) {
 *       HAL_Timer_IRQHandler();
 *   }
 */
void HAL_Timer_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_TIMER_H */
