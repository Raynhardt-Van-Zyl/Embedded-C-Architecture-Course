/**
 * @file    stm32f401xe.h
 * @brief   STM32F401xE Register Definitions Header File
 * @details Minimal but complete register definitions for STM32F401RE microcontroller.
 *          Uses CMSIS-style naming conventions with direct memory-mapped register access.
 * 
 * @note    This header provides essential register definitions for bare-metal development.
 *          Not all bits are defined - only those required for typical embedded examples.
 * 
 * @author  Embedded C Architecture Course
 * @version 1.0
 */

#ifndef STM32F401XE_H
#define STM32F401XE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*============================================================================*/
/*                          MEMORY MAP CONSTANTS                               */
/*============================================================================*/

#define FLASH_BASE          (0x08000000UL)      /*!< Flash memory base address (512KB) */
#define SRAM_BASE           (0x20000000UL)      /*!< SRAM base address (96KB) */
#define PERIPH_BASE         (0x40000000UL)      /*!< Peripheral base address */
#define APB1PERIPH_BASE     (PERIPH_BASE)       /*!< APB1 peripheral base */
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)  /*!< APB2 peripheral base */
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)  /*!< AHB1 peripheral base */
#define AHB2PERIPH_BASE     (PERIPH_BASE + 0x08000000UL)  /*!< AHB2 peripheral base */

/*============================================================================*/
/*                          SYSTEM CONTROL BLOCK                               */
/*============================================================================*/

#define SCS_BASE            (0xE000E000UL)      /*!< System Control Space base */
#define NVIC_BASE           (SCS_BASE + 0x0E00UL)       /*!< NVIC base address */
#define SCB_BASE            (SCS_BASE + 0x0D00UL)       /*!< System Control Block base */
#define SYSTICK_BASE        (SCS_BASE + 0x0010UL)       /*!< SysTick base address */

/*============================================================================*/
/*                          PERIPHERAL BASE ADDRESSES                          */
/*============================================================================*/

/*--------------------------- TIMERS (APB1) -----------------------------------*/
#define TIM2_BASE           (APB1PERIPH_BASE + 0x0000UL)
#define TIM3_BASE           (APB1PERIPH_BASE + 0x0400UL)
#define TIM4_BASE           (APB1PERIPH_BASE + 0x0800UL)
#define TIM5_BASE           (APB1PERIPH_BASE + 0x0C00UL)

/*--------------------------- TIMERS (APB2) -----------------------------------*/
#define TIM1_BASE           (APB2PERIPH_BASE + 0x0000UL)
#define TIM9_BASE           (APB2PERIPH_BASE + 0x4000UL)
#define TIM10_BASE          (APB2PERIPH_BASE + 0x4400UL)
#define TIM11_BASE          (APB2PERIPH_BASE + 0x4800UL)

/*--------------------------- SPI (APB1/APB2) ---------------------------------*/
#define SPI1_BASE           (APB2PERIPH_BASE + 0x3000UL)
#define SPI2_BASE           (APB1PERIPH_BASE + 0x3800UL)
#define SPI3_BASE           (APB1PERIPH_BASE + 0x3C00UL)

/*--------------------------- I2C (APB1) --------------------------------------*/
#define I2C1_BASE           (APB1PERIPH_BASE + 0x5400UL)
#define I2C2_BASE           (APB1PERIPH_BASE + 0x5800UL)
#define I2C3_BASE           (APB1PERIPH_BASE + 0x5C00UL)

/*--------------------------- USART (APB1/APB2) -------------------------------*/
#define USART1_BASE         (APB2PERIPH_BASE + 0x1000UL)
#define USART2_BASE         (APB1PERIPH_BASE + 0x4400UL)
#define USART6_BASE         (APB2PERIPH_BASE + 0x1400UL)

/*--------------------------- GPIO (AHB1) -------------------------------------*/
#define GPIOA_BASE          (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE          (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE          (AHB1PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE          (AHB1PERIPH_BASE + 0x0C00UL)
#define GPIOE_BASE          (AHB1PERIPH_BASE + 0x1000UL)
#define GPIOH_BASE          (AHB1PERIPH_BASE + 0x1C00UL)

/*--------------------------- RCC (AHB1) --------------------------------------*/
#define RCC_BASE            (AHB1PERIPH_BASE + 0x3800UL)

/*--------------------------- FLASH (AHB1) ------------------------------------*/
#define FLASH_R_BASE        (AHB1PERIPH_BASE + 0x3C00UL)

/*--------------------------- PWR (APB1) --------------------------------------*/
#define PWR_BASE            (APB1PERIPH_BASE + 0x7000UL)

/*--------------------------- ADC (APB2) --------------------------------------*/
#define ADC1_BASE           (APB2PERIPH_BASE + 0x2000UL)
#define ADC_BASE            (APB2PERIPH_BASE + 0x2000UL)  /*!< Common ADC registers */

/*--------------------------- SYSCFG (APB2) -----------------------------------*/
#define SYSCFG_BASE         (APB2PERIPH_BASE + 0x3800UL)

/*--------------------------- EXTI (APB2) -------------------------------------*/
#define EXTI_BASE           (PERIPH_BASE + 0x3C00UL)

/*============================================================================*/
/*                          REGISTER TYPE DEFINITIONS                          */
/*============================================================================*/

/**
 * @brief   Macro to convert a base address to a typed pointer
 * @details This macro casts peripheral base addresses to their respective
 *          register structure pointers for convenient register access.
 */
#define REG32(base, offset)  (*(volatile uint32_t *)((base) + (offset)))

/*============================================================================*/
/*                          GPIO REGISTERS                                     */
/*============================================================================*/

/**
 * @brief   GPIO Register Structure
 * @details Each GPIO port has a set of control and data registers.
 *          The mode register controls digital/analog function.
 *          The output type register controls push-pull/open-drain.
 *          The output speed register controls slew rate.
 *          The pull-up/pull-down register controls internal resistors.
 *          The input data register reflects pin states.
 *          The output data register sets output states.
 *          The bit set/reset register allows atomic bit manipulation.
 *          The alternate function registers select peripheral functions.
 */
typedef struct {
    volatile uint32_t MODER;        /*!< GPIO port mode register,                 Addr offset: 0x00 */
    volatile uint32_t OTYPER;       /*!< GPIO port output type register,          Addr offset: 0x04 */
    volatile uint32_t OSPEEDR;      /*!< GPIO port output speed register,         Addr offset: 0x08 */
    volatile uint32_t PUPDR;        /*!< GPIO port pull-up/pull-down register,    Addr offset: 0x0C */
    volatile uint32_t IDR;          /*!< GPIO port input data register,           Addr offset: 0x10 */
    volatile uint32_t ODR;          /*!< GPIO port output data register,          Addr offset: 0x14 */
    volatile uint32_t BSRR;         /*!< GPIO port bit set/reset register,        Addr offset: 0x18 */
    volatile uint32_t LCKR;         /*!< GPIO port configuration lock register,   Addr offset: 0x1C */
    volatile uint32_t AFRL;         /*!< GPIO alternate function low register,    Addr offset: 0x20 */
    volatile uint32_t AFRH;         /*!< GPIO alternate function high register,   Addr offset: 0x24 */
} GPIO_TypeDef;

/*----------------------------------------------------------------------------*/
/* GPIO MODER Register Bit Definitions                                        */
/*----------------------------------------------------------------------------*/
#define GPIO_MODER_MODE0_Pos            (0U)
#define GPIO_MODER_MODE0_Msk            (0x3UL << GPIO_MODER_MODE0_Pos)
#define GPIO_MODER_MODE0_INPUT          (0x0UL << GPIO_MODER_MODE0_Pos)   /*!< Input mode */
#define GPIO_MODER_MODE0_OUTPUT         (0x1UL << GPIO_MODER_MODE0_Pos)   /*!< General purpose output mode */
#define GPIO_MODER_MODE0_AF             (0x2UL << GPIO_MODER_MODE0_Pos)   /*!< Alternate function mode */
#define GPIO_MODER_MODE0_ANALOG         (0x3UL << GPIO_MODER_MODE0_Pos)   /*!< Analog mode */

/*----------------------------------------------------------------------------*/
/* GPIO OTYPER Register Bit Definitions                                       */
/*----------------------------------------------------------------------------*/
#define GPIO_OTYPER_OT0_Pos             (0U)
#define GPIO_OTYPER_OT0_Msk             (0x1UL << GPIO_OTYPER_OT0_Pos)
#define GPIO_OTYPER_OT0_PP              (0x0UL << GPIO_OTYPER_OT0_Pos)    /*!< Push-pull output */
#define GPIO_OTYPER_OT0_OD              (0x1UL << GPIO_OTYPER_OT0_Pos)    /*!< Open-drain output */

/*----------------------------------------------------------------------------*/
/* GPIO OSPEEDR Register Bit Definitions                                      */
/*----------------------------------------------------------------------------*/
#define GPIO_OSPEEDR_OSPEED0_Pos        (0U)
#define GPIO_OSPEEDR_OSPEED0_Msk        (0x3UL << GPIO_OSPEEDR_OSPEED0_Pos)
#define GPIO_OSPEEDR_OSPEED0_LOW        (0x0UL << GPIO_OSPEEDR_OSPEED0_Pos)   /*!< Low speed */
#define GPIO_OSPEEDR_OSPEED0_MEDIUM     (0x1UL << GPIO_OSPEEDR_OSPEED0_Pos)   /*!< Medium speed */
#define GPIO_OSPEEDR_OSPEED0_HIGH       (0x2UL << GPIO_OSPEEDR_OSPEED0_Pos)   /*!< High speed */
#define GPIO_OSPEEDR_OSPEED0_VERY_HIGH  (0x3UL << GPIO_OSPEEDR_OSPEED0_Pos)   /*!< Very high speed */

/*----------------------------------------------------------------------------*/
/* GPIO PUPDR Register Bit Definitions                                        */
/*----------------------------------------------------------------------------*/
#define GPIO_PUPDR_PUPD0_Pos            (0U)
#define GPIO_PUPDR_PUPD0_Msk            (0x3UL << GPIO_PUPDR_PUPD0_Pos)
#define GPIO_PUPDR_PUPD0_NONE           (0x0UL << GPIO_PUPDR_PUPD0_Pos)   /*!< No pull-up, pull-down */
#define GPIO_PUPDR_PUPD0_UP             (0x1UL << GPIO_PUPDR_PUPD0_Pos)   /*!< Pull-up */
#define GPIO_PUPDR_PUPD0_DOWN           (0x2UL << GPIO_PUPDR_PUPD0_Pos)   /*!< Pull-down */

/*----------------------------------------------------------------------------*/
/* GPIO IDR/ODR Register Bit Definitions                                      */
/*----------------------------------------------------------------------------*/
#define GPIO_IDR_ID0_Pos                (0U)
#define GPIO_IDR_ID0                    (0x1UL << GPIO_IDR_ID0_Pos)

#define GPIO_ODR_OD0_Pos                (0U)
#define GPIO_ODR_OD0                    (0x1UL << GPIO_ODR_OD0_Pos)

/*----------------------------------------------------------------------------*/
/* GPIO BSRR Register Bit Definitions                                         */
/*----------------------------------------------------------------------------*/
#define GPIO_BSRR_BS0_Pos               (0U)
#define GPIO_BSRR_BS0                   (0x1UL << GPIO_BSRR_BS0_Pos)     /*!< Bit 0 set */
#define GPIO_BSRR_BR0_Pos               (16U)
#define GPIO_BSRR_BR0                   (0x1UL << GPIO_BSRR_BR0_Pos)     /*!< Bit 0 reset */

/*----------------------------------------------------------------------------*/
/* GPIO AFRL/AFRH Register Bit Definitions                                    */
/*----------------------------------------------------------------------------*/
#define GPIO_AFRL_AFSEL0_Pos            (0U)
#define GPIO_AFRL_AFSEL0_Msk            (0xFUL << GPIO_AFRL_AFSEL0_Pos)
#define GPIO_AFRL_AFSEL0_AF0            (0x0UL << GPIO_AFRL_AFSEL0_Pos)  /*!< AF0 selection */
#define GPIO_AFRL_AFSEL0_AF1            (0x1UL << GPIO_AFRL_AFSEL0_Pos)  /*!< AF1 selection */
#define GPIO_AFRL_AFSEL0_AF2            (0x2UL << GPIO_AFRL_AFSEL0_Pos)  /*!< AF2 selection */
#define GPIO_AFRL_AFSEL0_AF3            (0x3UL << GPIO_AFRL_AFSEL0_Pos)  /*!< AF3 selection */
#define GPIO_AFRL_AFSEL0_AF4            (0x4UL << GPIO_AFRL_AFSEL0_Pos)  /*!< AF4 selection */
#define GPIO_AFRL_AFSEL0_AF5            (0x5UL << GPIO_AFRL_AFSEL0_Pos)  /*!< AF5 selection */
#define GPIO_AFRL_AFSEL0_AF6            (0x6UL << GPIO_AFRL_AFSEL0_Pos)  /*!< AF6 selection */
#define GPIO_AFRL_AFSEL0_AF7            (0x7UL << GPIO_AFRL_AFSEL0_Pos)  /*!< AF7 selection */

#define GPIO_AFRH_AFSEL8_Pos            (0U)
#define GPIO_AFRH_AFSEL8_Msk            (0xFUL << GPIO_AFRH_AFSEL8_Pos)

/*============================================================================*/
/*                          RCC REGISTERS                                      */
/*============================================================================*/

/**
 * @brief   RCC Register Structure
 * @details Reset and Clock Control registers manage:
 *          - System clock configuration (HSI, HSE, PLL)
 *          - Peripheral clock enable/disable
 *          - Reset control for peripherals
 *          - Clock source selection and prescalers
 */
typedef struct {
    volatile uint32_t CR;           /*!< Clock control register,                  Addr offset: 0x00 */
    volatile uint32_t PLLCFGR;      /*!< PLL configuration register,              Addr offset: 0x04 */
    volatile uint32_t CFGR;         /*!< Clock configuration register,            Addr offset: 0x08 */
    volatile uint32_t CIR;          /*!< Clock interrupt register,                Addr offset: 0x0C */
    volatile uint32_t AHB1RSTR;     /*!< AHB1 peripheral reset register,          Addr offset: 0x10 */
    volatile uint32_t AHB2RSTR;     /*!< AHB2 peripheral reset register,          Addr offset: 0x14 */
    volatile uint32_t Reserved0[2]; /*!< Reserved,                                Addr offset: 0x18-0x1C */
    volatile uint32_t APB1RSTR;     /*!< APB1 peripheral reset register,          Addr offset: 0x20 */
    volatile uint32_t APB2RSTR;     /*!< APB2 peripheral reset register,          Addr offset: 0x24 */
    volatile uint32_t Reserved1[2]; /*!< Reserved,                                Addr offset: 0x28-0x2C */
    volatile uint32_t AHB1ENR;      /*!< AHB1 peripheral clock enable register,   Addr offset: 0x30 */
    volatile uint32_t AHB2ENR;      /*!< AHB2 peripheral clock enable register,   Addr offset: 0x34 */
    volatile uint32_t Reserved2[2]; /*!< Reserved,                                Addr offset: 0x38-0x3C */
    volatile uint32_t APB1ENR;      /*!< APB1 peripheral clock enable register,   Addr offset: 0x40 */
    volatile uint32_t APB2ENR;      /*!< APB2 peripheral clock enable register,   Addr offset: 0x44 */
    volatile uint32_t Reserved3[2]; /*!< Reserved,                                Addr offset: 0x48-0x4C */
    volatile uint32_t AHB1LPENR;    /*!< AHB1 peripheral clock enable in low mode,Addr offset: 0x50 */
    volatile uint32_t AHB2LPENR;    /*!< AHB2 peripheral clock enable in low mode,Addr offset: 0x54 */
    volatile uint32_t Reserved4[2]; /*!< Reserved,                                Addr offset: 0x58-0x5C */
    volatile uint32_t APB1LPENR;    /*!< APB1 peripheral clock enable in low mode,Addr offset: 0x60 */
    volatile uint32_t APB2LPENR;    /*!< APB2 peripheral clock enable in low mode,Addr offset: 0x64 */
    volatile uint32_t Reserved5[2]; /*!< Reserved,                                Addr offset: 0x68-0x6C */
    volatile uint32_t BDCR;         /*!< Backup domain control register,          Addr offset: 0x70 */
    volatile uint32_t CSR;          /*!< Clock control & status register,         Addr offset: 0x74 */
    volatile uint32_t Reserved6[2]; /*!< Reserved,                                Addr offset: 0x78-0x7C */
    volatile uint32_t SSCGR;        /*!< Spread spectrum clock generation,        Addr offset: 0x80 */
    volatile uint32_t PLLI2SCFGR;   /*!< PLLI2S configuration register,           Addr offset: 0x84 */
} RCC_TypeDef;

/*----------------------------------------------------------------------------*/
/* RCC CR Register Bit Definitions                                            */
/*----------------------------------------------------------------------------*/
#define RCC_CR_HSION_Pos                (0U)
#define RCC_CR_HSION                    (0x1UL << RCC_CR_HSION_Pos)     /*!< Internal High Speed clock enable */
#define RCC_CR_HSIRDY_Pos               (1U)
#define RCC_CR_HSIRDY                   (0x1UL << RCC_CR_HSIRDY_Pos)    /*!< Internal High Speed clock ready flag */
#define RCC_CR_HSEON_Pos                (16U)
#define RCC_CR_HSEON                    (0x1UL << RCC_CR_HSEON_Pos)     /*!< External High Speed clock enable */
#define RCC_CR_HSERDY_Pos               (17U)
#define RCC_CR_HSERDY                   (0x1UL << RCC_CR_HSERDY_Pos)    /*!< External High Speed clock ready flag */
#define RCC_CR_PLLON_Pos                (24U)
#define RCC_CR_PLLON                    (0x1UL << RCC_CR_PLLON_Pos)     /*!< Main PLL enable */
#define RCC_CR_PLLRDY_Pos               (25U)
#define RCC_CR_PLLRDY                   (0x1UL << RCC_CR_PLLRDY_Pos)    /*!< Main PLL ready flag */

/*----------------------------------------------------------------------------*/
/* RCC PLLCFGR Register Bit Definitions                                       */
/*----------------------------------------------------------------------------*/
#define RCC_PLLCFGR_PLLM_Pos            (0U)
#define RCC_PLLCFGR_PLLM_Msk            (0x3FUL << RCC_PLLCFGR_PLLM_Pos) /*!< PLLM[5:0] bits */
#define RCC_PLLCFGR_PLLN_Pos            (6U)
#define RCC_PLLCFGR_PLLN_Msk            (0x1FFUL << RCC_PLLCFGR_PLLN_Pos) /*!< PLLN[8:0] bits */
#define RCC_PLLCFGR_PLLP_Pos            (16U)
#define RCC_PLLCFGR_PLLP_Msk            (0x3UL << RCC_PLLCFGR_PLLP_Pos)  /*!< PLLP[1:0] bits */
#define RCC_PLLCFGR_PLLSRC_Pos          (22U)
#define RCC_PLLCFGR_PLLSRC_HSI          (0x0UL << RCC_PLLCFGR_PLLSRC_Pos) /*!< HSI as PLL source */
#define RCC_PLLCFGR_PLLSRC_HSE          (0x1UL << RCC_PLLCFGR_PLLSRC_Pos) /*!< HSE as PLL source */
#define RCC_PLLCFGR_PLLQ_Pos            (24U)
#define RCC_PLLCFGR_PLLQ_Msk            (0xFUL << RCC_PLLCFGR_PLLQ_Pos)  /*!< PLLQ[3:0] bits */

/*----------------------------------------------------------------------------*/
/* RCC CFGR Register Bit Definitions                                          */
/*----------------------------------------------------------------------------*/
#define RCC_CFGR_SW_Pos                 (0U)
#define RCC_CFGR_SW_Msk                 (0x3UL << RCC_CFGR_SW_Pos)       /*!< SW[1:0] bits */
#define RCC_CFGR_SW_HSI                 (0x0UL << RCC_CFGR_SW_Pos)       /*!< HSI selected as system clock */
#define RCC_CFGR_SW_HSE                 (0x1UL << RCC_CFGR_SW_Pos)       /*!< HSE selected as system clock */
#define RCC_CFGR_SW_PLL                 (0x2UL << RCC_CFGR_SW_Pos)       /*!< PLL selected as system clock */
#define RCC_CFGR_SWS_Pos                (2U)
#define RCC_CFGR_SWS_Msk                (0x3UL << RCC_CFGR_SWS_Pos)      /*!< SWS[1:0] bits (read-only status) */
#define RCC_CFGR_SWS_HSI                (0x0UL << RCC_CFGR_SWS_Pos)      /*!< HSI used as system clock */
#define RCC_CFGR_SWS_HSE                (0x1UL << RCC_CFGR_SWS_Pos)      /*!< HSE used as system clock */
#define RCC_CFGR_SWS_PLL                (0x2UL << RCC_CFGR_SWS_Pos)      /*!< PLL used as system clock */
#define RCC_CFGR_HPRE_Pos               (4U)
#define RCC_CFGR_HPRE_Msk               (0xFUL << RCC_CFGR_HPRE_Pos)     /*!< AHB prescaler */
#define RCC_CFGR_HPRE_DIV1              (0x0UL << RCC_CFGR_HPRE_Pos)     /*!< AHB clock = SYSCLK */
#define RCC_CFGR_PPRE1_Pos              (10U)
#define RCC_CFGR_PPRE1_Msk              (0x7UL << RCC_CFGR_PPRE1_Pos)    /*!< APB1 prescaler */
#define RCC_CFGR_PPRE1_DIV1             (0x0UL << RCC_CFGR_PPRE1_Pos)    /*!< APB1 clock = AHB */
#define RCC_CFGR_PPRE1_DIV2             (0x4UL << RCC_CFGR_PPRE1_Pos)    /*!< APB1 clock = AHB/2 */
#define RCC_CFGR_PPRE1_DIV4             (0x5UL << RCC_CFGR_PPRE1_Pos)    /*!< APB1 clock = AHB/4 */
#define RCC_CFGR_PPRE2_Pos              (13U)
#define RCC_CFGR_PPRE2_Msk              (0x7UL << RCC_CFGR_PPRE2_Pos)    /*!< APB2 prescaler */
#define RCC_CFGR_PPRE2_DIV1             (0x0UL << RCC_CFGR_PPRE2_Pos)    /*!< APB2 clock = AHB */
#define RCC_CFGR_PPRE2_DIV2             (0x4UL << RCC_CFGR_PPRE2_Pos)    /*!< APB2 clock = AHB/2 */

/*----------------------------------------------------------------------------*/
/* RCC AHB1ENR Register Bit Definitions (Peripheral Clock Enable)             */
/*----------------------------------------------------------------------------*/
#define RCC_AHB1ENR_GPIOAEN_Pos         (0U)
#define RCC_AHB1ENR_GPIOAEN             (0x1UL << RCC_AHB1ENR_GPIOAEN_Pos)  /*!< GPIOA clock enable */
#define RCC_AHB1ENR_GPIOBEN_Pos         (1U)
#define RCC_AHB1ENR_GPIOBEN             (0x1UL << RCC_AHB1ENR_GPIOBEN_Pos)  /*!< GPIOB clock enable */
#define RCC_AHB1ENR_GPIOCEN_Pos         (2U)
#define RCC_AHB1ENR_GPIOCEN             (0x1UL << RCC_AHB1ENR_GPIOCEN_Pos)  /*!< GPIOC clock enable */
#define RCC_AHB1ENR_GPIODEN_Pos         (3U)
#define RCC_AHB1ENR_GPIODEN             (0x1UL << RCC_AHB1ENR_GPIODEN_Pos)  /*!< GPIOD clock enable */
#define RCC_AHB1ENR_GPIOEEN_Pos         (4U)
#define RCC_AHB1ENR_GPIOEEN             (0x1UL << RCC_AHB1ENR_GPIOEEN_Pos)  /*!< GPIOE clock enable */
#define RCC_AHB1ENR_GPIOHEN_Pos         (7U)
#define RCC_AHB1ENR_GPIOHEN             (0x1UL << RCC_AHB1ENR_GPIOHEN_Pos)  /*!< GPIOH clock enable */
#define RCC_AHB1ENR_CRCEN_Pos           (12U)
#define RCC_AHB1ENR_CRCEN               (0x1UL << RCC_AHB1ENR_CRCEN_Pos)    /*!< CRC clock enable */
#define RCC_AHB1ENR_DMA1EN_Pos          (21U)
#define RCC_AHB1ENR_DMA1EN              (0x1UL << RCC_AHB1ENR_DMA1EN_Pos)   /*!< DMA1 clock enable */
#define RCC_AHB1ENR_DMA2EN_Pos          (22U)
#define RCC_AHB1ENR_DMA2EN              (0x1UL << RCC_AHB1ENR_DMA2EN_Pos)   /*!< DMA2 clock enable */

/*----------------------------------------------------------------------------*/
/* RCC APB1ENR Register Bit Definitions (Peripheral Clock Enable)             */
/*----------------------------------------------------------------------------*/
#define RCC_APB1ENR_TIM2EN_Pos          (0U)
#define RCC_APB1ENR_TIM2EN              (0x1UL << RCC_APB1ENR_TIM2EN_Pos)   /*!< TIM2 clock enable */
#define RCC_APB1ENR_TIM3EN_Pos          (1U)
#define RCC_APB1ENR_TIM3EN              (0x1UL << RCC_APB1ENR_TIM3EN_Pos)   /*!< TIM3 clock enable */
#define RCC_APB1ENR_TIM4EN_Pos          (2U)
#define RCC_APB1ENR_TIM4EN              (0x1UL << RCC_APB1ENR_TIM4EN_Pos)   /*!< TIM4 clock enable */
#define RCC_APB1ENR_TIM5EN_Pos          (3U)
#define RCC_APB1ENR_TIM5EN              (0x1UL << RCC_APB1ENR_TIM5EN_Pos)   /*!< TIM5 clock enable */
#define RCC_APB1ENR_WWDGEN_Pos          (11U)
#define RCC_APB1ENR_WWDGEN              (0x1UL << RCC_APB1ENR_WWDGEN_Pos)   /*!< WWDG clock enable */
#define RCC_APB1ENR_SPI2EN_Pos          (14U)
#define RCC_APB1ENR_SPI2EN              (0x1UL << RCC_APB1ENR_SPI2EN_Pos)   /*!< SPI2 clock enable */
#define RCC_APB1ENR_SPI3EN_Pos          (15U)
#define RCC_APB1ENR_SPI3EN              (0x1UL << RCC_APB1ENR_SPI3EN_Pos)   /*!< SPI3 clock enable */
#define RCC_APB1ENR_USART2EN_Pos        (17U)
#define RCC_APB1ENR_USART2EN            (0x1UL << RCC_APB1ENR_USART2EN_Pos) /*!< USART2 clock enable */
#define RCC_APB1ENR_I2C1EN_Pos          (21U)
#define RCC_APB1ENR_I2C1EN              (0x1UL << RCC_APB1ENR_I2C1EN_Pos)   /*!< I2C1 clock enable */
#define RCC_APB1ENR_I2C2EN_Pos          (22U)
#define RCC_APB1ENR_I2C2EN              (0x1UL << RCC_APB1ENR_I2C2EN_Pos)   /*!< I2C2 clock enable */
#define RCC_APB1ENR_I2C3EN_Pos          (23U)
#define RCC_APB1ENR_I2C3EN              (0x1UL << RCC_APB1ENR_I2C3EN_Pos)   /*!< I2C3 clock enable */
#define RCC_APB1ENR_PWREN_Pos           (28U)
#define RCC_APB1ENR_PWREN               (0x1UL << RCC_APB1ENR_PWREN_Pos)    /*!< PWR clock enable */

/*----------------------------------------------------------------------------*/
/* RCC APB2ENR Register Bit Definitions (Peripheral Clock Enable)             */
/*----------------------------------------------------------------------------*/
#define RCC_APB2ENR_TIM1EN_Pos          (0U)
#define RCC_APB2ENR_TIM1EN              (0x1UL << RCC_APB2ENR_TIM1EN_Pos)   /*!< TIM1 clock enable */
#define RCC_APB2ENR_USART1EN_Pos        (4U)
#define RCC_APB2ENR_USART1EN            (0x1UL << RCC_APB2ENR_USART1EN_Pos) /*!< USART1 clock enable */
#define RCC_APB2ENR_USART6EN_Pos        (5U)
#define RCC_APB2ENR_USART6EN            (0x1UL << RCC_APB2ENR_USART6EN_Pos) /*!< USART6 clock enable */
#define RCC_APB2ENR_ADC1EN_Pos          (8U)
#define RCC_APB2ENR_ADC1EN              (0x1UL << RCC_APB2ENR_ADC1EN_Pos)   /*!< ADC1 clock enable */
#define RCC_APB2ENR_SPI1EN_Pos          (12U)
#define RCC_APB2ENR_SPI1EN              (0x1UL << RCC_APB2ENR_SPI1EN_Pos)   /*!< SPI1 clock enable */
#define RCC_APB2ENR_SYSCFGEN_Pos        (14U)
#define RCC_APB2ENR_SYSCFGEN            (0x1UL << RCC_APB2ENR_SYSCFGEN_Pos) /*!< SYSCFG clock enable */
#define RCC_APB2ENR_TIM9EN_Pos          (16U)
#define RCC_APB2ENR_TIM9EN              (0x1UL << RCC_APB2ENR_TIM9EN_Pos)   /*!< TIM9 clock enable */
#define RCC_APB2ENR_TIM10EN_Pos         (17U)
#define RCC_APB2ENR_TIM10EN             (0x1UL << RCC_APB2ENR_TIM10EN_Pos)  /*!< TIM10 clock enable */
#define RCC_APB2ENR_TIM11EN_Pos         (18U)
#define RCC_APB2ENR_TIM11EN             (0x1UL << RCC_APB2ENR_TIM11EN_Pos)  /*!< TIM11 clock enable */

/*============================================================================*/
/*                          USART REGISTERS                                    */
/*============================================================================*/

/**
 * @brief   USART Register Structure
 * @details Universal Synchronous/Asynchronous Receiver Transmitter registers
 *          control serial communication including:
 *          - Baud rate generation
 *          - Data frame format (8/9 bits, parity, stop bits)
 *          - Transmit and receive operations
 *          - Status flags and interrupts
 */
typedef struct {
    volatile uint32_t SR;           /*!< Status register,                        Addr offset: 0x00 */
    volatile uint32_t DR;           /*!< Data register,                          Addr offset: 0x04 */
    volatile uint32_t BRR;          /*!< Baud rate register,                     Addr offset: 0x08 */
    volatile uint32_t CR1;          /*!< Control register 1,                     Addr offset: 0x0C */
    volatile uint32_t CR2;          /*!< Control register 2,                     Addr offset: 0x10 */
    volatile uint32_t CR3;          /*!< Control register 3,                     Addr offset: 0x14 */
    volatile uint32_t GTPR;         /*!< Guard time and prescaler register,      Addr offset: 0x18 */
} USART_TypeDef;

/*----------------------------------------------------------------------------*/
/* USART SR Register Bit Definitions                                          */
/*----------------------------------------------------------------------------*/
#define USART_SR_PE_Pos                 (0U)
#define USART_SR_PE                     (0x1UL << USART_SR_PE_Pos)      /*!< Parity error */
#define USART_SR_FE_Pos                 (1U)
#define USART_SR_FE                     (0x1UL << USART_SR_FE_Pos)      /*!< Framing error */
#define USART_SR_NE_Pos                 (2U)
#define USART_SR_NE                     (0x1UL << USART_SR_NE_Pos)      /*!< Noise error flag */
#define USART_SR_ORE_Pos                (3U)
#define USART_SR_ORE                    (0x1UL << USART_SR_ORE_Pos)     /*!< Overrun error */
#define USART_SR_IDLE_Pos               (4U)
#define USART_SR_IDLE                   (0x1UL << USART_SR_IDLE_Pos)    /*!< IDLE line detected */
#define USART_SR_RXNE_Pos               (5U)
#define USART_SR_RXNE                   (0x1UL << USART_SR_RXNE_Pos)    /*!< Read data register not empty */
#define USART_SR_TC_Pos                 (6U)
#define USART_SR_TC                     (0x1UL << USART_SR_TC_Pos)      /*!< Transmission complete */
#define USART_SR_TXE_Pos                (7U)
#define USART_SR_TXE                    (0x1UL << USART_SR_TXE_Pos)     /*!< Transmit data register empty */

/*----------------------------------------------------------------------------*/
/* USART CR1 Register Bit Definitions                                         */
/*----------------------------------------------------------------------------*/
#define USART_CR1_SBK_Pos               (0U)
#define USART_CR1_SBK                   (0x1UL << USART_CR1_SBK_Pos)    /*!< Send break */
#define USART_CR1_RE_Pos                (2U)
#define USART_CR1_RE                    (0x1UL << USART_CR1_RE_Pos)     /*!< Receiver enable */
#define USART_CR1_TE_Pos                (3U)
#define USART_CR1_TE                    (0x1UL << USART_CR1_TE_Pos)     /*!< Transmitter enable */
#define USART_CR1_IDLEIE_Pos            (4U)
#define USART_CR1_IDLEIE                (0x1UL << USART_CR1_IDLEIE_Pos) /*!< IDLE interrupt enable */
#define USART_CR1_RXNEIE_Pos            (5U)
#define USART_CR1_RXNEIE                (0x1UL << USART_CR1_RXNEIE_Pos) /*!< RXNE interrupt enable */
#define USART_CR1_TCIE_Pos              (6U)
#define USART_CR1_TCIE                  (0x1UL << USART_CR1_TCIE_Pos)   /*!< Transmission complete interrupt enable */
#define USART_CR1_TXEIE_Pos             (7U)
#define USART_CR1_TXEIE                 (0x1UL << USART_CR1_TXEIE_Pos)  /*!< TXE interrupt enable */
#define USART_CR1_PEIE_Pos              (8U)
#define USART_CR1_PEIE                  (0x1UL << USART_CR1_PEIE_Pos)   /*!< PE interrupt enable */
#define USART_CR1_PS_Pos                (9U)
#define USART_CR1_PS                    (0x1UL << USART_CR1_PS_Pos)     /*!< Parity selection */
#define USART_CR1_PCE_Pos               (10U)
#define USART_CR1_PCE                   (0x1UL << USART_CR1_PCE_Pos)    /*!< Parity control enable */
#define USART_CR1_WAKE_Pos              (11U)
#define USART_CR1_WAKE                  (0x1UL << USART_CR1_WAKE_Pos)   /*!< Receiver wakeup method */
#define USART_CR1_M_Pos                 (12U)
#define USART_CR1_M                     (0x1UL << USART_CR1_M_Pos)      /*!< Word length (0=8bit, 1=9bit) */
#define USART_CR1_UE_Pos                (13U)
#define USART_CR1_UE                    (0x1UL << USART_CR1_UE_Pos)     /*!< USART enable */

/*----------------------------------------------------------------------------*/
/* USART CR2 Register Bit Definitions                                         */
/*----------------------------------------------------------------------------*/
#define USART_CR2_STOP_Pos              (12U)
#define USART_CR2_STOP_Msk              (0x3UL << USART_CR2_STOP_Pos)   /*!< STOP bits */
#define USART_CR2_STOP_1                (0x0UL << USART_CR2_STOP_Pos)   /*!< 1 stop bit */
#define USART_CR2_STOP_0_5              (0x1UL << USART_CR2_STOP_Pos)   /*!< 0.5 stop bits */
#define USART_CR2_STOP_2                (0x2UL << USART_CR2_STOP_Pos)   /*!< 2 stop bits */
#define USART_CR2_STOP_1_5              (0x3UL << USART_CR2_STOP_Pos)   /*!< 1.5 stop bits */

/*----------------------------------------------------------------------------*/
/* USART BRR Register - Baud Rate Calculation                                 */
/*----------------------------------------------------------------------------*/
/*
 * Baud rate calculation:
 * BRR = fPCLK / (16 x BaudRate)  for OVER8=0 (default)
 * Example: 84MHz PCLK, 115200 baud:
 * BRR = 84000000 / (16 x 115200) = 45.57 ≈ 45 (0x2D)
 * DIV_Mantissa = 45, DIV_Fraction = 9 (0.57 x 16 ≈ 9)
 * BRR = (45 << 4) | 9 = 0x2D9
 */
#define USART_BRR_DIV_MANTISSA_Pos      (4U)
#define USART_BRR_DIV_MANTISSA_Msk      (0xFFFUL << USART_BRR_DIV_MANTISSA_Pos)
#define USART_BRR_DIV_FRACTION_Pos      (0U)
#define USART_BRR_DIV_FRACTION_Msk      (0xFUL << USART_BRR_DIV_FRACTION_Pos)

/*============================================================================*/
/*                          I2C REGISTERS                                     */
/*============================================================================*/

/**
 * @brief   I2C Register Structure
 * @details Inter-Integrated Circuit registers control I2C bus operations:
 *          - Clock configuration and timing
 *          - Addressing (7-bit or 10-bit)
 *          - Start/stop condition generation
 *          - Data transfer and acknowledgment
 */
typedef struct {
    volatile uint32_t CR1;          /*!< Control register 1,                     Addr offset: 0x00 */
    volatile uint32_t CR2;          /*!< Control register 2,                     Addr offset: 0x04 */
    volatile uint32_t OAR1;         /*!< Own address register 1,                 Addr offset: 0x08 */
    volatile uint32_t OAR2;         /*!< Own address register 2,                 Addr offset: 0x0C */
    volatile uint32_t DR;           /*!< Data register,                          Addr offset: 0x10 */
    volatile uint32_t SR1;          /*!< Status register 1,                      Addr offset: 0x14 */
    volatile uint32_t SR2;          /*!< Status register 2,                      Addr offset: 0x18 */
    volatile uint32_t CCR;          /*!< Clock control register,                 Addr offset: 0x1C */
    volatile uint32_t TRISE;        /*!< TRISE register,                         Addr offset: 0x20 */
} I2C_TypeDef;

/*----------------------------------------------------------------------------*/
/* I2C CR1 Register Bit Definitions                                           */
/*----------------------------------------------------------------------------*/
#define I2C_CR1_PE_Pos                  (0U)
#define I2C_CR1_PE                      (0x1UL << I2C_CR1_PE_Pos)       /*!< Peripheral enable */
#define I2C_CR1_SMBUS_Pos               (1U)
#define I2C_CR1_SMBUS                   (0x1UL << I2C_CR1_SMBUS_Pos)    /*!< SMBus mode */
#define I2C_CR1_SMBTYPE_Pos             (3U)
#define I2C_CR1_SMBTYPE                 (0x1UL << I2C_CR1_SMBTYPE_Pos)  /*!< SMBus type */
#define I2C_CR1_ENARP_Pos               (4U)
#define I2C_CR1_ENARP                   (0x1UL << I2C_CR1_ENARP_Pos)    /*!< ARP enable */
#define I2C_CR1_ENPEC_Pos               (5U)
#define I2C_CR1_ENPEC                   (0x1UL << I2C_CR1_ENPEC_Pos)    /*!< PEC enable */
#define I2C_CR1_ENGC_Pos                (6U)
#define I2C_CR1_ENGC                    (0x1UL << I2C_CR1_ENGC_Pos)     /*!< General call enable */
#define I2C_CR1_NOSTRETCH_Pos           (7U)
#define I2C_CR1_NOSTRETCH               (0x1UL << I2C_CR1_NOSTRETCH_Pos) /*!< Clock stretching disable */
#define I2C_CR1_START_Pos               (8U)
#define I2C_CR1_START                   (0x1UL << I2C_CR1_START_Pos)    /*!< Start generation */
#define I2C_CR1_STOP_Pos                (9U)
#define I2C_CR1_STOP                    (0x1UL << I2C_CR1_STOP_Pos)     /*!< Stop generation */
#define I2C_CR1_ACK_Pos                 (10U)
#define I2C_CR1_ACK                     (0x1UL << I2C_CR1_ACK_Pos)      /*!< Acknowledge enable */
#define I2C_CR1_POS_Pos                 (11U)
#define I2C_CR1_POS                     (0x1UL << I2C_CR1_POS_Pos)      /*!< Acknowledge/PEC Position */
#define I2C_CR1_PEC_Pos                 (12U)
#define I2C_CR1_PEC                     (0x1UL << I2C_CR1_PEC_Pos)      /*!< Packet error checking */
#define I2C_CR1_ALERT_Pos               (13U)
#define I2C_CR1_ALERT                   (0x1UL << I2C_CR1_ALERT_Pos)    /*!< SMBus alert */
#define I2C_CR1_SWRST_Pos               (15U)
#define I2C_CR1_SWRST                   (0x1UL << I2C_CR1_SWRST_Pos)    /*!< Software reset */

/*----------------------------------------------------------------------------*/
/* I2C CR2 Register Bit Definitions                                           */
/*----------------------------------------------------------------------------*/
#define I2C_CR2_FREQ_Pos                (0U)
#define I2C_CR2_FREQ_Msk                (0x3FUL << I2C_CR2_FREQ_Pos)    /*!< FREQ[5:0] bits (peripheral clock) */
#define I2C_CR2_ITERREN_Pos             (8U)
#define I2C_CR2_ITERREN                 (0x1UL << I2C_CR2_ITERREN_Pos)  /*!< Error interrupt enable */
#define I2C_CR2_ITEVTEN_Pos             (9U)
#define I2C_CR2_ITEVTEN                 (0x1UL << I2C_CR2_ITEVTEN_Pos)  /*!< Event interrupt enable */
#define I2C_CR2_ITBUFEN_Pos             (10U)
#define I2C_CR2_ITBUFEN                 (0x1UL << I2C_CR2_ITBUFEN_Pos)  /*!< Buffer interrupt enable */
#define I2C_CR2_DMAEN_Pos               (11U)
#define I2C_CR2_DMAEN                   (0x1UL << I2C_CR2_DMAEN_Pos)    /*!< DMA requests enable */
#define I2C_CR2_LAST_Pos                (12U)
#define I2C_CR2_LAST                    (0x1UL << I2C_CR2_LAST_Pos)     /*!< DMA last transfer */

/*----------------------------------------------------------------------------*/
/* I2C SR1 Register Bit Definitions                                           */
/*----------------------------------------------------------------------------*/
#define I2C_SR1_SB_Pos                  (0U)
#define I2C_SR1_SB                      (0x1UL << I2C_SR1_SB_Pos)       /*!< Start bit (Master mode) */
#define I2C_SR1_ADDR_Pos                (1U)
#define I2C_SR1_ADDR                    (0x1UL << I2C_SR1_ADDR_Pos)     /*!< Address sent (Master mode) */
#define I2C_SR1_BTF_Pos                 (2U)
#define I2C_SR1_BTF                     (0x1UL << I2C_SR1_BTF_Pos)      /*!< Byte transfer finished */
#define I2C_SR1_ADD10_Pos               (3U)
#define I2C_SR1_ADD10                   (0x1UL << I2C_SR1_ADD10_Pos)    /*!< 10-bit header sent */
#define I2C_SR1_STOPF_Pos               (4U)
#define I2C_SR1_STOPF                   (0x1UL << I2C_SR1_STOPF_Pos)    /*!< Stop detection (Slave mode) */
#define I2C_SR1_RXNE_Pos                (6U)
#define I2C_SR1_RXNE                    (0x1UL << I2C_SR1_RXNE_Pos)     /*!< Data register not empty */
#define I2C_SR1_TXE_Pos                 (7U)
#define I2C_SR1_TXE                     (0x1UL << I2C_SR1_TXE_Pos)      /*!< Data register empty */
#define I2C_SR1_BERR_Pos                (8U)
#define I2C_SR1_BERR                    (0x1UL << I2C_SR1_BERR_Pos)     /*!< Bus error */
#define I2C_SR1_ARLO_Pos                (9U)
#define I2C_SR1_ARLO                    (0x1UL << I2C_SR1_ARLO_Pos)     /*!< Arbitration lost */
#define I2C_SR1_AF_Pos                  (10U)
#define I2C_SR1_AF                      (0x1UL << I2C_SR1_AF_Pos)       /*!< Acknowledge failure */
#define I2C_SR1_OVR_Pos                 (11U)
#define I2C_SR1_OVR                     (0x1UL << I2C_SR1_OVR_Pos)      /*!< Overrun/Underrun */
#define I2C_SR1_TIMEOUT_Pos             (14U)
#define I2C_SR1_TIMEOUT                 (0x1UL << I2C_SR1_TIMEOUT_Pos)  /*!< Timeout or Tlow error */

/*----------------------------------------------------------------------------*/
/* I2C SR2 Register Bit Definitions                                           */
/*----------------------------------------------------------------------------*/
#define I2C_SR2_MSL_Pos                 (0U)
#define I2C_SR2_MSL                     (0x1UL << I2C_SR2_MSL_Pos)      /*!< Master/Slave */
#define I2C_SR2_BUSY_Pos                (1U)
#define I2C_SR2_BUSY                    (0x1UL << I2C_SR2_BUSY_Pos)     /*!< Bus busy */
#define I2C_SR2_TRA_Pos                 (2U)
#define I2C_SR2_TRA                     (0x1UL << I2C_SR2_TRA_Pos)      /*!< Transmitter/receiver */
#define I2C_SR2_GENCALL_Pos             (4U)
#define I2C_SR2_GENCALL                 (0x1UL << I2C_SR2_GENCALL_Pos)  /*!< General call (Slave mode) */
#define I2C_SR2_DUALF_Pos               (7U)
#define I2C_SR2_DUALF                   (0x1UL << I2C_SR2_DUALF_Pos)    /*!< Dual flag (Slave mode) */

/*----------------------------------------------------------------------------*/
/* I2C CCR Register Bit Definitions                                           */
/*----------------------------------------------------------------------------*/
#define I2C_CCR_CCR_Pos                 (0U)
#define I2C_CCR_CCR_Msk                 (0xFFFUL << I2C_CCR_CCR_Pos)    /*!< Clock control register */
#define I2C_CCR_DUTY_Pos                (14U)
#define I2C_CCR_DUTY                    (0x1UL << I2C_CCR_DUTY_Pos)     /*!< Fast mode duty cycle */
#define I2C_CCR_FS_Pos                  (15U)
#define I2C_CCR_FS                      (0x1UL << I2C_CCR_FS_Pos)       /*!< I2C master mode selection */

/*============================================================================*/
/*                          SPI REGISTERS                                     */
/*============================================================================*/

/**
 * @brief   SPI Register Structure
 * @details Serial Peripheral Interface registers control:
 *          - Clock phase and polarity (CPOL, CPHA)
 *          - Data frame format (8/16 bits)
 *          - Master/slave mode selection
 *          - Baud rate prescaler
 */
typedef struct {
    volatile uint32_t CR1;          /*!< Control register 1,                     Addr offset: 0x00 */
    volatile uint32_t CR2;          /*!< Control register 2,                     Addr offset: 0x04 */
    volatile uint32_t SR;           /*!< Status register,                        Addr offset: 0x08 */
    volatile uint32_t DR;           /*!< Data register,                          Addr offset: 0x0C */
    volatile uint32_t CRCPR;        /*!< CRC polynomial register,                Addr offset: 0x10 */
    volatile uint32_t RXCRCR;       /*!< RX CRC register,                        Addr offset: 0x14 */
    volatile uint32_t TXCRCR;       /*!< TX CRC register,                        Addr offset: 0x18 */
    volatile uint32_t I2SCFGR;      /*!< I2S configuration register,             Addr offset: 0x1C */
    volatile uint32_t I2SPR;        /*!< I2S prescaler register,                 Addr offset: 0x20 */
} SPI_TypeDef;

/*----------------------------------------------------------------------------*/
/* SPI CR1 Register Bit Definitions                                           */
/*----------------------------------------------------------------------------*/
#define SPI_CR1_CPHA_Pos                (0U)
#define SPI_CR1_CPHA                    (0x1UL << SPI_CR1_CPHA_Pos)     /*!< Clock phase */
#define SPI_CR1_CPOL_Pos                (1U)
#define SPI_CR1_CPOL                    (0x1UL << SPI_CR1_CPOL_Pos)     /*!< Clock polarity */
#define SPI_CR1_MSTR_Pos                (2U)
#define SPI_CR1_MSTR                    (0x1UL << SPI_CR1_MSTR_Pos)     /*!< Master selection */
#define SPI_CR1_BR_Pos                  (3U)
#define SPI_CR1_BR_Msk                  (0x7UL << SPI_CR1_BR_Pos)       /*!< BR[2:0] baud rate control */
#define SPI_CR1_BR_DIV2                 (0x0UL << SPI_CR1_BR_Pos)       /*!< fPCLK/2 */
#define SPI_CR1_BR_DIV4                 (0x1UL << SPI_CR1_BR_Pos)       /*!< fPCLK/4 */
#define SPI_CR1_BR_DIV8                 (0x2UL << SPI_CR1_BR_Pos)       /*!< fPCLK/8 */
#define SPI_CR1_BR_DIV16                (0x3UL << SPI_CR1_BR_Pos)       /*!< fPCLK/16 */
#define SPI_CR1_BR_DIV32                (0x4UL << SPI_CR1_BR_Pos)       /*!< fPCLK/32 */
#define SPI_CR1_BR_DIV64                (0x5UL << SPI_CR1_BR_Pos)       /*!< fPCLK/64 */
#define SPI_CR1_BR_DIV128               (0x6UL << SPI_CR1_BR_Pos)       /*!< fPCLK/128 */
#define SPI_CR1_BR_DIV256               (0x7UL << SPI_CR1_BR_Pos)       /*!< fPCLK/256 */
#define SPI_CR1_SPE_Pos                 (6U)
#define SPI_CR1_SPE                     (0x1UL << SPI_CR1_SPE_Pos)      /*!< SPI enable */
#define SPI_CR1_LSBFIRST_Pos            (7U)
#define SPI_CR1_LSBFIRST                (0x1UL << SPI_CR1_LSBFIRST_Pos) /*!< Frame format */
#define SPI_CR1_SSI_Pos                 (8U)
#define SPI_CR1_SSI                     (0x1UL << SPI_CR1_SSI_Pos)      /*!< Internal slave select */
#define SPI_CR1_SSM_Pos                 (9U)
#define SPI_CR1_SSM                     (0x1UL << SPI_CR1_SSM_Pos)      /*!< Software slave management */
#define SPI_CR1_RXONLY_Pos              (10U)
#define SPI_CR1_RXONLY                  (0x1UL << SPI_CR1_RXONLY_Pos)   /*!< Receive only */
#define SPI_CR1_DFF_Pos                 (11U)
#define SPI_CR1_DFF                     (0x1UL << SPI_CR1_DFF_Pos)      /*!< Data frame format (0=8bit, 1=16bit) */
#define SPI_CR1_CRCNEXT_Pos             (12U)
#define SPI_CR1_CRCNEXT                 (0x1UL << SPI_CR1_CRCNEXT_Pos)  /*!< CRC transfer next */
#define SPI_CR1_CRCEN_Pos               (13U)
#define SPI_CR1_CRCEN                   (0x1UL << SPI_CR1_CRCEN_Pos)    /*!< CRC enable */
#define SPI_CR1_BIDIOE_Pos              (14U)
#define SPI_CR1_BIDIOE                  (0x1UL << SPI_CR1_BIDIOE_Pos)   /*!< Output enable in bidirectional mode */
#define SPI_CR1_BIDIMODE_Pos            (15U)
#define SPI_CR1_BIDIMODE                (0x1UL << SPI_CR1_BIDIMODE_Pos) /*!< Bidirectional data mode enable */

/*----------------------------------------------------------------------------*/
/* SPI SR Register Bit Definitions                                            */
/*----------------------------------------------------------------------------*/
#define SPI_SR_RXNE_Pos                 (0U)
#define SPI_SR_RXNE                     (0x1UL << SPI_SR_RXNE_Pos)      /*!< Receive buffer not empty */
#define SPI_SR_TXE_Pos                  (1U)
#define SPI_SR_TXE                      (0x1UL << SPI_SR_TXE_Pos)       /*!< Transmit buffer empty */
#define SPI_SR_CHSIDE_Pos               (2U)
#define SPI_SR_CHSIDE                   (0x1UL << SPI_SR_CHSIDE_Pos)    /*!< Channel side */
#define SPI_SR_UDR_Pos                  (3U)
#define SPI_SR_UDR                      (0x1UL << SPI_SR_UDR_Pos)       /*!< Underrun flag */
#define SPI_SR_CRCERR_Pos               (4U)
#define SPI_SR_CRCERR                   (0x1UL << SPI_SR_CRCERR_Pos)    /*!< CRC error flag */
#define SPI_SR_MODF_Pos                 (5U)
#define SPI_SR_MODF                     (0x1UL << SPI_SR_MODF_Pos)      /*!< Mode fault */
#define SPI_SR_OVR_Pos                  (6U)
#define SPI_SR_OVR                      (0x1UL << SPI_SR_OVR_Pos)       /*!< Overrun flag */
#define SPI_SR_BSY_Pos                  (7U)
#define SPI_SR_BSY                      (0x1UL << SPI_SR_BSY_Pos)       /*!< Busy flag */

/*============================================================================*/
/*                          TIMER REGISTERS                                   */
/*============================================================================*/

/**
 * @brief   TIM Register Structure (Advanced/General Purpose)
 * @details Timer registers control:
 *          - Counter operation (up/down counting)
 *          - Prescaler and auto-reload values
 *          - Input capture and output compare
 *          - PWM generation
 */
typedef struct {
    volatile uint32_t CR1;          /*!< Control register 1,                     Addr offset: 0x00 */
    volatile uint32_t CR2;          /*!< Control register 2,                     Addr offset: 0x04 */
    volatile uint32_t SMCR;         /*!< Slave mode control register,            Addr offset: 0x08 */
    volatile uint32_t DIER;         /*!< DMA/Interrupt enable register,          Addr offset: 0x0C */
    volatile uint32_t SR;           /*!< Status register,                        Addr offset: 0x10 */
    volatile uint32_t EGR;          /*!< Event generation register,              Addr offset: 0x14 */
    volatile uint32_t CCMR1;        /*!< Capture/compare mode register 1,        Addr offset: 0x18 */
    volatile uint32_t CCMR2;        /*!< Capture/compare mode register 2,        Addr offset: 0x1C */
    volatile uint32_t CCER;         /*!< Capture/compare enable register,        Addr offset: 0x20 */
    volatile uint32_t CNT;          /*!< Counter register,                       Addr offset: 0x24 */
    volatile uint32_t PSC;          /*!< Prescaler register,                     Addr offset: 0x28 */
    volatile uint32_t ARR;          /*!< Auto-reload register,                   Addr offset: 0x2C */
    volatile uint32_t RCR;          /*!< Repetition counter register,            Addr offset: 0x30 */
    volatile uint32_t CCR1;         /*!< Capture/compare register 1,             Addr offset: 0x34 */
    volatile uint32_t CCR2;         /*!< Capture/compare register 2,             Addr offset: 0x38 */
    volatile uint32_t CCR3;         /*!< Capture/compare register 3,             Addr offset: 0x3C */
    volatile uint32_t CCR4;         /*!< Capture/compare register 4,             Addr offset: 0x40 */
    volatile uint32_t BDTR;         /*!< Break and dead-time register,           Addr offset: 0x44 */
    volatile uint32_t DCR;          /*!< DMA control register,                   Addr offset: 0x48 */
    volatile uint32_t DMAR;         /*!< DMA address for full transfer,          Addr offset: 0x4C */
    volatile uint32_t OR;           /*!< Option register,                        Addr offset: 0x50 */
} TIM_TypeDef;

/*----------------------------------------------------------------------------*/
/* TIM CR1 Register Bit Definitions                                           */
/*----------------------------------------------------------------------------*/
#define TIM_CR1_CEN_Pos                 (0U)
#define TIM_CR1_CEN                     (0x1UL << TIM_CR1_CEN_Pos)      /*!< Counter enable */
#define TIM_CR1_UDIS_Pos                (1U)
#define TIM_CR1_UDIS                    (0x1UL << TIM_CR1_UDIS_Pos)     /*!< Update disable */
#define TIM_CR1_URS_Pos                 (2U)
#define TIM_CR1_URS                     (0x1UL << TIM_CR1_URS_Pos)      /*!< Update request source */
#define TIM_CR1_OPM_Pos                 (3U)
#define TIM_CR1_OPM                     (0x1UL << TIM_CR1_OPM_Pos)      /*!< One pulse mode */
#define TIM_CR1_DIR_Pos                 (4U)
#define TIM_CR1_DIR                     (0x1UL << TIM_CR1_DIR_Pos)      /*!< Direction */
#define TIM_CR1_CMS_Pos                 (5U)
#define TIM_CR1_CMS_Msk                 (0x3UL << TIM_CR1_CMS_Pos)      /*!< CMS[1:0] Center-aligned mode */
#define TIM_CR1_ARPE_Pos                (7U)
#define TIM_CR1_ARPE                    (0x1UL << TIM_CR1_ARPE_Pos)     /*!< Auto-reload preload enable */

/*----------------------------------------------------------------------------*/
/* TIM DIER Register Bit Definitions                                          */
/*----------------------------------------------------------------------------*/
#define TIM_DIER_UIE_Pos                (0U)
#define TIM_DIER_UIE                    (0x1UL << TIM_DIER_UIE_Pos)     /*!< Update interrupt enable */
#define TIM_DIER_CC1IE_Pos              (1U)
#define TIM_DIER_CC1IE                  (0x1UL << TIM_DIER_CC1IE_Pos)   /*!< CC1 interrupt enable */
#define TIM_DIER_CC2IE_Pos              (2U)
#define TIM_DIER_CC2IE                  (0x1UL << TIM_DIER_CC2IE_Pos)   /*!< CC2 interrupt enable */
#define TIM_DIER_CC3IE_Pos              (3U)
#define TIM_DIER_CC3IE                  (0x1UL << TIM_DIER_CC3IE_Pos)   /*!< CC3 interrupt enable */
#define TIM_DIER_CC4IE_Pos              (4U)
#define TIM_DIER_CC4IE                  (0x1UL << TIM_DIER_CC4IE_Pos)   /*!< CC4 interrupt enable */

/*----------------------------------------------------------------------------*/
/* TIM SR Register Bit Definitions                                            */
/*----------------------------------------------------------------------------*/
#define TIM_SR_UIF_Pos                  (0U)
#define TIM_SR_UIF                      (0x1UL << TIM_SR_UIF_Pos)       /*!< Update interrupt flag */
#define TIM_SR_CC1IF_Pos                (1U)
#define TIM_SR_CC1IF                    (0x1UL << TIM_SR_CC1IF_Pos)     /*!< CC1 interrupt flag */
#define TIM_SR_CC2IF_Pos                (2U)
#define TIM_SR_CC2IF                    (0x1UL << TIM_SR_CC2IF_Pos)     /*!< CC2 interrupt flag */
#define TIM_SR_CC3IF_Pos                (3U)
#define TIM_SR_CC3IF                    (0x1UL << TIM_SR_CC3IF_Pos)     /*!< CC3 interrupt flag */
#define TIM_SR_CC4IF_Pos                (4U)
#define TIM_SR_CC4IF                    (0x1UL << TIM_SR_CC4IF_Pos)     /*!< CC4 interrupt flag */

/*----------------------------------------------------------------------------*/
/* TIM CCMR1 Register Bit Definitions (Output Compare Mode)                    */
/*----------------------------------------------------------------------------*/
#define TIM_CCMR1_CC1S_Pos              (0U)
#define TIM_CCMR1_CC1S_Msk              (0x3UL << TIM_CCMR1_CC1S_Pos)   /*!< CC1S[1:0] Capture/Compare selection */
#define TIM_CCMR1_OC1PE_Pos             (3U)
#define TIM_CCMR1_OC1PE                 (0x1UL << TIM_CCMR1_OC1PE_Pos)  /*!< OC1 preload enable */
#define TIM_CCMR1_OC1M_Pos              (4U)
#define TIM_CCMR1_OC1M_Msk              (0x7UL << TIM_CCMR1_OC1M_Pos)   /*!< OC1M[2:0] Output compare mode */
#define TIM_CCMR1_OC1M_FROZEN           (0x0UL << TIM_CCMR1_OC1M_Pos)   /*!< Frozen (no effect on OC1) */
#define TIM_CCMR1_OC1M_ACTIVE           (0x1UL << TIM_CCMR1_OC1M_Pos)   /*!< Set channel to active level on match */
#define TIM_CCMR1_OC1M_INACTIVE         (0x2UL << TIM_CCMR1_OC1M_Pos)   /*!< Set channel to inactive level on match */
#define TIM_CCMR1_OC1M_TOGGLE           (0x3UL << TIM_CCMR1_OC1M_Pos)   /*!< Toggle on match */
#define TIM_CCMR1_OC1M_FORCE_INACTIVE   (0x4UL << TIM_CCMR1_OC1M_Pos)   /*!< Force inactive level */
#define TIM_CCMR1_OC1M_FORCE_ACTIVE     (0x5UL << TIM_CCMR1_OC1M_Pos)   /*!< Force active level */
#define TIM_CCMR1_OC1M_PWM1             (0x6UL << TIM_CCMR1_OC1M_Pos)   /*!< PWM mode 1 */
#define TIM_CCMR1_OC1M_PWM2             (0x7UL << TIM_CCMR1_OC1M_Pos)   /*!< PWM mode 2 */

/*----------------------------------------------------------------------------*/
/* TIM CCER Register Bit Definitions                                          */
/*----------------------------------------------------------------------------*/
#define TIM_CCER_CC1E_Pos               (0U)
#define TIM_CCER_CC1E                   (0x1UL << TIM_CCER_CC1E_Pos)    /*!< CC1 channel enable */
#define TIM_CCER_CC1P_Pos               (1U)
#define TIM_CCER_CC1P                   (0x1UL << TIM_CCER_CC1P_Pos)    /*!< CC1 output polarity */

/*----------------------------------------------------------------------------*/
/* TIM BDTR Register Bit Definitions (Advanced Timers only)                    */
/*----------------------------------------------------------------------------*/
#define TIM_BDTR_MOE_Pos                (15U)
#define TIM_BDTR_MOE                    (0x1UL << TIM_BDTR_MOE_Pos)     /*!< Main output enable */

/*============================================================================*/
/*                          FLASH REGISTERS                                   */
/*============================================================================*/

/**
 * @brief   FLASH Register Structure
 * @details Flash memory interface registers control:
 *          - Flash access latency (wait states)
 *          - Prefetch buffer and instruction cache
 *          - Flash programming and erasing
 *          - Write protection
 */
typedef struct {
    volatile uint32_t ACR;          /*!< Access control register,                Addr offset: 0x00 */
    volatile uint32_t KEYR;         /*!< Key register,                           Addr offset: 0x04 */
    volatile uint32_t OPTKEYR;      /*!< Option key register,                    Addr offset: 0x08 */
    volatile uint32_t SR;           /*!< Status register,                        Addr offset: 0x0C */
    volatile uint32_t CR;           /*!< Control register,                       Addr offset: 0x10 */
    volatile uint32_t OPTCR;        /*!< Option control register,                Addr offset: 0x14 */
} FLASH_TypeDef;

/*----------------------------------------------------------------------------*/
/* FLASH ACR Register Bit Definitions                                         */
/*----------------------------------------------------------------------------*/
#define FLASH_ACR_LATENCY_Pos           (0U)
#define FLASH_ACR_LATENCY_Msk           (0x7UL << FLASH_ACR_LATENCY_Pos)   /*!< LATENCY[2:0] Wait states */
#define FLASH_ACR_LATENCY_0WS           (0x0UL << FLASH_ACR_LATENCY_Pos)   /*!< 0 wait states (0-30 MHz) */
#define FLASH_ACR_LATENCY_1WS           (0x1UL << FLASH_ACR_LATENCY_Pos)   /*!< 1 wait state (30-60 MHz) */
#define FLASH_ACR_LATENCY_2WS           (0x2UL << FLASH_ACR_LATENCY_Pos)   /*!< 2 wait states (60-84 MHz) */
#define FLASH_ACR_PRFTEN_Pos            (8U)
#define FLASH_ACR_PRFTEN                (0x1UL << FLASH_ACR_PRFTEN_Pos)    /*!< Prefetch enable */
#define FLASH_ACR_ICEN_Pos              (9U)
#define FLASH_ACR_ICEN                  (0x1UL << FLASH_ACR_ICEN_Pos)      /*!< Instruction cache enable */
#define FLASH_ACR_DCEN_Pos              (10U)
#define FLASH_ACR_DCEN                  (0x1UL << FLASH_ACR_DCEN_Pos)      /*!< Data cache enable */
#define FLASH_ACR_ICRST_Pos             (11U)
#define FLASH_ACR_ICRST                 (0x1UL << FLASH_ACR_ICRST_Pos)     /*!< Instruction cache reset */
#define FLASH_ACR_DCRST_Pos             (12U)
#define FLASH_ACR_DCRST                 (0x1UL << FLASH_ACR_DCRST_Pos)     /*!< Data cache reset */

/*============================================================================*/
/*                          ADC REGISTERS                                     */
/*============================================================================*/

/**
 * @brief   ADC Register Structure
 * @details Analog-to-Digital Converter registers control:
 *          - Channel selection and sequencing
 *          - Sampling time configuration
 *          - Conversion triggering
 *          - Data alignment and resolution
 */
typedef struct {
    volatile uint32_t SR;           /*!< ADC status register,                    Addr offset: 0x00 */
    volatile uint32_t CR1;          /*!< ADC control register 1,                 Addr offset: 0x04 */
    volatile uint32_t CR2;          /*!< ADC control register 2,                 Addr offset: 0x08 */
    volatile uint32_t SMPR1;        /*!< ADC sample time register 1,             Addr offset: 0x0C */
    volatile uint32_t SMPR2;        /*!< ADC sample time register 2,             Addr offset: 0x10 */
    volatile uint32_t JOFR1;        /*!< ADC injected channel offset 1,          Addr offset: 0x14 */
    volatile uint32_t JOFR2;        /*!< ADC injected channel offset 2,          Addr offset: 0x18 */
    volatile uint32_t JOFR3;        /*!< ADC injected channel offset 3,          Addr offset: 0x1C */
    volatile uint32_t JOFR4;        /*!< ADC injected channel offset 4,          Addr offset: 0x20 */
    volatile uint32_t HTR;          /*!< ADC watchdog higher threshold,          Addr offset: 0x24 */
    volatile uint32_t LTR;          /*!< ADC watchdog lower threshold,           Addr offset: 0x28 */
    volatile uint32_t SQR1;         /*!< ADC regular sequence register 1,        Addr offset: 0x2C */
    volatile uint32_t SQR2;         /*!< ADC regular sequence register 2,        Addr offset: 0x30 */
    volatile uint32_t SQR3;         /*!< ADC regular sequence register 3,        Addr offset: 0x34 */
    volatile uint32_t JSQR;         /*!< ADC injected sequence register,         Addr offset: 0x38 */
    volatile uint32_t JDR1;         /*!< ADC injected data register 1,           Addr offset: 0x3C */
    volatile uint32_t JDR2;         /*!< ADC injected data register 2,           Addr offset: 0x40 */
    volatile uint32_t JDR3;         /*!< ADC injected data register 3,           Addr offset: 0x44 */
    volatile uint32_t JDR4;         /*!< ADC injected data register 4,           Addr offset: 0x48 */
    volatile uint32_t DR;           /*!< ADC regular data register,              Addr offset: 0x4C */
} ADC_TypeDef;

/*----------------------------------------------------------------------------*/
/* ADC CR1 Register Bit Definitions                                           */
/*----------------------------------------------------------------------------*/
#define ADC_CR1_AWDEN_Pos               (23U)
#define ADC_CR1_AWDEN                   (0x1UL << ADC_CR1_AWDEN_Pos)    /*!< Analog watchdog enable */
#define ADC_CR1_EOCIE_Pos               (5U)
#define ADC_CR1_EOCIE                   (0x1UL << ADC_CR1_EOCIE_Pos)    /*!< EOC interrupt enable */

/*----------------------------------------------------------------------------*/
/* ADC CR2 Register Bit Definitions                                           */
/*----------------------------------------------------------------------------*/
#define ADC_CR2_ADON_Pos                (0U)
#define ADC_CR2_ADON                    (0x1UL << ADC_CR2_ADON_Pos)     /*!< A/D converter ON/OFF */
#define ADC_CR2_CONT_Pos                (1U)
#define ADC_CR2_CONT                    (0x1UL << ADC_CR2_CONT_Pos)     /*!< Continuous conversion */
#define ADC_CR2_SWSTART_Pos             (30U)
#define ADC_CR2_SWSTART                 (0x1UL << ADC_CR2_SWSTART_Pos)  /*!< Start conversion of regular channels */

/*----------------------------------------------------------------------------*/
/* ADC SR Register Bit Definitions                                            */
/*----------------------------------------------------------------------------*/
#define ADC_SR_EOC_Pos                  (1U)
#define ADC_SR_EOC                      (0x1UL << ADC_SR_EOC_Pos)       /*!< End of conversion */

/*============================================================================*/
/*                          NVIC REGISTERS                                    */
/*============================================================================*/

/**
 * @brief   NVIC Register Structure
 * @details Nested Vectored Interrupt Controller registers control:
 *          - Interrupt enable/disable
 *          - Interrupt pending/clear
 *          - Interrupt priority
 */
typedef struct {
    volatile uint32_t ISER[8];      /*!< Interrupt set-enable registers,         Addr offset: 0x00 */
           uint32_t Reserved0[24];
    volatile uint32_t ICER[8];      /*!< Interrupt clear-enable registers,       Addr offset: 0x80 */
           uint32_t Reserved1[24];
    volatile uint32_t ISPR[8];      /*!< Interrupt set-pending registers,        Addr offset: 0x100 */
           uint32_t Reserved2[24];
    volatile uint32_t ICPR[8];      /*!< Interrupt clear-pending registers,      Addr offset: 0x180 */
           uint32_t Reserved3[24];
    volatile uint32_t IABR[8];      /*!< Interrupt active bit registers,         Addr offset: 0x200 */
           uint32_t Reserved4[56];
    volatile uint8_t  IPR[240];     /*!< Interrupt priority registers,           Addr offset: 0x300 */
           uint32_t Reserved5[644];
    volatile uint32_t STIR;         /*!< Software trigger interrupt register,    Addr offset: 0xE00 */
} NVIC_Type;

/*----------------------------------------------------------------------------*/
/* NVIC IRQ Numbers for STM32F401RE                                           */
/*----------------------------------------------------------------------------*/
typedef enum {
    WWDG_IRQn                    =  0,   /*!< Window Watchdog Interrupt */
    PVD_IRQn                     =  1,   /*!< PVD through EXTI Line detection */
    TAMP_STAMP_IRQn              =  2,   /*!< Tamper and TimeStamp interrupts */
    RTC_WKUP_IRQn                =  3,   /*!< RTC Wakeup interrupt */
    FLASH_IRQn                   =  4,   /*!< FLASH global Interrupt */
    RCC_IRQn                     =  5,   /*!< RCC global Interrupt */
    EXTI0_IRQn                   =  6,   /*!< EXTI Line0 Interrupt */
    EXTI1_IRQn                   =  7,   /*!< EXTI Line1 Interrupt */
    EXTI2_IRQn                   =  8,   /*!< EXTI Line2 Interrupt */
    EXTI3_IRQn                   =  9,   /*!< EXTI Line3 Interrupt */
    EXTI4_IRQn                   = 10,   /*!< EXTI Line4 Interrupt */
    DMA1_Stream0_IRQn            = 11,   /*!< DMA1 Stream 0 global Interrupt */
    DMA1_Stream1_IRQn            = 12,   /*!< DMA1 Stream 1 global Interrupt */
    DMA1_Stream2_IRQn            = 13,   /*!< DMA1 Stream 2 global Interrupt */
    DMA1_Stream3_IRQn            = 14,   /*!< DMA1 Stream 3 global Interrupt */
    DMA1_Stream4_IRQn            = 15,   /*!< DMA1 Stream 4 global Interrupt */
    DMA1_Stream5_IRQn            = 16,   /*!< DMA1 Stream 5 global Interrupt */
    DMA1_Stream6_IRQn            = 17,   /*!< DMA1 Stream 6 global Interrupt */
    ADC_IRQn                     = 18,   /*!< ADC1 global Interrupt */
    TIM1_BRK_TIM9_IRQn           = 24,   /*!< TIM1 Break interrupt and TIM9 global interrupt */
    TIM1_UP_TIM10_IRQn           = 25,   /*!< TIM1 Update Interrupt and TIM10 global interrupt */
    TIM1_TRG_COM_TIM11_IRQn      = 26,   /*!< TIM1 Trigger and Commutation Interrupt and TIM11 global interrupt */
    TIM1_CC_IRQn                 = 27,   /*!< TIM1 Capture Compare Interrupt */
    TIM2_IRQn                    = 28,   /*!< TIM2 global Interrupt */
    TIM3_IRQn                    = 29,   /*!< TIM3 global Interrupt */
    TIM4_IRQn                    = 30,   /*!< TIM4 global Interrupt */
    TIM5_IRQn                    = 50,   /*!< TIM5 global Interrupt */
    SPI1_IRQn                    = 35,   /*!< SPI1 global Interrupt */
    SPI2_IRQn                    = 36,   /*!< SPI2 global Interrupt */
    USART1_IRQn                  = 37,   /*!< USART1 global Interrupt */
    USART2_IRQn                  = 38,   /*!< USART2 global Interrupt */
    I2C1_EV_IRQn                 = 31,   /*!< I2C1 Event Interrupt */
    I2C1_ER_IRQn                 = 32,   /*!< I2C1 Error Interrupt */
    I2C2_EV_IRQn                 = 33,   /*!< I2C2 Event Interrupt */
    I2C2_ER_IRQn                 = 34,   /*!< I2C2 Error Interrupt */
    SPI3_IRQn                    = 51,   /*!< SPI3 global Interrupt */
    USART6_IRQn                  = 71,   /*!< USART6 global Interrupt */
    I2C3_EV_IRQn                 = 72,   /*!< I2C3 Event Interrupt */
    I2C3_ER_IRQn                 = 73,   /*!< I2C3 Error Interrupt */
} IRQn_Type;

/*============================================================================*/
/*                          SYSTICK REGISTERS                                 */
/*============================================================================*/

/**
 * @brief   SysTick Register Structure
 * @details System timer registers for simple delay and OS tick generation
 */
typedef struct {
    volatile uint32_t CTRL;         /*!< SysTick control and status register,    Addr offset: 0x00 */
    volatile uint32_t LOAD;         /*!< SysTick reload value register,          Addr offset: 0x04 */
    volatile uint32_t VAL;          /*!< SysTick current value register,         Addr offset: 0x08 */
    volatile uint32_t CALIB;        /*!< SysTick calibration register,           Addr offset: 0x0C */
} SysTick_Type;

#define SysTick_CTRL_ENABLE_Pos         (0U)
#define SysTick_CTRL_ENABLE_Msk         (0x1UL << SysTick_CTRL_ENABLE_Pos)
#define SysTick_CTRL_TICKINT_Pos        (1U)
#define SysTick_CTRL_TICKINT_Msk        (0x1UL << SysTick_CTRL_TICKINT_Pos)
#define SysTick_CTRL_CLKSOURCE_Pos      (2U)
#define SysTick_CTRL_CLKSOURCE_Msk      (0x1UL << SysTick_CTRL_CLKSOURCE_Pos)
#define SysTick_CTRL_COUNTFLAG_Pos      (16U)
#define SysTick_CTRL_COUNTFLAG_Msk      (0x1UL << SysTick_CTRL_COUNTFLAG_Pos)

/*============================================================================*/
/*                          SCB REGISTERS                                     */
/*============================================================================*/

/**
 * @brief   System Control Block Structure
 */
typedef struct {
    volatile uint32_t CPUID;        /*!< CPUID base register,                    Addr offset: 0x00 */
    volatile uint32_t ICSR;         /*!< Interrupt control and state register,   Addr offset: 0x04 */
    volatile uint32_t VTOR;         /*!< Vector table offset register,           Addr offset: 0x08 */
    volatile uint32_t AIRCR;        /*!< Application interrupt/reset control,    Addr offset: 0x0C */
    volatile uint32_t SCR;          /*!< System control register,                Addr offset: 0x10 */
    volatile uint32_t CCR;          /*!< Configuration control register,         Addr offset: 0x14 */
    volatile uint8_t  SHPR[12U];    /*!< System handlers priority registers,     Addr offset: 0x18 */
    volatile uint32_t SHCSR;        /*!< System handler control and state,       Addr offset: 0x24 */
    volatile uint32_t CFSR;         /*!< Configurable fault status register,     Addr offset: 0x28 */
    volatile uint32_t HFSR;         /*!< Hard fault status register,             Addr offset: 0x2C */
    volatile uint32_t DFSR;         /*!< Debug fault status register,            Addr offset: 0x30 */
    volatile uint32_t MMFAR;        /*!< Mem-manage address register,            Addr offset: 0x34 */
    volatile uint32_t BFAR;         /*!< Bus fault address register,             Addr offset: 0x38 */
    volatile uint32_t AFSR;         /*!< Auxiliary fault status register,        Addr offset: 0x3C */
} SCB_Type;

#define SCB_AIRCR_VECTKEY_Pos           (16U)
#define SCB_AIRCR_VECTKEY_Msk           (0xFFFFUL << SCB_AIRCR_VECTKEY_Pos)
#define SCB_AIRCR_VECTKEY               (0x05FAUL << SCB_AIRCR_VECTKEY_Pos)
#define SCB_AIRCR_SYSRESETREQ_Pos       (2U)
#define SCB_AIRCR_SYSRESETREQ_Msk       (0x1UL << SCB_AIRCR_SYSRESETREQ_Pos)

/*============================================================================*/
/*                          PERIPHERAL DECLARATIONS                           */
/*============================================================================*/

#define GPIOA               ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC               ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD               ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE               ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOH               ((GPIO_TypeDef *)GPIOH_BASE)

#define RCC                 ((RCC_TypeDef *)RCC_BASE)
#define FLASH               ((FLASH_TypeDef *)FLASH_R_BASE)

#define TIM1                ((TIM_TypeDef *)TIM1_BASE)
#define TIM2                ((TIM_TypeDef *)TIM2_BASE)
#define TIM3                ((TIM_TypeDef *)TIM3_BASE)
#define TIM4                ((TIM_TypeDef *)TIM4_BASE)
#define TIM5                ((TIM_TypeDef *)TIM5_BASE)
#define TIM9                ((TIM_TypeDef *)TIM9_BASE)
#define TIM10               ((TIM_TypeDef *)TIM10_BASE)
#define TIM11               ((TIM_TypeDef *)TIM11_BASE)

#define USART1              ((USART_TypeDef *)USART1_BASE)
#define USART2              ((USART_TypeDef *)USART2_BASE)
#define USART6              ((USART_TypeDef *)USART6_BASE)

#define I2C1                ((I2C_TypeDef *)I2C1_BASE)
#define I2C2                ((I2C_TypeDef *)I2C2_BASE)
#define I2C3                ((I2C_TypeDef *)I2C3_BASE)

#define SPI1                ((SPI_TypeDef *)SPI1_BASE)
#define SPI2                ((SPI_TypeDef *)SPI2_BASE)
#define SPI3                ((SPI_TypeDef *)SPI3_BASE)

#define ADC1                ((ADC_TypeDef *)ADC1_BASE)

#define NVIC                ((NVIC_Type *)NVIC_BASE)
#define SCB                 ((SCB_Type *)SCB_BASE)
#define SysTick             ((SysTick_Type *)SYSTICK_BASE)

/*============================================================================*/
/*                          UTILITY MACROS                                    */
/*============================================================================*/

/**
 * @brief   Bit manipulation macros for register access
 */
#define SET_BIT(REG, BIT)           ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT)         ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT)          ((REG) & (BIT))
#define CLEAR_REG(REG)              ((REG) = (0x0U))
#define WRITE_REG(REG, VAL)         ((REG) = (VAL))
#define READ_REG(REG)               ((REG))
#define MODIFY_REG(REG, CLEARMASK, SETMASK) \
    WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

/**
 * @brief   Bit position and mask macros
 */
#define BIT(Pos)                    (1UL << (Pos))
#define BIT_MASK(Pos, Msk)          (((uint32_t)(Msk)) << (Pos))

/**
 * @brief   Position and mask extraction macros
 */
#define POS_VAL(Val, Pos)           ((Val) << (Pos))
#define GET_POS(Reg, Pos, Msk)      (((Reg) & (Msk)) >> (Pos))

#ifdef __cplusplus
}
#endif

#endif /* STM32F401XE_H */
