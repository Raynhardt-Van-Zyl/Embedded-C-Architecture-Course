cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(NUCLEO_F401RE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
)

set(CPU_DEFINES
    STM32F401RE
    STM32F401xx
    USE_FULL_ASSERT
)

set(COMMON_FLAGS
    ${MCU_FLAGS}
    -Os
    -g3
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    -ffunction-sections
    -fdata-sections
    -fno-builtin
    -fno-common
)

set(CMAKE_C_FLAGS
    "${COMMON_FLAGS} -std=c11 -Wstrict-prototypes"
)

set(CMAKE_CXX_FLAGS
    "${COMMON_FLAGS} -std=c++17 -fno-exceptions -fno-rtti"
)

set(CMAKE_ASM_FLAGS
    "${MCU_FLAGS} -g3"
)

set(CMAKE_EXE_LINKER_FLAGS
    "${MCU_FLAGS} -Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles -nostdlib"
)

set(NUCLEO_INCLUDES
    ${NUCLEO_F401RE_ROOT}/bsp
    ${NUCLEO_F401RE_ROOT}/hal
    ${NUCLEO_F401RE_ROOT}/include/cmsis
    ${NUCLEO_F401RE_ROOT}/startup
)

set(BSP_SOURCES
    ${NUCLEO_F401RE_ROOT}/bsp/bsp.c
)

set(HAL_SOURCES
    ${NUCLEO_F401RE_ROOT}/hal/hal_gpio.c
    ${NUCLEO_F401RE_ROOT}/hal/hal_uart.c
    ${NUCLEO_F401RE_ROOT}/hal/hal_i2c.c
    ${NUCLEO_F401RE_ROOT}/hal/hal_timer.c
)

set(LINKER_SCRIPT
    ${NUCLEO_F401RE_ROOT}/linker/STM32F401RETx_FLASH.ld
)

function(add_nucleo_example EXAMPLE_NAME EXAMPLE_DIR)
    set(EXAMPLE_TARGET nucleo-f401re-${EXAMPLE_NAME})
    
    add_executable(${EXAMPLE_TARGET}
        ${EXAMPLE_DIR}/main.c
        ${BSP_SOURCES}
        ${HAL_SOURCES}
    )
    
    target_include_directories(${EXAMPLE_TARGET} PRIVATE
        ${NUCLEO_INCLUDES}
        ${EXAMPLE_DIR}
    )
    
    target_compile_definitions(${EXAMPLE_TARGET} PRIVATE
        ${CPU_DEFINES}
    )
    
    target_link_options(${EXAMPLE_TARGET} PRIVATE
        -T${LINKER_SCRIPT}
        -Wl,-Map=${EXAMPLE_TARGET}.map
    )
    
    add_custom_command(TARGET ${EXAMPLE_TARGET} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${EXAMPLE_TARGET} ${EXAMPLE_TARGET}.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${EXAMPLE_TARGET} ${EXAMPLE_TARGET}.hex
        COMMAND ${CMAKE_SIZE} ${EXAMPLE_TARGET}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating binary and hex files for ${EXAMPLE_TARGET}"
    )
    
    add_custom_target(${EXAMPLE_TARGET}-flash
        COMMAND openocd -f board/st_nucleo_f4.cfg -c "program ${EXAMPLE_TARGET} verify reset exit"
        DEPENDS ${EXAMPLE_TARGET}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Flashing ${EXAMPLE_TARGET} via ST-Link"
    )
endfunction()

add_nucleo_example(01_blink ${NUCLEO_F401RE_ROOT}/examples/01_blink)
add_nucleo_example(02_uart_printf ${NUCLEO_F401RE_ROOT}/examples/02_uart_printf)
add_nucleo_example(03_i2c_sensor ${NUCLEO_F401RE_ROOT}/examples/03_i2c_sensor)
add_nucleo_example(04_timer_interrupt ${NUCLEO_F401RE_ROOT}/examples/04_timer_interrupt)

add_custom_target(all_examples
    DEPENDS
        nucleo-f401re-01_blink
        nucleo-f401re-02_uart_printf
        nucleo-f401re-03_i2c_sensor
        nucleo-f401re-04_timer_interrupt
)

add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake_install.cmake
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/*.bin
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/*.hex
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/*.map
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/Makefile
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/cmake_install.cmake
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/CMakeCache.txt
)

message(STATUS "")
message(STATUS "STM32F401RE Nucleo Build Configuration")
message(STATUS "=====================================")
message(STATUS "MCU:        STM32F401RE (Cortex-M4F)")
message(STATUS "Clock:      84 MHz")
message(STATUS "Optimization: -Os (size)")
message(STATUS "Debug:      -g3 (maximum)")
message(STATUS "")
message(STATUS "Examples:")
message(STATUS "  01_blink          - LED blink with SysTick delay")
message(STATUS "  02_uart_printf    - UART printf and echo")
message(STATUS "  03_i2c_sensor     - I2C bus scanner and sensor read")
message(STATUS "  04_timer_interrupt - Timer interrupt LED control")
message(STATUS "")
message(STATUS "Build targets:")
message(STATUS "  make nucleo-f401re-01_blink          - Build blink example")
message(STATUS "  make nucleo-f401re-02_uart_printf    - Build UART example")
message(STATUS "  make nucleo-f401re-03_i2c_sensor     - Build I2C example")
message(STATUS "  make nucleo-f401re-04_timer_interrupt - Build timer example")
message(STATUS "  make all_examples                    - Build all examples")
message(STATUS "  make nucleo-f401re-XX-flash          - Flash example via ST-Link")
message(STATUS "")
