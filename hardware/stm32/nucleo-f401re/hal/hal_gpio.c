/**
 * @file hal_gpio.c
 * @brief GPIO Hardware Abstraction Layer Implementation for STM32F401RE
 * 
 * This file implements direct register-level GPIO control for the STM32F401RE.
 * All operations are performed without using ST HAL or LL libraries.
 * 
 * Key Design Decisions:
 *   1. BSRR register used for atomic set/reset operations
 *   2. Read-modify-write for toggle (non-atomic, use with caution in ISRs)
 *   3. Automatic clock enable in Init function
 * 
 * @author Embedded Systems Course
 * @date 2024
 */

#include "hal_gpio.h"

/*============================================================================*/
/*                          PRIVATE HELPER FUNCTIONS                           */
/*============================================================================*/

/**
 * @brief Get the clock enable bit mask for a GPIO port
 * 
 * Maps GPIO port pointer to the corresponding RCC AHB1ENR bit.
 * This allows automatic clock enabling based on port parameter.
 * 
 * @param port Pointer to GPIO port
 * @return Clock enable bit mask, or 0 if invalid port
 */
static uint32_t GetClockEnableBit(GPIO_Regs_t *port)
{
    /*
     * Compare port pointer against known base addresses.
     * Each GPIO port has a unique clock enable bit in RCC->AHB1ENR.
     */
    if (port == GPIOA) {
        return RCC_AHB1ENR_GPIOAEN;
    } else if (port == GPIOB) {
        return RCC_AHB1ENR_GPIOBEN;
    } else if (port == GPIOC) {
        return RCC_AHB1ENR_GPIOCEN;
    } else if (port == GPIOD) {
        return RCC_AHB1ENR_GPIODEN;
    } else if (port == GPIOE) {
        return RCC_AHB1ENR_GPIOEEN;
    } else if (port == GPIOH) {
        return RCC_AHB1ENR_GPIOHEN;
    }
    
    /* Invalid port - return 0 (no bits set) */
    return 0U;
}

/**
 * @brief Get pin number from pin bitmask
 * 
 * Converts a pin bitmask (e.g., GPIO_PIN_5 = 0x0020) to pin number (5).
 * Uses Count Trailing Zeros (CTZ) intrinsic for efficiency.
 * 
 * Note: Only valid for single-pin bitmasks. For multi-pin masks,
 * returns the lowest pin number.
 * 
 * @param pin Pin bitmask
 * @return Pin number (0-15)
 */
static inline uint8_t GetPinNumber(uint16_t pin)
{
    /*
     * Count Trailing Zeros (CTZ) counts the number of zero bits
     * before the first 1 bit, which equals the bit position.
     * 
     * Example: GPIO_PIN_5 = 0b00100000 = 32
     *          CTZ(0x0020) = 5 (five zeros before the 1)
     * 
     * GCC/Clang provide __builtin_ctz() which compiles to
     * a single instruction on ARM Cortex-M (RBIT + CLZ).
     */
    return (uint8_t)__builtin_ctz(pin);
}

/*============================================================================*/
/*                          PUBLIC FUNCTION IMPLEMENTATIONS                    */
/*============================================================================*/

/**
 * @brief Enable GPIO port clock
 * 
 * Enables the AHB1 clock for the specified GPIO port.
 * This MUST be called before accessing any GPIO registers.
 * 
 * Clock Enable Sequence:
 *   1. Set the appropriate bit in RCC->AHB1ENR
 *   2. Wait for clock to stabilize (typically immediate on STM32)
 * 
 * Power Consumption Note:
 *   - Each enabled GPIO port adds ~30-50µA to power consumption
 *   - Disable unused ports in low-power applications
 * 
 * @param port Pointer to GPIO port (GPIOA, GPIOB, etc.)
 */
void HAL_GPIO_ClockEnable(GPIO_Regs_t *port)
{
    uint32_t clock_bit;
    
    /* Get the clock enable bit for this port */
    clock_bit = GetClockEnableBit(port);
    
    if (clock_bit != 0U) {
        /*
         * Set the clock enable bit in AHB1ENR.
         * 
         * AHB1ENR is the AHB1 Peripheral Clock Enable Register.
         * Setting a bit enables the clock to that peripheral.
         * 
         * After enabling, the peripheral registers can be accessed
         * on the next clock cycle (no delay required on STM32F4).
         */
        RCC->AHB1ENR |= clock_bit;
        
        /*
         * Optional: Add a small delay or memory barrier to ensure
         * the clock is stable before accessing GPIO registers.
         * 
         * DSB() ensures all memory accesses complete before continuing.
         * This is generally not needed but can prevent issues with
         * aggressive compiler optimizations.
         */
        __asm__ volatile("dsb" ::: "memory");
    }
}

/**
 * @brief Initialize GPIO pin with full configuration
 * 
 * This function performs complete GPIO pin configuration:
 * 
 * Step 1: Enable Clock
 *   - Each GPIO port must have its clock enabled
 *   - Clock is enabled via RCC->AHB1ENR register
 * 
 * Step 2: Configure Mode (MODER register)
 *   - 2 bits per pin: [2n+1:2n]
 *   - 00 = Input, 01 = Output, 10 = Alternate Function, 11 = Analog
 * 
 * Step 3: Configure Output Type (OTYPER register)
 *   - 1 bit per pin: [n]
 *   - 0 = Push-Pull, 1 = Open-Drain
 * 
 * Step 4: Configure Speed (OSPEEDR register)
 *   - 2 bits per pin: [2n+1:2n]
 *   - Affects slew rate and power consumption
 * 
 * Step 5: Configure Pull Resistors (PUPDR register)
 *   - 2 bits per pin: [2n+1:2n]
 *   - 00 = None, 01 = Pull-Up, 10 = Pull-Down
 * 
 * Step 6: Configure Alternate Function (if needed)
 *   - AFRL for pins 0-7, AFRH for pins 8-15
 *   - 4 bits per pin: [4n+3:4n]
 * 
 * @param port   Pointer to GPIO port
 * @param pin    Pin number bitmask
 * @param mode   Pin mode
 * @param otype  Output type
 * @param speed  Output speed
 * @param pull   Pull resistor configuration
 * @param af     Alternate function selection
 */
void HAL_GPIO_Init(GPIO_Regs_t *port, uint16_t pin, GPIO_Mode_t mode,
                   GPIO_OType_t otype, GPIO_Speed_t speed,
                   GPIO_Pull_t pull, GPIO_AF_t af)
{
    uint8_t pin_num;
    uint32_t temp_reg;
    
    /* Enable GPIO port clock */
    HAL_GPIO_ClockEnable(port);
    
    /* Get the pin number (0-15) from the bitmask */
    pin_num = GetPinNumber(pin);
    
    /*
     * ==================== MODE Configuration ====================
     * 
     * MODER register format:
     *   Bits [31:0] - 2 bits per pin, 16 pins total
     *   Pin n uses bits [2n+1:2n]
     * 
     * Example for pin 5:
     *   Bits used: [11:10]
     *   Position in register: 2 * 5 = 10
     *   Mask to clear: 0x3 << 10 = 0x0C00
     *   Value to set: mode << 10
     * 
     * Operation:
     *   1. Clear the 2 bits for this pin (AND with inverse of mask)
     *   2. Set the new mode value (OR with shifted mode)
     */
    temp_reg = port->MODER;
    temp_reg &= ~(0x03U << (pin_num * 2));      /* Clear 2 bits */
    temp_reg |= ((uint32_t)mode << (pin_num * 2)); /* Set mode */
    port->MODER = temp_reg;
    
    /*
     * ==================== OUTPUT TYPE Configuration ====================
     * 
     * OTYPER register format:
     *   Bits [15:0] - 1 bit per pin
     *   Bit n = output type for pin n
     *   0 = Push-pull, 1 = Open-drain
     * 
     * Example for pin 5:
     *   Bit used: 5
     *   Mask to clear: 0x1 << 5 = 0x0020
     */
    temp_reg = port->OTYPER;
    temp_reg &= ~(0x01U << pin_num);            /* Clear 1 bit */
    temp_reg |= ((uint32_t)otype << pin_num);   /* Set otype */
    port->OTYPER = temp_reg;
    
    /*
     * ==================== SPEED Configuration ====================
     * 
     * OSPEEDR register format:
     *   Bits [31:0] - 2 bits per pin
     *   Same layout as MODER
     */
    temp_reg = port->OSPEEDR;
    temp_reg &= ~(0x03U << (pin_num * 2));      /* Clear 2 bits */
    temp_reg |= ((uint32_t)speed << (pin_num * 2)); /* Set speed */
    port->OSPEEDR = temp_reg;
    
    /*
     * ==================== PULL-UP/PULL-DOWN Configuration ====================
     * 
     * PUPDR register format:
     *   Bits [31:0] - 2 bits per pin
     *   Same layout as MODER
     *   
     * Internal pull resistors are weak (~40kΩ)
     * Not suitable for high-current loads
     */
    temp_reg = port->PUPDR;
    temp_reg &= ~(0x03U << (pin_num * 2));      /* Clear 2 bits */
    temp_reg |= ((uint32_t)pull << (pin_num * 2)); /* Set pull */
    port->PUPDR = temp_reg;
    
    /*
     * ==================== ALTERNATE FUNCTION Configuration ====================
     * 
     * Only configure if mode is ALTERNATE
     * 
     * AF registers format:
     *   AFRL: Pins 0-7, 4 bits per pin [4n+3:4n]
     *   AFRH: Pins 8-15, 4 bits per pin [4(n-8)+3:4(n-8)]
     * 
     * Example for pin 5 (uses AFRL):
     *   Bits used: [23:20] (bits 20-23)
     *   Position: 4 * 5 = 20
     *   
     * Example for pin 12 (uses AFRH):
     *   Bits used: [19:16] (bits 16-19)
     *   Position: 4 * (12 - 8) = 16
     */
    if (mode == GPIO_MODE_ALTERNATE) {
        if (pin_num < 8) {
            /* Pin 0-7: Use AFRL register */
            temp_reg = port->AFRL;
            temp_reg &= ~(0x0FU << (pin_num * 4));      /* Clear 4 bits */
            temp_reg |= ((uint32_t)af << (pin_num * 4)); /* Set AF */
            port->AFRL = temp_reg;
        } else {
            /* Pin 8-15: Use AFRH register */
            temp_reg = port->AFRH;
            temp_reg &= ~(0x0FU << ((pin_num - 8) * 4));      /* Clear 4 bits */
            temp_reg |= ((uint32_t)af << ((pin_num - 8) * 4)); /* Set AF */
            port->AFRH = temp_reg;
        }
    }
}

/**
 * @brief Set GPIO pin high (VDD)
 * 
 * Uses the BSRR (Bit Set/Reset Register) for atomic operation.
 * 
 * BSRR Register Layout:
 *   Bits [15:0]  - Set bits (writing 1 sets corresponding ODR bit)
 *   Bits [31:16] - Reset bits (writing 1 clears corresponding ODR bit)
 * 
 * Why BSRR instead of ODR?
 *   1. Atomic: No read-modify-write sequence needed
 *   2. Thread-safe: Can be used in ISRs without disabling interrupts
 *   3. Fast: Single 32-bit write operation
 * 
 * Example: Set pin 5
 *   BSRR = 0x00000020 (bit 5 set in lower half)
 *   This sets ODR bit 5 to 1
 * 
 * @param port Pointer to GPIO port
 * @param pin  Pin number bitmask
 */
void HAL_GPIO_Set(GPIO_Regs_t *port, uint16_t pin)
{
    /*
     * Write to lower 16 bits of BSRR to set pins.
     * The pin parameter is already a bitmask (e.g., 0x0020 for pin 5).
     * Writing this value sets the corresponding ODR bit.
     */
    port->BSRR = (uint32_t)pin;
}

/**
 * @brief Set GPIO pin low (GND)
 * 
 * Uses the upper 16 bits of BSRR for atomic reset operation.
 * 
 * BSRR Reset Mechanism:
 *   Writing 1 to bit 16+n clears ODR bit n.
 *   This allows set and reset to share one register.
 * 
 * Example: Reset pin 5
 *   BSRR = 0x00200000 (bit 21 = 16 + 5)
 *   This clears ODR bit 5 to 0
 * 
 * Alternative: BRR Register
 *   Some STM32 devices have a separate BRR register for reset.
 *   BSRR bits [31:16] function identically to BRR.
 * 
 * @param port Pointer to GPIO port
 * @param pin  Pin number bitmask
 */
void HAL_GPIO_Reset(GPIO_Regs_t *port, uint16_t pin)
{
    /*
     * Write to upper 16 bits of BSRR to reset pins.
     * Shift pin bitmask left by 16 to access reset bits.
     */
    port->BSRR = ((uint32_t)pin << 16);
}

/**
 * @brief Toggle GPIO pin output
 * 
 * Toggles the pin by XORing the ODR register with the pin bitmask.
 * 
 * WARNING: This operation is NOT atomic!
 *   1. Read ODR
 *   2. XOR with pin
 *   3. Write ODR
 * 
 * In an ISR context, if main code is also toggling pins on the same port,
 * the changes could be lost. For atomic toggle:
 *   1. Read IDR to get current state
 *   2. Use BSRR to set or reset based on current state
 * 
 * @param port Pointer to GPIO port
 * @param pin  Pin number bitmask
 */
void HAL_GPIO_Toggle(GPIO_Regs_t *port, uint16_t pin)
{
    /*
     * XOR the ODR register with the pin mask.
     * This flips only the bits corresponding to the pin(s).
     * 
     * XOR truth table:
     *   A ^ 0 = A  (no change)
     *   A ^ 1 = ~A (toggle)
     */
    port->ODR ^= pin;
}

/**
 * @brief Read GPIO pin input state
 * 
 * Reads the IDR (Input Data Register) which reflects the actual
 * logic level on the pin, regardless of mode.
 * 
 * IDR Register:
 *   Bits [15:0] - Input state for each pin
 *   Read-only register
 *   Updated every AHB1 clock cycle
 * 
 * For output pins:
 *   IDR reflects the actual pin state, not ODR.
 *   This is useful for verifying output or detecting short circuits.
 * 
 * @param port Pointer to GPIO port
 * @param pin  Pin number bitmask
 * @return GPIO_PIN_SET if pin is high, GPIO_PIN_RESET if low
 */
GPIO_PinState_t HAL_GPIO_Read(GPIO_Regs_t *port, uint16_t pin)
{
    /*
     * Check if the corresponding bit in IDR is set.
     * 
     * (IDR & pin) evaluates to:
     *   - 0 if pin is low
     *   - pin value (non-zero) if pin is high
     * 
     * The != 0 comparison converts to boolean, then to enum.
     */
    if ((port->IDR & pin) != 0x00U) {
        return GPIO_PIN_SET;
    } else {
        return GPIO_PIN_RESET;
    }
}

/**
 * @brief Write GPIO pin output state
 * 
 * Convenience function that calls HAL_GPIO_Set or HAL_GPIO_Reset
 * based on the desired state.
 * 
 * @param port  Pointer to GPIO port
 * @param pin   Pin number bitmask
 * @param state Desired pin state
 */
void HAL_GPIO_Write(GPIO_Regs_t *port, uint16_t pin, GPIO_PinState_t state)
{
    if (state == GPIO_PIN_SET) {
        HAL_GPIO_Set(port, pin);
    } else {
        HAL_GPIO_Reset(port, pin);
    }
}
