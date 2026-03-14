/**
 * @file hal_i2c.h
 * @brief I2C Hardware Abstraction Layer for STM32F401RE
 * 
 * This module provides polling-based I2C communication using I2C1.
 * I2C1 is configured on the following pins:
 *   PB6 - I2C1_SCL (clock)
 *   PB7 - I2C1_SDA (data)
 * 
 * I2C Protocol Overview:
 *   - Multi-master, multi-slave bus
 *   - Two-wire interface: SCL (clock) + SDA (data)
 *   - Open-drain outputs with pull-up resistors
 *   - Standard mode: 100 kHz, Fast mode: 400 kHz
 * 
 * Typical Sensor Communication:
 *   1. START condition
 *   2. Send device address + write bit
 *   3. Send register address
 *   4. REPEATED START
 *   5. Send device address + read bit
 *   6. Read data byte(s)
 *   7. STOP condition
 * 
 * Clock Configuration:
 *   - I2C1 is on APB1 bus
 *   - APB1 clock (PCLK1) = 42 MHz
 *   - Clock is used to generate SCL timing
 * 
 * @author Embedded Systems Course
 * @date 2024
 */

#ifndef HAL_I2C_H
#define HAL_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/*                          CONFIGURATION CONSTANTS                           */
/*============================================================================*/

/**
 * I2C Base Addresses
 * From STM32F401 Reference Manual (RM0368)
 */
#define I2C1_BASE           (0x40005400UL)
#define I2C2_BASE           (0x40005800UL)
#define I2C3_BASE           (0x40005C00UL)

/**
 * RCC Base Address
 */
#define RCC_BASE            (0x40023800UL)

/**
 * GPIO Base Addresses
 */
#define GPIOB_BASE          (0x40020400UL)

/*============================================================================*/
/*                          REGISTER DEFINITIONS                               */
/*============================================================================*/

/**
 * I2C Register Structure
 * 
 * Register offsets from Reference Manual:
 *   CR1:   0x00 - Control register 1
 *   CR2:   0x04 - Control register 2
 *   OAR1:  0x08 - Own address register 1
 *   OAR2:  0x0C - Own address register 2
 *   DR:    0x10 - Data register
 *   SR1:   0x14 - Status register 1
 *   SR2:   0x18 - Status register 2
 *   CCR:   0x1C - Clock control register
 *   TRISE: 0x20 - Rise time register
 */
typedef struct {
    volatile uint32_t CR1;      /* Offset 0x00: Control register 1 */
    volatile uint32_t CR2;      /* Offset 0x04: Control register 2 */
    volatile uint32_t OAR1;     /* Offset 0x08: Own address register 1 */
    volatile uint32_t OAR2;     /* Offset 0x0C: Own address register 2 */
    volatile uint32_t DR;       /* Offset 0x10: Data register */
    volatile uint32_t SR1;      /* Offset 0x14: Status register 1 */
    volatile uint32_t SR2;      /* Offset 0x18: Status register 2 */
    volatile uint32_t CCR;      /* Offset 0x1C: Clock control register */
    volatile uint32_t TRISE;    /* Offset 0x20: Rise time register */
} I2C_Regs_t;

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
 * GPIO Register Structure (minimal)
 */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFRL;
    volatile uint32_t AFRH;
} GPIO_Regs_t;

/* Peripheral pointers */
#define I2C1                ((I2C_Regs_t *)I2C1_BASE)
#define I2C2                ((I2C_Regs_t *)I2C2_BASE)
#define I2C3                ((I2C_Regs_t *)I2C3_BASE)
#define RCC                 ((RCC_Regs_t *)RCC_BASE)
#define GPIOB               ((GPIO_Regs_t *)GPIOB_BASE)

/*============================================================================*/
/*                          I2C CR1 REGISTER BITS                              */
/*============================================================================*/

/**
 * Control Register 1 (CR1) Bit Definitions
 */
#define I2C_CR1_PE          (1U << 0)    /* Peripheral enable */
#define I2C_CR1_SMBUS       (1U << 1)    /* SMBus mode */
#define I2C_CR1_SMBTYPE     (1U << 3)    /* SMBus type */
#define I2C_CR1_ENARP       (1U << 4)    /* ARP enable */
#define I2C_CR1_ENPEC       (1U << 5)    /* PEC enable */
#define I2C_CR1_ENGC        (1U << 6)    /* General call enable */
#define I2C_CR1_NOSTRETCH   (1U << 7)    /* Clock stretching disable */
#define I2C_CR1_START       (1U << 8)    /* Start generation */
#define I2C_CR1_STOP        (1U << 9)    /* Stop generation */
#define I2C_CR1_ACK         (1U << 10)   /* Acknowledge enable */
#define I2C_CR1_POS         (1U << 11)   /* Acknowledge/PEC position */
#define I2C_CR1_PEC         (1U << 12)   /* Packet error checking */
#define I2C_CR1_ALERT       (1U << 13)   /* SMBus alert */
#define I2C_CR1_SWRST       (1U << 15)   /* Software reset */

/*
 * Key CR1 Bits Explained:
 * 
 * PE (Peripheral Enable):
 *   - Must be set to use I2C
 *   - Must be cleared then set to reset peripheral
 * 
 * START:
 *   - Set to generate START or REPEATED START
 *   - Cleared by hardware after START sent
 * 
 * STOP:
 *   - Set to generate STOP after current byte
 *   - Cleared by hardware after STOP sent
 * 
 * ACK:
 *   - Set to send ACK after receiving a byte
 *   - Clear to send NACK (for last byte in read)
 * 
 * SWRST:
 *   - Set to reset I2C peripheral
 *   - Must be cleared before using I2C
 */

/*============================================================================*/
/*                          I2C CR2 REGISTER BITS                              */
/*============================================================================*/

/**
 * Control Register 2 (CR2) Bit Definitions
 */
#define I2C_CR2_FREQ_1      (1U << 0)    /* Peripheral clock frequency bit 0 */
#define I2C_CR2_FREQ_2      (1U << 1)    /* Peripheral clock frequency bit 1 */
#define I2C_CR2_FREQ_4      (1U << 2)    /* Peripheral clock frequency bit 2 */
#define I2C_CR2_FREQ_8      (1U << 3)    /* Peripheral clock frequency bit 3 */
#define I2C_CR2_FREQ_16     (1U << 4)    /* Peripheral clock frequency bit 4 */
#define I2C_CR2_FREQ_32     (1U << 5)    /* Peripheral clock frequency bit 5 */
#define I2C_CR2_ITERREN     (1U << 8)    /* Error interrupt enable */
#define I2C_CR2_ITEVTEN     (1U << 9)    /* Event interrupt enable */
#define I2C_CR2_ITBUFEN     (1U << 10)   /* Buffer interrupt enable */
#define I2C_CR2_DMAEN       (1U << 11)   /* DMA requests enable */
#define I2C_CR2_LAST        (1U << 12)   /* DMA last transfer */

/*
 * CR2 FREQ[5:0]:
 *   Must be programmed with APB1 clock frequency in MHz.
 *   For 42 MHz APB1: FREQ = 42 (0x2A)
 *   This value is used for timing calculations.
 */

/*============================================================================*/
/*                          I2C SR1 REGISTER BITS                              */
/*============================================================================*/

/**
 * Status Register 1 (SR1) Bit Definitions
 * 
 * These flags indicate I2C events and must be cleared in specific ways.
 */
#define I2C_SR1_SB          (1U << 0)    /* Start bit (master mode) */
#define I2C_SR1_ADDR        (1U << 1)    /* Address sent/matched */
#define I2C_SR1_BTF         (1U << 2)    /* Byte transfer finished */
#define I2C_SR1_ADD10       (1U << 3)    /* 10-bit header sent */
#define I2C_SR1_STOPF       (1U << 4)    /* Stop detection (slave) */
#define I2C_SR1_RXNE        (1U << 6)    /* Data register not empty (receive) */
#define I2C_SR1_TXE         (1U << 7)    /* Data register empty (transmit) */
#define I2C_SR1_BERR        (1U << 8)    /* Bus error */
#define I2C_SR1_ARLO        (1U << 9)    /* Arbitration lost */
#define I2C_SR1_AF          (1U << 10)   /* Acknowledge failure */
#define I2C_SR1_OVR         (1U << 11)   /* Overrun/underrun */
#define I2C_SR1_PECERR      (1U << 12)   /* PEC error */
#define I2C_SR1_TIMEOUT     (1U << 14)   /* Timeout or Tlow error */
#define I2C_SR1_SMBALERT    (1U << 15)   /* SMBus alert */

/*
 * Key SR1 Flags and How to Clear Them:
 * 
 * SB (Start Bit):
 *   - Set after START generated
 *   - Clear by: Read SR1, then write DR (address)
 * 
 * ADDR (Address Sent):
 *   - Set after address sent and ACK received
 *   - Clear by: Read SR1, then read SR2
 * 
 * BTF (Byte Transfer Finished):
 *   - Set when byte transfer complete
 *   - Clear by: Read SR1, then read/write DR
 * 
 * RXNE (Receive Buffer Not Empty):
 *   - Set when data available in DR
 *   - Clear by: Read DR
 * 
 * TXE (Transmit Buffer Empty):
 *   - Set when DR can accept new data
 *   - Clear by: Write DR
 * 
 * AF (Acknowledge Failure):
 *   - Set when NACK received
 *   - Clear by: Write 0 to AF bit
 */

/*============================================================================*/
/*                          I2C SR2 REGISTER BITS                              */
/*============================================================================*/

/**
 * Status Register 2 (SR2) Bit Definitions
 * 
 * These flags indicate current I2C status (read-only mostly).
 */
#define I2C_SR2_MSL         (1U << 0)    /* Master/slave (1=master) */
#define I2C_SR2_BUSY        (1U << 1)    /* Bus busy */
#define I2C_SR2_TRA         (1U << 2)    /* Transmitter/receiver */
#define I2C_SR2_GENCALL     (1U << 4)    /* General call received */
#define I2C_SR2_SMBDEFAULT  (1U << 5)    /* SMBus default address */
#define I2C_SR2_SMBHOST     (1U << 6)    /* SMBus host header */
#define I2C_SR2_DUALF       (1U << 7)    /* Dual flag */
#define I2C_SR2_PEC         (1U << 8)    /* PEC value */
#define I2C_SR2_OAR3        (1U << 9)    /* OAR3 matched */

/*============================================================================*/
/*                          I2C CCR REGISTER BITS                              */
/*============================================================================*/

/**
 * Clock Control Register (CCR) Bit Definitions
 * 
 * Controls I2C timing (SCL frequency).
 */
#define I2C_CCR_CCR_MASK    (0x0FFFU)    /* CCR value mask (12 bits) */
#define I2C_CCR_DUTY        (1U << 14)   /* Fast mode duty cycle */
#define I2C_CCR_FS          (1U << 15)   /* Fast mode selection (0=std, 1=fast) */

/*
 * CCR Calculation:
 * 
 * Standard Mode (FS=0, 100 kHz max):
 *   Thigh = CCR × TPCLK1
 *   Tlow  = CCR × TPCLK1
 *   Tscl  = 2 × CCR × TPCLK1
 *   CCR = fPCLK1 / (2 × fSCL)
 *   For 100 kHz: CCR = 42MHz / (2 × 100kHz) = 210
 * 
 * Fast Mode (FS=1, 400 kHz max):
 *   DUTY=0: Thigh = CCR × TPCLK1, Tlow = 2 × CCR × TPCLK1
 *   DUTY=1: Thigh = 9 × CCR × TPCLK1, Tlow = 16 × CCR × TPCLK1
 */

/*============================================================================*/
/*                          RCC CLOCK ENABLE BITS                              */
/*============================================================================*/

/**
 * RCC APB1ENR I2C Clock Enable Bits
 */
#define RCC_APB1ENR_I2C1EN     (1U << 21)
#define RCC_APB1ENR_I2C2EN     (1U << 22)
#define RCC_APB1ENR_I2C3EN     (1U << 30)

/**
 * RCC AHB1ENR GPIO Clock Enable
 */
#define RCC_AHB1ENR_GPIOBEN    (1U << 1)

/*============================================================================*/
/*                          CLOCK CONFIGURATION                                */
/*============================================================================*/

/**
 * APB1 clock frequency for I2C timing calculations
 * Assumes 42 MHz APB1 clock (SYSCLK = 84 MHz)
 */
#define I2C_PCLK_FREQUENCY     42000000UL

/**
 * I2C Standard Mode Frequency
 */
#define I2C_STANDARD_FREQ      100000UL    /* 100 kHz */

/**
 * I2C Fast Mode Frequency
 */
#define I2C_FAST_FREQ          400000UL    /* 400 kHz */

/*============================================================================*/
/*                          ERROR CODES                                        */
/*============================================================================*/

/**
 * I2C operation result codes
 */
typedef enum {
    I2C_OK              = 0,    /* Operation successful */
    I2C_ERROR_NACK      = -1,   /* NACK received (no device or wrong address) */
    I2C_ERROR_TIMEOUT   = -2,   /* Operation timed out */
    I2C_ERROR_BUS       = -3,   /* Bus error */
    I2C_ERROR_ARBITRATION = -4  /* Arbitration lost */
} I2C_Status_t;

/*============================================================================*/
/*                          PUBLIC FUNCTION PROTOTYPES                         */
/*============================================================================*/

/**
 * @brief Initialize I2C1 for standard mode (100 kHz) communication
 * 
 * Performs complete I2C1 initialization:
 *   1. Enable GPIOB clock
 *   2. Enable I2C1 clock
 *   3. Configure PB6 (SCL) and PB7 (SDA) as alternate function
 *   4. Configure I2C timing for 100 kHz
 *   5. Enable I2C peripheral
 * 
 * Timing Configuration:
 *   - CR2.FREQ = 42 (APB1 clock in MHz)
 *   - CCR = 210 (for 100 kHz with 42 MHz clock)
 *   - TRISE = 43 (maximum rise time)
 * 
 * Pin Configuration:
 *   - Mode: Alternate function (AF4 for I2C1)
 *   - Output type: Open-drain (required for I2C)
 *   - Speed: High
 *   - Pull: None (external pull-ups required)
 */
void HAL_I2C_Init(void);

/**
 * @brief Generate I2C START condition
 * 
 * Sends a START condition on the I2C bus.
 * Must be called before any transfer.
 * 
 * Sequence:
 *   1. Set START bit in CR1
 *   2. Wait for SB flag in SR1
 * 
 * @return I2C_OK on success, error code on failure
 */
I2C_Status_t HAL_I2C_Start(void);

/**
 * @brief Generate I2C STOP condition
 * 
 * Sends a STOP condition on the I2C bus.
 * Should be called after transfer is complete.
 * 
 * Sequence:
 *   1. Set STOP bit in CR1
 *   2. Wait for STOP to be sent (BUSY flag clears)
 */
void HAL_I2C_Stop(void);

/**
 * @brief Write a single byte to I2C bus
 * 
 * Writes one byte to the data register and waits for transfer.
 * 
 * Sequence:
 *   1. Write data to DR
 *   2. Wait for TXE flag (data register empty)
 *   3. Wait for BTF flag (byte transfer finished)
 * 
 * @param byte Data byte to transmit
 * @return I2C_OK on success, error code on failure
 */
I2C_Status_t HAL_I2C_WriteByte(uint8_t byte);

/**
 * @brief Read a single byte from I2C bus
 * 
 * Reads one byte from the I2C bus.
 * 
 * For last byte in sequence:
 *   - NACK must be sent after reading
 * 
 * Sequence:
 *   1. Wait for RXNE flag
 *   2. Read data from DR
 * 
 * @param ack Whether to send ACK (true) or NACK (false) after byte
 * @return Received data byte
 */
uint8_t HAL_I2C_ReadByte(bool ack);

/**
 * @brief Send device address for write operation
 * 
 * Sends the 7-bit device address followed by write bit (0).
 * 
 * Sequence:
 *   1. Write (address << 1 | 0) to DR
 *   2. Wait for ADDR flag
 *   3. Clear ADDR by reading SR1 then SR2
 * 
 * @param address 7-bit device address (0x00-0x7F)
 * @return I2C_OK on success, I2C_ERROR_NACK if device doesn't respond
 */
I2C_Status_t HAL_I2C_SendAddressWrite(uint8_t address);

/**
 * @brief Send device address for read operation
 * 
 * Sends the 7-bit device address followed by read bit (1).
 * 
 * Sequence:
 *   1. Write (address << 1 | 1) to DR
 *   2. Wait for ADDR flag
 *   3. Clear ADDR by reading SR1 then SR2
 * 
 * @param address 7-bit device address (0x00-0x7F)
 * @return I2C_OK on success, I2C_ERROR_NACK if device doesn't respond
 */
I2C_Status_t HAL_I2C_SendAddressRead(uint8_t address);

/**
 * @brief Write data to a specific device register
 * 
 * Typical sensor write sequence:
 *   1. START
 *   2. Send device address + write
 *   3. Send register address
 *   4. Send data byte(s)
 *   5. STOP
 * 
 * @param dev_addr   7-bit device address
 * @param reg_addr   Register address to write to
 * @param data       Pointer to data buffer
 * @param length     Number of bytes to write
 * @return I2C_OK on success, error code on failure
 */
I2C_Status_t HAL_I2C_WriteRegister(uint8_t dev_addr, uint8_t reg_addr,
                                    const uint8_t *data, uint16_t length);

/**
 * @brief Read data from a specific device register
 * 
 * Typical sensor read sequence:
 *   1. START
 *   2. Send device address + write
 *   3. Send register address
 *   4. REPEATED START
 *   5. Send device address + read
 *   6. Read data byte(s)
 *   7. STOP
 * 
 * @param dev_addr   7-bit device address
 * @param reg_addr   Register address to read from
 * @param data       Pointer to data buffer
 * @param length     Number of bytes to read
 * @return I2C_OK on success, error code on failure
 */
I2C_Status_t HAL_I2C_ReadRegister(uint8_t dev_addr, uint8_t reg_addr,
                                   uint8_t *data, uint16_t length);

/**
 * @brief Check if I2C bus is busy
 * 
 * @return true if bus is busy, false if idle
 */
bool HAL_I2C_IsBusBusy(void);

/**
 * @brief Reset I2C peripheral
 * 
 * Performs a software reset of the I2C peripheral.
 * Used to recover from bus errors or hangs.
 */
void HAL_I2C_Reset(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_I2C_H */
