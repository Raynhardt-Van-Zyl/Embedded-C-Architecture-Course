/**
 * @file hal_i2c.c
 * @brief I2C Hardware Abstraction Layer Implementation for STM32F401RE
 * 
 * This file implements polling-based I2C communication using I2C1.
 * 
 * Key Implementation Details:
 *   - Standard mode (100 kHz)
 *   - 7-bit addressing
 *   - Polling mode (no interrupts)
 *   - Timeout protection on all wait loops
 * 
 * I2C Protocol State Machine:
 *   Master Transmitter: START -> ADDR+W -> REG -> DATA -> STOP
 *   Master Receiver:    START -> ADDR+W -> REG -> RESTART -> ADDR+R -> DATA -> STOP
 * 
 * @author Embedded Systems Course
 * @date 2024
 */

#include "hal_i2c.h"

/*============================================================================*/
/*                          CONFIGURATION CONSTANTS                           */
/*============================================================================*/

/**
 * GPIO Configuration Values for I2C1
 */
#define GPIO_MODE_ALTERNATE     0x02    /* Alternate function mode */
#define GPIO_OTYPE_OPENDRAIN    0x01    /* Open-drain output (required for I2C) */
#define GPIO_SPEED_HIGH         0x02    /* High speed */
#define GPIO_PULL_NONE          0x00    /* No internal pull (external pulls required) */
#define GPIO_AF4_I2C1           0x04    /* AF4 maps to I2C1/I2C2/I2C3 */

/**
 * Timeout value for polling operations
 * Adjust based on application requirements
 */
#define I2C_TIMEOUT_COUNT       100000U

/*============================================================================*/
/*                          PUBLIC FUNCTION IMPLEMENTATIONS                    */
/*============================================================================*/

/**
 * @brief Initialize I2C1 for standard mode (100 kHz) communication
 * 
 * Complete I2C initialization with detailed register explanations:
 * 
 * INITIALIZATION ORDER (from Reference Manual):
 *   1. Enable peripheral clocks (GPIO and I2C)
 *   2. Configure GPIO pins for I2C (open-drain AF)
 *   3. Program CR2 (peripheral clock frequency)
 *   4. Program CCR (clock control for SCL frequency)
 *   5. Program TRISE (maximum rise time)
 *   6. Enable I2C (PE bit)
 */
void HAL_I2C_Init(void)
{
    uint32_t temp;
    
    /*
     * ==================== STEP 1: Enable Clocks ====================
     */
    
    /* Enable GPIOB clock (bit 1 in AHB1ENR) */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    
    /* Enable I2C1 clock (bit 21 in APB1ENR) */
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    
    /*
     * ==================== STEP 2: Configure GPIO Pins ====================
     * 
     * PB6: I2C1_SCL (Alternate Function 4)
     * PB7: I2C1_SDA (Alternate Function 4)
     * 
     * I2C requires open-drain outputs with external pull-up resistors.
     * The Nucleo board typically has 4.7kΩ pull-ups on I2C lines.
     */
    
    /* Configure PB6 (SCL) - Pin 6 */
    /* MODER[13:12] = 10 (alternate function) */
    temp = GPIOB->MODER;
    temp &= ~(0x03U << 12);                 /* Clear bits 13:12 */
    temp |= (GPIO_MODE_ALTERNATE << 12);    /* Set AF mode */
    GPIOB->MODER = temp;
    
    /* OTYPER[6] = 1 (open-drain) - CRITICAL for I2C! */
    GPIOB->OTYPER |= (GPIO_OTYPE_OPENDRAIN << 6);
    
    /* OSPEEDR[13:12] = 10 (high speed) */
    temp = GPIOB->OSPEEDR;
    temp &= ~(0x03U << 12);
    temp |= (GPIO_SPEED_HIGH << 12);
    GPIOB->OSPEEDR = temp;
    
    /* PUPDR[13:12] = 00 (no pull - external pulls used) */
    temp = GPIOB->PUPDR;
    temp &= ~(0x03U << 12);
    GPIOB->PUPDR = temp;
    
    /* AFRL[27:24] = 0100 (AF4) - Pin 6 uses bits [27:24] */
    temp = GPIOB->AFRL;
    temp &= ~(0x0FU << 24);
    temp |= (GPIO_AF4_I2C1 << 24);
    GPIOB->AFRL = temp;
    
    /* Configure PB7 (SDA) - Pin 7 */
    /* MODER[15:14] = 10 (alternate function) */
    temp = GPIOB->MODER;
    temp &= ~(0x03U << 14);
    temp |= (GPIO_MODE_ALTERNATE << 14);
    GPIOB->MODER = temp;
    
    /* OTYPER[7] = 1 (open-drain) - CRITICAL for I2C! */
    GPIOB->OTYPER |= (GPIO_OTYPE_OPENDRAIN << 7);
    
    /* OSPEEDR[15:14] = 10 (high speed) */
    temp = GPIOB->OSPEEDR;
    temp &= ~(0x03U << 14);
    temp |= (GPIO_SPEED_HIGH << 14);
    GPIOB->OSPEEDR = temp;
    
    /* PUPDR[15:14] = 00 (no pull) */
    temp = GPIOB->PUPDR;
    temp &= ~(0x03U << 14);
    GPIOB->PUPDR = temp;
    
    /* AFRL[31:28] = 0100 (AF4) - Pin 7 uses bits [31:28] */
    temp = GPIOB->AFRL;
    temp &= ~(0x0FU << 28);
    temp |= (GPIO_AF4_I2C1 << 28);
    GPIOB->AFRL = temp;
    
    /*
     * ==================== STEP 3: Software Reset (Optional) ====================
     * 
     * Perform a software reset to ensure I2C is in known state.
     * This clears any stuck conditions from previous operations.
     */
    I2C1->CR1 |= I2C_CR1_SWRST;
    for (volatile int i = 0; i < 100; i++);  /* Small delay */
    I2C1->CR1 &= ~I2C_CR1_SWRST;
    
    /*
     * ==================== STEP 4: Configure CR2 ====================
     * 
     * CR2 Configuration:
     *   - FREQ[5:0]: Peripheral clock frequency in MHz
     *   - Must be programmed before CCR and TRISE
     * 
     * For 42 MHz APB1 clock:
     *   FREQ = 42 (0x2A)
     * 
     * This value is used internally for timing calculations.
     */
    I2C1->CR2 = (I2C1->CR2 & ~0x3FU) | 42U;  /* Set FREQ = 42 MHz */
    
    /*
     * ==================== STEP 5: Configure CCR (Clock Control) ====================
     * 
     * CCR determines the SCL clock frequency.
     * 
     * For Standard Mode (FS=0, 100 kHz):
     *   Tscl = 2 × CCR × Tpclk1
     *   CCR = fPCLK1 / (2 × fSCL)
     * 
     * Calculation:
     *   CCR = 42,000,000 / (2 × 100,000)
     *   CCR = 42,000,000 / 200,000
     *   CCR = 210
     * 
     * This gives:
     *   Thigh = 210 × 23.8ns = 5 µs
     *   Tlow = 210 × 23.8ns = 5 µs
     *   Tscl = 10 µs → fSCL = 100 kHz
     */
    I2C1->CCR = 210;  /* CCR value for 100 kHz */
    
    /*
     * ==================== STEP 6: Configure TRISE ====================
     * 
     * TRISE (Maximum Rise Time) Register:
     *   Programs the maximum allowed SCL rise time.
     * 
     * For Standard Mode (100 kHz):
     *   Maximum rise time = 1000 ns
     *   TRISE = (trise / Tpclk1) + 1
     *   TRISE = (1000ns / 23.8ns) + 1 ≈ 43
     * 
     * For Fast Mode (400 kHz):
     *   Maximum rise time = 300 ns
     *   TRISE = (300ns / 23.8ns) + 1 ≈ 13
     */
    I2C1->TRISE = 43;  /* Maximum rise time for standard mode */
    
    /*
     * ==================== STEP 7: Enable I2C ====================
     * 
     * Set PE bit to enable the I2C peripheral.
     */
    I2C1->CR1 |= I2C_CR1_PE;
}

/**
 * @brief Generate I2C START condition
 * 
 * START Condition Sequence:
 *   1. Set START bit in CR1
 *   2. Wait for SB flag in SR1
 *   3. SB is cleared by reading SR1 then writing DR (address)
 * 
 * The START condition signals the beginning of a transfer.
 * In multi-master systems, it also claims the bus.
 * 
 * @return I2C_OK on success, I2C_ERROR_TIMEOUT on failure
 */
I2C_Status_t HAL_I2C_Start(void)
{
    uint32_t timeout = I2C_TIMEOUT_COUNT;
    
    /*
     * Set START bit to generate START condition.
     * After START is sent, SB flag is set.
     */
    I2C1->CR1 |= I2C_CR1_START;
    
    /*
     * Wait for SB flag.
     * SB is set when START condition is generated.
     * 
     * SB is cleared by software sequence:
     *   1. Read SR1
     *   2. Write DR with address
     * 
     * This sequence happens in SendAddress functions.
     */
    while ((I2C1->SR1 & I2C_SR1_SB) == 0U)
    {
        if (--timeout == 0U)
        {
            return I2C_ERROR_TIMEOUT;
        }
    }
    
    return I2C_OK;
}

/**
 * @brief Generate I2C STOP condition
 * 
 * STOP Condition Sequence:
 *   1. Set STOP bit in CR1
 *   2. Wait for STOP to be sent
 * 
 * The STOP bit is cleared by hardware when STOP is detected.
 * Wait for BUSY flag to clear indicates bus is free.
 */
void HAL_I2C_Stop(void)
{
    uint32_t timeout = I2C_TIMEOUT_COUNT;
    
    /*
     * Set STOP bit to generate STOP condition.
     * STOP is generated after current byte transfer.
     */
    I2C1->CR1 |= I2C_CR1_STOP;
    
    /*
     * Wait for STOP to be sent.
     * The STOP bit is cleared by hardware after STOP condition.
     */
    while ((I2C1->CR1 & I2C_CR1_STOP) != 0U)
    {
        if (--timeout == 0U)
        {
            break;  /* Timeout - bus may be stuck */
        }
    }
    
    /*
     * Additional wait for BUSY to clear.
     * BUSY is set when SDA or SCL is low.
     */
    timeout = I2C_TIMEOUT_COUNT;
    while ((I2C1->SR2 & I2C_SR2_BUSY) != 0U)
    {
        if (--timeout == 0U)
        {
            break;
        }
    }
}

/**
 * @brief Write a single byte to I2C bus
 * 
 * Write Sequence:
 *   1. Write data to DR
 *   2. Wait for TXE flag (data moved to shift register)
 *   3. Wait for BTF flag (byte transfer finished)
 * 
 * TXE is set when DR is empty and can accept new data.
 * BTF is set when byte transfer is complete and DR is empty.
 * 
 * @param byte Data byte to transmit
 * @return I2C_OK on success, error code on failure
 */
I2C_Status_t HAL_I2C_WriteByte(uint8_t byte)
{
    uint32_t timeout = I2C_TIMEOUT_COUNT;
    
    /*
     * Write data to DR register.
     * This clears TXE flag and starts transmission.
     */
    I2C1->DR = byte;
    
    /*
     * Wait for TXE flag.
     * TXE is set when DR is empty (data moved to shift register).
     */
    while ((I2C1->SR1 & I2C_SR1_TXE) == 0U)
    {
        /* Check for NACK (device didn't acknowledge) */
        if ((I2C1->SR1 & I2C_SR1_AF) != 0U)
        {
            /* Clear AF flag by writing 0 */
            I2C1->SR1 &= ~I2C_SR1_AF;
            return I2C_ERROR_NACK;
        }
        
        if (--timeout == 0U)
        {
            return I2C_ERROR_TIMEOUT;
        }
    }
    
    /*
     * Wait for BTF (Byte Transfer Finished).
     * BTF is set when:
     *   - Data has been transmitted
     *   - DR is empty
     *   - No new data has been written
     * 
     * This ensures the byte has been fully transmitted.
     */
    timeout = I2C_TIMEOUT_COUNT;
    while ((I2C1->SR1 & I2C_SR1_BTF) == 0U)
    {
        if (--timeout == 0U)
        {
            return I2C_ERROR_TIMEOUT;
        }
    }
    
    return I2C_OK;
}

/**
 * @brief Read a single byte from I2C bus
 * 
 * Read Sequence:
 *   1. Configure ACK/NACK before reading last byte
 *   2. Wait for RXNE flag
 *   3. Read data from DR
 * 
 * ACK Handling:
 *   - ACK=1: Send ACK after each byte (continue reading)
 *   - ACK=0: Send NACK after this byte (end of read)
 * 
 * For proper NACK generation on last byte:
 *   1. Clear ACK before reading second-to-last byte
 *   2. Generate STOP after reading last byte
 * 
 * @param ack Whether to send ACK (true) or NACK (false)
 * @return Received data byte
 */
uint8_t HAL_I2C_ReadByte(bool ack)
{
    uint32_t timeout = I2C_TIMEOUT_COUNT;
    
    /*
     * Configure ACK before reading.
     * 
     * ACK=1: Hardware sends ACK after receiving byte
     * ACK=0: Hardware sends NACK after receiving byte
     * 
     * For multi-byte reads:
     *   - ACK for all bytes except last
     *   - NACK for last byte to signal end of transfer
     */
    if (ack)
    {
        I2C1->CR1 |= I2C_CR1_ACK;   /* Enable ACK */
    }
    else
    {
        I2C1->CR1 &= ~I2C_CR1_ACK;  /* Disable ACK (send NACK) */
    }
    
    /*
     * Wait for RXNE flag.
     * RXNE is set when data is available in DR.
     */
    while ((I2C1->SR1 & I2C_SR1_RXNE) == 0U)
    {
        if (--timeout == 0U)
        {
            return 0xFF;  /* Return dummy value on timeout */
        }
    }
    
    /*
     * Read data from DR.
     * This clears RXNE flag.
     */
    return (uint8_t)(I2C1->DR & 0xFFU);
}

/**
 * @brief Send device address for write operation
 * 
 * Address Write Sequence:
 *   1. Write (address << 1 | 0) to DR
 *   2. Wait for ADDR flag
 *   3. Clear ADDR by reading SR1 then SR2
 * 
 * The address is shifted left by 1, with R/W bit as LSB.
 *   R/W = 0: Write operation
 *   R/W = 1: Read operation
 * 
 * ADDR flag is set when:
 *   - Address has been sent
 *   - ACK has been received from slave
 * 
 * If NACK is received, ADDR is not set and AF flag is set.
 * 
 * @param address 7-bit device address (0x00-0x7F)
 * @return I2C_OK on success, I2C_ERROR_NACK if no ACK
 */
I2C_Status_t HAL_I2C_SendAddressWrite(uint8_t address)
{
    uint32_t timeout = I2C_TIMEOUT_COUNT;
    
    /*
     * Write address with R/W = 0 (write).
     * 
     * Format: [A6 A5 A4 A3 A2 A1 A0 R/W]
     *         [   7-bit address    | 0 ]
     */
    I2C1->DR = (uint8_t)(address << 1);  /* R/W bit = 0 for write */
    
    /*
     * Wait for ADDR flag or NACK.
     * 
     * ADDR is set when address is acknowledged.
     * AF is set when address is not acknowledged (NACK).
     */
    while ((I2C1->SR1 & I2C_SR1_ADDR) == 0U)
    {
        /* Check for acknowledge failure (NACK) */
        if ((I2C1->SR1 & I2C_SR1_AF) != 0U)
        {
            /* Clear AF flag */
            I2C1->SR1 &= ~I2C_SR1_AF;
            return I2C_ERROR_NACK;
        }
        
        if (--timeout == 0U)
        {
            return I2C_ERROR_TIMEOUT;
        }
    }
    
    /*
     * Clear ADDR flag by reading SR1 then SR2.
     * This is a specific hardware sequence.
     * 
     * Reading SR2 also provides status information:
     *   - MSL: Master mode
     *   - BUSY: Bus busy
     *   - TRA: Transmitter mode
     */
    uint32_t temp = I2C1->SR1;    /* Read SR1 */
    temp = I2C1->SR2;             /* Read SR2 to clear ADDR */
    (void)temp;                   /* Suppress unused warning */
    
    return I2C_OK;
}

/**
 * @brief Send device address for read operation
 * 
 * Address Read Sequence:
 *   1. Write (address << 1 | 1) to DR
 *   2. Wait for ADDR flag
 *   3. Clear ADDR by reading SR1 then SR2
 * 
 * For single-byte reads, clear ACK before clearing ADDR.
 * For multi-byte reads, handle ACK based on remaining bytes.
 * 
 * @param address 7-bit device address (0x00-0x7F)
 * @return I2C_OK on success, I2C_ERROR_NACK if no ACK
 */
I2C_Status_t HAL_I2C_SendAddressRead(uint8_t address)
{
    uint32_t timeout = I2C_TIMEOUT_COUNT;
    
    /*
     * Write address with R/W = 1 (read).
     * 
     * Format: [A6 A5 A4 A3 A2 A1 A0 R/W]
     *         [   7-bit address    | 1 ]
     */
    I2C1->DR = (uint8_t)((address << 1) | 0x01U);  /* R/W bit = 1 for read */
    
    /*
     * Wait for ADDR flag or NACK.
     */
    while ((I2C1->SR1 & I2C_SR1_ADDR) == 0U)
    {
        if ((I2C1->SR1 & I2C_SR1_AF) != 0U)
        {
            I2C1->SR1 &= ~I2C_SR1_AF;
            return I2C_ERROR_NACK;
        }
        
        if (--timeout == 0U)
        {
            return I2C_ERROR_TIMEOUT;
        }
    }
    
    /*
     * Clear ADDR flag.
     * Note: For single-byte read, ACK should be cleared before this sequence.
     */
    uint32_t temp = I2C1->SR1;
    temp = I2C1->SR2;
    (void)temp;
    
    return I2C_OK;
}

/**
 * @brief Write data to a specific device register
 * 
 * Complete write transaction:
 *   START -> [ADDR+W] -> [REG] -> [DATA0] -> ... -> [DATAn] -> STOP
 * 
 * Typical use: Writing configuration to sensor registers.
 * 
 * @param dev_addr   7-bit device address
 * @param reg_addr   Register address to write to
 * @param data       Pointer to data buffer
 * @param length     Number of bytes to write
 * @return I2C_OK on success, error code on failure
 */
I2C_Status_t HAL_I2C_WriteRegister(uint8_t dev_addr, uint8_t reg_addr,
                                    const uint8_t *data, uint16_t length)
{
    I2C_Status_t status;
    
    /* Generate START condition */
    status = HAL_I2C_Start();
    if (status != I2C_OK)
    {
        HAL_I2C_Stop();
        return status;
    }
    
    /* Send device address with write bit */
    status = HAL_I2C_SendAddressWrite(dev_addr);
    if (status != I2C_OK)
    {
        HAL_I2C_Stop();
        return status;
    }
    
    /* Send register address */
    status = HAL_I2C_WriteByte(reg_addr);
    if (status != I2C_OK)
    {
        HAL_I2C_Stop();
        return status;
    }
    
    /* Write data bytes */
    for (uint16_t i = 0; i < length; i++)
    {
        status = HAL_I2C_WriteByte(data[i]);
        if (status != I2C_OK)
        {
            HAL_I2C_Stop();
            return status;
        }
    }
    
    /* Generate STOP condition */
    HAL_I2C_Stop();
    
    return I2C_OK;
}

/**
 * @brief Read data from a specific device register
 * 
 * Complete read transaction:
 *   START -> [ADDR+W] -> [REG] -> RESTART -> [ADDR+R] -> [DATA0] -> ... -> [DATAn] -> STOP
 * 
 * The repeated START (RESTART) allows changing direction without releasing bus.
 * 
 * For single-byte read:
 *   - Must clear ACK before clearing ADDR
 *   - Set STOP after ADDR cleared
 * 
 * For multi-byte read (N bytes):
 *   - Keep ACK enabled for bytes 1 to N-1
 *   - Clear ACK before reading byte N-1
 *   - Set STOP after reading byte N
 * 
 * @param dev_addr   7-bit device address
 * @param reg_addr   Register address to read from
 * @param data       Pointer to data buffer
 * @param length     Number of bytes to read
 * @return I2C_OK on success, error code on failure
 */
I2C_Status_t HAL_I2C_ReadRegister(uint8_t dev_addr, uint8_t reg_addr,
                                   uint8_t *data, uint16_t length)
{
    I2C_Status_t status;
    
    /* Handle empty read */
    if (length == 0U)
    {
        return I2C_OK;
    }
    
    /* ===== Phase 1: Write register address ===== */
    
    /* Generate START condition */
    status = HAL_I2C_Start();
    if (status != I2C_OK)
    {
        HAL_I2C_Stop();
        return status;
    }
    
    /* Send device address with write bit */
    status = HAL_I2C_SendAddressWrite(dev_addr);
    if (status != I2C_OK)
    {
        HAL_I2C_Stop();
        return status;
    }
    
    /* Send register address */
    status = HAL_I2C_WriteByte(reg_addr);
    if (status != I2C_OK)
    {
        HAL_I2C_Stop();
        return status;
    }
    
    /* ===== Phase 2: Read data ===== */
    
    /* Generate REPEATED START */
    status = HAL_I2C_Start();
    if (status != I2C_OK)
    {
        HAL_I2C_Stop();
        return status;
    }
    
    /* Send device address with read bit */
    status = HAL_I2C_SendAddressRead(dev_addr);
    if (status != I2C_OK)
    {
        HAL_I2C_Stop();
        return status;
    }
    
    /* Read data bytes */
    if (length == 1U)
    {
        /*
         * Single byte read:
         *   - NACK must be sent after the byte
         *   - STOP must be generated
         * 
         * Special handling for 1-byte read:
         *   ACK should have been cleared before ADDR sequence
         *   We do it here for simplicity
         */
        I2C1->CR1 &= ~I2C_CR1_ACK;  /* Disable ACK (will send NACK) */
        HAL_I2C_Stop();              /* Generate STOP before reading */
        data[0] = HAL_I2C_ReadByte(false);
    }
    else if (length == 2U)
    {
        /*
         * Two-byte read:
         *   - ACK for first byte
         *   - NACK for second byte
         *   - Set STOP after first byte read
         * 
         * Special sequence for 2-byte read:
         *   - Set POS bit (acknowledge position)
         *   - Clear ACK
         *   - Wait for BTF after first byte
         *   - Set STOP
         *   - Read both bytes
         */
        I2C1->CR1 |= I2C_CR1_ACK;   /* Enable ACK for first byte */
        
        /* Read first byte (ACK will be sent) */
        data[0] = HAL_I2C_ReadByte(true);
        
        /* Disable ACK for last byte */
        I2C1->CR1 &= ~I2C_CR1_ACK;
        
        /* Generate STOP */
        HAL_I2C_Stop();
        
        /* Read second byte (NACK was sent) */
        data[1] = HAL_I2C_ReadByte(false);
    }
    else
    {
        /*
         * Multi-byte read (3+ bytes):
         *   - ACK for bytes 1 to N-2
         *   - NACK for last byte
         * 
         * For N-1 byte: Clear ACK, wait for BTF, then STOP
         */
        I2C1->CR1 |= I2C_CR1_ACK;  /* Enable ACK */
        
        /* Read all bytes except last two */
        for (uint16_t i = 0; i < (length - 2); i++)
        {
            data[i] = HAL_I2C_ReadByte(true);  /* Send ACK */
        }
        
        /* Read second-to-last byte, then prepare for last byte */
        data[length - 2] = HAL_I2C_ReadByte(true);
        
        /* Disable ACK for last byte */
        I2C1->CR1 &= ~I2C_CR1_ACK;
        
        /* Generate STOP */
        HAL_I2C_Stop();
        
        /* Read last byte */
        data[length - 1] = HAL_I2C_ReadByte(false);
    }
    
    return I2C_OK;
}

/**
 * @brief Check if I2C bus is busy
 * 
 * BUSY flag indicates SDA or SCL line is low.
 * This can be used to check if bus is available before starting.
 * 
 * @return true if bus is busy, false if idle
 */
bool HAL_I2C_IsBusBusy(void)
{
    return ((I2C1->SR2 & I2C_SR2_BUSY) != 0U);
}

/**
 * @brief Reset I2C peripheral
 * 
 * Performs software reset to recover from bus errors.
 * This clears all internal states and flags.
 * 
 * Use when:
 *   - Bus is stuck (BUSY never clears)
 *   - Repeated timeouts
 *   - After arbitration lost
 */
void HAL_I2C_Reset(void)
{
    /*
     * Software reset sequence:
     *   1. Disable I2C (clear PE)
     *   2. Set SWRST bit
     *   3. Wait briefly
     *   4. Clear SWRST bit
     *   5. Re-initialize
     */
    
    /* Disable peripheral */
    I2C1->CR1 &= ~I2C_CR1_PE;
    
    /* Set software reset */
    I2C1->CR1 |= I2C_CR1_SWRST;
    
    /* Wait for reset to take effect */
    for (volatile int i = 0; i < 1000; i++);
    
    /* Clear software reset */
    I2C1->CR1 &= ~I2C_CR1_SWRST;
    
    /* Re-initialize I2C */
    HAL_I2C_Init();
}
