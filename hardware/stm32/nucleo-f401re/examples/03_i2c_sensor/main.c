/**
 * @file    main.c
 * @brief   I2C Sensor Example for STM32F401RE Nucleo
 * @details This example demonstrates:
 *          - I2C bus initialization
 *          - I2C bus scanning for connected devices
 *          - Reading sensor chip ID (BME280)
 *          - Basic I2C start/stop/write/read operations
 * 
 * Hardware Configuration:
 *   - I2C:  I2C1 on Arduino header
 *   - SCL:  PB6 (I2C1_SCL, alternate function AF4)
 *   - SDA:  PB7 (I2C1_SDA, alternate function AF4)
 *   - Speed: 100 kHz (standard mode)
 * 
 * I2C Protocol Basics:
 *   - Two-wire interface: SCL (clock) + SDA (data)
 *   - Open-drain outputs with external pull-up resistors
 *   - Master-slave architecture
 *   - 7-bit addressing (0x00-0x7F)
 * 
 * I2C Transaction Sequence:
 *   1. START condition (SDA goes low while SCL high)
 *   2. Send device address + R/W bit
 *   3. Wait for ACK from slave
 *   4. Send/receive data bytes
 *   5. STOP condition (SDA goes high while SCL high)
 * 
 * BME280 Sensor:
 *   - I2C address: 0x76 (SDO=GND) or 0x77 (SDO=VDD)
 *   - Chip ID register: 0xD0
 *   - Expected ID value: 0x60
 * 
 * Expected Behavior:
 *   - Scans I2C bus addresses 0x03-0x77
 *   - Prints found devices to UART
 *   - If BME280 found, reads and displays chip ID
 * 
 * @author  Embedded C Architecture Course
 * @version 1.0
 */

#include "bsp.h"
#include "../hal/hal_i2c.h"

/*============================================================================*/
/*                          CONFIGURATION                                      */
/*============================================================================*/

#define I2C_SCAN_START      (0x03U)
#define I2C_SCAN_END        (0x77U)

#define BME280_ADDR_LOW     (0x76U)
#define BME280_ADDR_HIGH    (0x77U)
#define BME280_CHIP_ID_REG  (0xD0U)
#define BME280_CHIP_ID      (0x60U)

/*============================================================================*/
/*                          PRIVATE VARIABLES                                  */
/*============================================================================*/

static uint8_t i2c_buffer[16];
static uint8_t found_devices[128];
static uint8_t found_count = 0;

/*============================================================================*/
/*                          PRIVATE FUNCTIONS                                  */
/*============================================================================*/

/**
 * @brief   Convert nibble to hex character
 * @param   nibble: 4-bit value (0-15)
 * @return  Hex character ('0'-'9', 'A'-'F')
 */
static char nibble_to_hex(uint8_t nibble) {
    nibble &= 0x0F;
    if (nibble < 10) {
        return '0' + nibble;
    }
    return 'A' + (nibble - 10);
}

/**
 * @brief   Print byte as two hex characters
 * @param   byte: Byte value to print
 */
static void print_hex_byte(uint8_t byte) {
    char str[3];
    str[0] = nibble_to_hex(byte >> 4);
    str[1] = nibble_to_hex(byte);
    str[2] = '\0';
    BSP_UART_PutString(str);
}

/**
 * @brief   Scan I2C bus for connected devices
 * @details Attempts to address each device and check for ACK.
 *          Found devices are stored in found_devices array.
 */
static void i2c_scan_bus(void) {
    found_count = 0;
    
    BSP_UART_PutString("\r\nScanning I2C bus...\r\n");
    
    for (uint8_t addr = I2C_SCAN_START; addr <= I2C_SCAN_END; addr++) {
        I2C_Status_t status = HAL_I2C_Start();
        if (status != I2C_OK) {
            continue;
        }
        
        status = HAL_I2C_SendAddressWrite(addr);
        HAL_I2C_Stop();
        
        if (status == I2C_OK) {
            found_devices[found_count++] = addr;
            BSP_UART_PutString("  Found device at 0x");
            print_hex_byte(addr);
            BSP_UART_PutString("\r\n");
        }
    }
    
    if (found_count == 0) {
        BSP_UART_PutString("  No I2C devices found.\r\n");
        BSP_UART_PutString("  Check connections and pull-up resistors.\r\n");
    } else {
        char count_str[4] = {0};
        uint8_t temp = found_count;
        int idx = 0;
        while (temp > 0 && idx < 3) {
            count_str[2 - idx] = '0' + (temp % 10);
            temp /= 10;
            idx++;
        }
        BSP_UART_PutString("  Total devices found: ");
        BSP_UART_PutString(count_str + (3 - idx));
        BSP_UART_PutString("\r\n");
    }
}

/**
 * @brief   Check for BME280 sensor and read chip ID
 * @details Looks for BME280 at addresses 0x76 and 0x77.
 *          Reads chip ID register (0xD0) and verifies expected value (0x60).
 */
static void check_bme280(void) {
    uint8_t bme280_addr = 0;
    uint8_t chip_id = 0;
    I2C_Status_t status;
    
    for (uint8_t addr = BME280_ADDR_LOW; addr <= BME280_ADDR_HIGH; addr++) {
        status = HAL_I2C_ReadRegister(addr, BME280_CHIP_ID_REG, &chip_id, 1);
        if (status == I2C_OK) {
            bme280_addr = addr;
            break;
        }
    }
    
    BSP_UART_PutString("\r\nBME280 Sensor Check:\r\n");
    
    if (bme280_addr != 0) {
        BSP_UART_PutString("  BME280 found at address 0x");
        print_hex_byte(bme280_addr);
        BSP_UART_PutString("\r\n");
        
        BSP_UART_PutString("  Chip ID: 0x");
        print_hex_byte(chip_id);
        
        if (chip_id == BME280_CHIP_ID) {
            BSP_UART_PutString(" (valid)\r\n");
        } else {
            BSP_UART_PutString(" (unexpected!)\r\n");
        }
    } else {
        BSP_UART_PutString("  BME280 not found.\r\n");
        BSP_UART_PutString("  Connect BME280 to PB6(SCL) and PB7(SDA).\r\n");
    }
}

/*============================================================================*/
/*                          MAIN FUNCTION                                      */
/*============================================================================*/

int main(void) {
    BSP_Init();
    BSP_UART_Init();
    HAL_I2C_Init();
    
    BSP_UART_PutString("\r\n");
    BSP_UART_PutString("================================\r\n");
    BSP_UART_PutString("  STM32F401RE I2C Scanner\r\n");
    BSP_UART_PutString("================================\r\n");
    BSP_UART_PutString("\r\n");
    BSP_UART_PutString("I2C1 Configuration:\r\n");
    BSP_UART_PutString("  SCL: PB6\r\n");
    BSP_UART_PutString("  SDA: PB7\r\n");
    BSP_UART_PutString("  Speed: 100 kHz\r\n");
    BSP_UART_PutString("\r\n");
    
    while (1) {
        i2c_scan_bus();
        check_bme280();
        
        BSP_UART_PutString("\r\nRescan in 5 seconds...\r\n");
        BSP_Delay(5000);
    }
    
    return 0;
}

/*============================================================================*/
/*                          INTERRUPT HANDLERS                                 */
/*============================================================================*/

void SysTick_Handler(void) {
    BSP_IncrementTick();
}
