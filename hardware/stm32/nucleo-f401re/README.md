# STM32F401RE Nucleo-64 Hardware Example

This directory contains a complete, register-level bare-metal example for the STM32F401RE microcontroller on the Nucleo-64 development board.

## Hardware Specifications

| Feature | Value |
|---------|-------|
| MCU | STM32F401RET6 |
| Core | ARM Cortex-M4F |
| Max Frequency | 84 MHz |
| Flash | 512 KB |
| SRAM | 96 KB |
| Package | LQFP64 |

## Board Features

- **LED**: LD2 (green) connected to PA5
- **Button**: B1 (blue) connected to PC13 (active-low)
- **ST-LINK V2**: On-board debugger/programmer
- **Virtual COM Port**: USART2 (PA2/PA3) to ST-LINK

## Directory Structure

```
nucleo-f401re/
├── bsp/                    # Board Support Package
│   ├── stm32f401xe.h       # Register definitions
│   ├── bsp.h               # BSP interface
│   └── bsp.c               # BSP implementation
├── hal/                    # Hardware Abstraction Layer
│   ├── hal_gpio.h/c        # GPIO driver
│   ├── hal_uart.h/c        # UART driver
│   ├── hal_i2c.h/c         # I2C driver
│   └── hal_timer.h/c       # Timer driver
├── startup/                # Startup code
│   ├── startup_stm32f401xe.s    # Vector table & init
│   └── system_stm32f401xe.c     # Clock configuration
├── linker/                 # Linker scripts
│   └── STM32F401RETx_FLASH.ld
├── examples/               # Example applications
│   ├── 01_blink/           # LED blink
│   ├── 02_uart_printf/     # UART output
│   ├── 03_i2c_sensor/      # I2C bus scan
│   └── 04_timer_interrupt/ # Timer interrupt
└── CMakeLists.txt          # Build configuration
```

## Clock Configuration

```
HSE (8 MHz from ST-LINK)
    ↓
PLL: M=8, N=336, P=4, Q=7
    ↓
SYSCLK: 84 MHz
    ↓
├── AHB:  84 MHz (HCLK)
├── APB1: 42 MHz (PCLK1)
└── APB2: 84 MHz (PCLK2)
```

## Peripheral Pin Assignments

| Peripheral | Pin | Function |
|------------|-----|----------|
| **LED** | PA5 | GPIO Output (LD2) |
| **Button** | PC13 | GPIO Input (B1) |
| **USART2_TX** | PA2 | Alternate Function 7 |
| **USART2_RX** | PA3 | Alternate Function 7 |
| **I2C1_SCL** | PB6 | Alternate Function 4 |
| **I2C1_SDA** | PB7 | Alternate Function 4 |
| **SPI1_SCK** | PA5 | Alternate Function 5* |
| **SPI1_MISO** | PA6 | Alternate Function 5 |
| **SPI1_MOSI** | PA7 | Alternate Function 5 |

*Note: PA5 is shared with LED, SPI and LED cannot be used simultaneously.

## Building the Examples

### Prerequisites

- ARM GCC Toolchain: `arm-none-eabi-gcc`
- CMake 3.20+
- OpenOCD (for flashing)

### Build Commands

```bash
# Create build directory
mkdir build && cd build

# Configure for target hardware
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc-toolchain.cmake

# Build all examples
cmake --build .

# Build specific example
cmake --build . --target blink
```

### Flashing with OpenOCD

```bash
# Flash blink example
openocd -f board/st_nucleo_f4.cfg -c "program blink.elf verify reset exit"
```

### Flashing with ST-LINK Utility

```bash
st-flash write blink.bin 0x08000000
```

## Example Descriptions

### 01_blink
Simple LED blink demonstrating:
- GPIO output configuration
- Non-blocking delay using SysTick
- Basic initialization sequence

### 02_uart_printf
UART output demonstrating:
- USART configuration (115200 baud)
- Printf-style output
- Character echo

### 03_i2c_sensor
I2C bus scan demonstrating:
- I2C master configuration
- Bus scanning for devices
- BME280 sensor detection

### 04_timer_interrupt
Timer interrupt demonstrating:
- TIM2 configuration for 1ms tick
- Proper ISR design
- Non-blocking LED control

## Register-Level Programming Notes

This example uses **direct register access** without ST's HAL or LL libraries. This approach:

1. **Teaches fundamentals** - You understand exactly what the hardware is doing
2. **Zero overhead** - No abstraction penalty
3. **Portable knowledge** - Applicable to any ARM Cortex-M MCU
4. **No vendor lock-in** - Not dependent on ST's libraries

### Key Patterns Used

```c
// Enable peripheral clock
RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

// Configure pin mode
GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE5) | (1U << GPIO_MODER_MODE5_Pos);

// Atomic pin set (BSRR register)
GPIOA->BSRR = GPIO_BSRR_BS5;  // Set bit 5

// Atomic pin reset (BSRR register)
GPIOA->BSRR = GPIO_BSRR_BR5;  // Reset bit 5
```

## Debugging

### Using GDB with OpenOCD

```bash
# Terminal 1: Start OpenOCD
openocd -f board/st_nucleo_f4.cfg

# Terminal 2: Start GDB
arm-none-eabi-gdb blink.elf
(gdb) target remote :3333
(gdb) monitor reset halt
(gdb) load
(gdb) continue
```

### Semihosting

To enable printf via debug probe:
```c
// In main.c
extern void initialise_monitor_handles(void);
initialise_monitor_handles();  // Call before any printf
```

## References

- [STM32F401 Reference Manual (RM0368)](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbxc-and-stm32f401xdxe-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STM32F401 Datasheet](https://www.st.com/resource/en/datasheet/stm32f401re.pdf)
- [Nucleo-64 Board Manual (UM1724)](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf)
- [ARM Cortex-M4 Technical Reference Manual](https://developer.arm.com/documentation/ddi0439/latest/)
