/**
 * @file startup_stm32f401xe.c
 * @brief Startup code for STM32F401RE
 * 
 * This file contains the vector table and the Reset_Handler that initializes
 * the C runtime environment (copy .data, zero .bss, call constructors).
 * 
 * @author Embedded C Architecture Course
 */

#include <stdint.h>

/* Symbols from the linker script */
extern uint32_t _sidata; /* Start of .data in Flash (LMA) */
extern uint32_t _sdata;  /* Start of .data in RAM (VMA) */
extern uint32_t _edata;  /* End of .data in RAM */
extern uint32_t _sbss;   /* Start of .bss in RAM */
extern uint32_t _ebss;   /* End of .bss in RAM */
extern uint32_t _estack; /* End of stack defined by linker script (top of RAM) */

/* The entry point of the main application */
extern int main(void);

/* CMSIS SystemInit if available */
extern void SystemInit(void) __attribute__((weak));

/**
 * @brief Default interrupt handler (infinite loop)
 */
void Default_Handler(void) {
    while (1);
}

/* Weak definitions for all peripheral interrupts */
void Reset_Handler(void);
void NMI_Handler(void)             __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void)      __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)             __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)          __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)         __attribute__((weak, alias("Default_Handler")));

/* Peripheral Interrupts (truncated for brevity, but includes the main ones) */
void WWDG_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void PVD_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void TAMP_STAMP_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void RTC_WKUP_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void EXTI0_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void EXTI1_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void EXTI2_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void EXTI3_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void EXTI4_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ADC_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void EXTI9_5_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void TIM1_BRK_TIM9_IRQHandler(void)__attribute__((weak, alias("Default_Handler")));
void TIM1_UP_TIM10_IRQHandler(void)__attribute__((weak, alias("Default_Handler")));
void TIM1_TRG_COM_TIM11_IRQHandler(void)__attribute__((weak, alias("Default_Handler")));
void TIM1_CC_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void TIM3_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void TIM4_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void I2C1_EV_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void I2C1_ER_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void I2C2_EV_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void I2C2_ER_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void SPI2_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void EXTI15_10_IRQHandler(void)    __attribute__((weak, alias("Default_Handler")));
void RTC_Alarm_IRQHandler(void)    __attribute__((weak, alias("Default_Handler")));
void OTG_FS_WKUP_IRQHandler(void)  __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream7_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SDIO_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void TIM5_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void SPI3_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void OTG_FS_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream7_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART6_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void I2C3_EV_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void I2C3_ER_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void FPU_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void SPI4_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));

/* Vector table (placed in .isr_vector section) */
__attribute__((section(".isr_vector")))
void (* const vector_table[])(void) = {
    (void (*)(void))0x20018000,   /* Top of Stack (96K RAM) */
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,                   /* Reserved */
    SVC_Handler,
    DebugMon_Handler,
    0,                            /* Reserved */
    PendSV_Handler,
    SysTick_Handler,
    WWDG_IRQHandler,              /* WWDG */
    PVD_IRQHandler,               /* PVD */
    TAMP_STAMP_IRQHandler,        /* TAMP_STAMP */
    RTC_WKUP_IRQHandler,          /* RTC_WKUP */
    FLASH_IRQHandler,             /* FLASH */
    RCC_IRQHandler,               /* RCC */
    EXTI0_IRQHandler,             /* EXTI0 */
    EXTI1_IRQHandler,             /* EXTI1 */
    EXTI2_IRQHandler,             /* EXTI2 */
    EXTI3_IRQHandler,             /* EXTI3 */
    EXTI4_IRQHandler,             /* EXTI4 */
    DMA1_Stream0_IRQHandler,      /* DMA1_Stream0 */
    DMA1_Stream1_IRQHandler,      /* DMA1_Stream1 */
    DMA1_Stream2_IRQHandler,      /* DMA1_Stream2 */
    DMA1_Stream3_IRQHandler,      /* DMA1_Stream3 */
    DMA1_Stream4_IRQHandler,      /* DMA1_Stream4 */
    DMA1_Stream5_IRQHandler,      /* DMA1_Stream5 */
    DMA1_Stream6_IRQHandler,      /* DMA1_Stream6 */
    ADC_IRQHandler,               /* ADC */
    0, 0, 0, 0,                   /* Reserved */
    EXTI9_5_IRQHandler,           /* EXTI9_5 */
    TIM1_BRK_TIM9_IRQHandler,     /* TIM1_BRK_TIM9 */
    TIM1_UP_TIM10_IRQHandler,     /* TIM1_UP_TIM10 */
    TIM1_TRG_COM_TIM11_IRQHandler, /* TIM1_TRG_COM_TIM11 */
    TIM1_CC_IRQHandler,           /* TIM1_CC */
    TIM2_IRQHandler,              /* TIM2 */
    TIM3_IRQHandler,              /* TIM3 */
    TIM4_IRQHandler,              /* TIM4 */
    I2C1_EV_IRQHandler,           /* I2C1_EV */
    I2C1_ER_IRQHandler,           /* I2C1_ER */
    I2C2_EV_IRQHandler,           /* I2C2_EV */
    I2C2_ER_IRQHandler,           /* I2C2_ER */
    SPI1_IRQHandler,              /* SPI1 */
    SPI2_IRQHandler,              /* SPI2 */
    USART1_IRQHandler,            /* USART1 */
    USART2_IRQHandler,            /* USART2 */
    0,                            /* Reserved */
    EXTI15_10_IRQHandler,         /* EXTI15_10 */
    RTC_Alarm_IRQHandler,         /* RTC_Alarm */
    OTG_FS_WKUP_IRQHandler,       /* OTG_FS_WKUP */
    0, 0, 0, 0,                   /* Reserved */
    DMA1_Stream7_IRQHandler,      /* DMA1_Stream7 */
    0,                            /* Reserved */
    SDIO_IRQHandler,              /* SDIO */
    TIM5_IRQHandler,              /* TIM5 */
    SPI3_IRQHandler,              /* SPI3 */
    0, 0, 0, 0,                   /* Reserved */
    DMA2_Stream0_IRQHandler,      /* DMA2_Stream0 */
    DMA2_Stream1_IRQHandler,      /* DMA2_Stream1 */
    DMA2_Stream2_IRQHandler,      /* DMA2_Stream2 */
    DMA2_Stream3_IRQHandler,      /* DMA2_Stream3 */
    DMA2_Stream4_IRQHandler,      /* DMA2_Stream4 */
    0, 0,                         /* Reserved */
    OTG_FS_IRQHandler,            /* OTG_FS */
    DMA2_Stream5_IRQHandler,      /* DMA2_Stream5 */
    DMA2_Stream6_IRQHandler,      /* DMA2_Stream6 */
    DMA2_Stream7_IRQHandler,      /* DMA2_Stream7 */
    USART6_IRQHandler,            /* USART6 */
    I2C3_EV_IRQHandler,           /* I2C3_EV */
    I2C3_ER_IRQHandler,           /* I2C3_ER */
    0, 0, 0, 0, 0, 0, 0,          /* Reserved */
    FPU_IRQHandler,               /* FPU */
    0, 0,                         /* Reserved */
    SPI4_IRQHandler               /* SPI4 */
};

/**
 * @brief Reset Handler
 * 
 * This is the first code that runs on power-up/reset.
 */
void Reset_Handler(void) {
    /* Copy the data segment initializers from flash to SRAM */
    uint32_t *src = &_sidata;
    uint32_t *dest = &_sdata;
    while (dest < &_edata) {
        *dest++ = *src++;
    }

    /* Zero fill the bss segment */
    dest = &_sbss;
    while (dest < &_ebss) {
        *dest++ = 0;
    }

    /* System initialization (clocks, etc) */
    if (SystemInit) {
        SystemInit();
    }

    /* Call the application's entry point */
    main();

    /* Should never reach here */
    while (1);
}
