# STM32F401RE Nucleo-64 Hardware Example

This directory contains register-level bare-metal examples for the STM32F401RE microcontroller on the Nucleo-64 development board.

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
├── bsp/
│   ├── stm32f401xe.h       # Register definitions
│   ├── bsp.h               # BSP interface
│   └── bsp.c               # BSP implementation
├── hal/
│   ├── hal_gpio.h/c        # GPIO driver
│   ├── hal_uart.h/c        # UART driver
│   ├── hal_i2c.h/c         # I2C driver
│   └── hal_timer.h/c       # Timer driver
├── examples/
│   ├── 01_blink/           # LED blink
│   ├── 02_uart_printf/     # UART output
│   ├── 03_i2c_sensor/      # I2C bus scan
│   └── 04_timer_interrupt/ # Timer interrupt
└── CMakeLists.txt          # Build configuration
```

> Note: startup code, CMSIS packs, and a board-specific linker script are not checked into this directory. The CMake file can reuse the repository-level linker script at `code/infrastructure/linker_script.ld` when present.

## Building the Examples

### Prerequisites

- ARM GCC Toolchain: `arm-none-eabi-gcc`
- CMake 3.20+
- OpenOCD (for flashing)

### Build Commands

```bash
# From repository root
cd hardware/stm32/nucleo-f401re
mkdir -p build && cd build
cmake ..
cmake --build . --target all_examples
```

### Flashing with OpenOCD

```bash
# Example: flash blink
openocd -f board/st_nucleo_f4.cfg -c "program nucleo-f401re-01_blink verify reset exit"
```

## Example Descriptions

### 01_blink
- GPIO output configuration
- Delay-based LED blinking

### 02_uart_printf
- USART configuration (115200 baud)
- Character output and simple echo

### 03_i2c_sensor
- I2C master configuration
- Bus scanning

### 04_timer_interrupt
- Periodic timer setup
- ISR-driven status updates

## Register-Level Programming Notes

This example intentionally uses direct register access instead of vendor HAL libraries to teach hardware fundamentals.

## References

- [STM32F401 Reference Manual (RM0368)](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbxc-and-stm32f401xdxe-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STM32F401 Datasheet](https://www.st.com/resource/en/datasheet/stm32f401re.pdf)
- [Nucleo-64 Board Manual (UM1724)](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf)
