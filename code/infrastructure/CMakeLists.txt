# ==============================================================================
# Master CMakeLists.txt for Embedded Architecture Course
# ==============================================================================
cmake_minimum_required(VERSION 3.20)

project(EmbeddedArchitectureCourse VERSION 1.0.0 LANGUAGES C ASM)

# ------------------------------------------------------------------------------
# Strict Compiler Flags
# ------------------------------------------------------------------------------
set(COMMON_COMPILER_FLAGS
    "-Wall"
    "-Wextra"
    "-Werror"
    "-Wconversion"
    "-pedantic"
    "-std=c11"
    "-Wshadow"
    "-Wdouble-promotion"
    "-Wundef"
    "-fno-common"
    "-ffunction-sections"
    "-fdata-sections"
)
add_compile_options(${COMMON_COMPILER_FLAGS})

# ------------------------------------------------------------------------------
# Include Directories
# ------------------------------------------------------------------------------
include_directories(
    .
    ../part2-modular-design/hal_example
    ../part2-modular-design/module_pattern
    ../part3-execution-models/isr_patterns
    ../part3-execution-models/rtos_patterns
    ../part3-execution-models/state_machines
    ../part3-execution-models/superloop
    ../part4-code-quality/assertions
    ../part4-code-quality/diagnostics
    ../part4-code-quality/error_handling
    ../part5-testability/mocking
    ../part6-standardization/legacy_migration
    ../../hardware/stm32/nucleo-f401re/include
    ../../hardware/stm32/nucleo-f401re/bsp
    ../../hardware/stm32/nucleo-f401re/hal
)

# ------------------------------------------------------------------------------
# Core Library (Platform-Agnostic Logic)
# ------------------------------------------------------------------------------
add_library(core_firmware STATIC
    ../part2-modular-design/module_pattern/module_template.c
    ../part3-execution-models/isr_patterns/isr_safe.c
    ../part3-execution-models/rtos_patterns/rtos_task_template.c
    ../part3-execution-models/rtos_patterns/safe_queue.c
    ../part3-execution-models/state_machines/hsm_template.c
    ../part3-execution-models/superloop/superloop.c
    ../part4-code-quality/assertions/app_assert.c
    ../part4-code-quality/diagnostics/diagnostics.c
    ../part4-code-quality/error_handling/error_handler.c
    ../part6-standardization/legacy_migration/legacy_adapter.c
)

# ------------------------------------------------------------------------------
# Hardware Library (STM32 Specific)
# ------------------------------------------------------------------------------
add_library(hardware_hal STATIC
    ../../hardware/stm32/nucleo-f401re/hal/hal_gpio.c
    ../../hardware/stm32/nucleo-f401re/hal/hal_uart.c
    ../../hardware/stm32/nucleo-f401re/hal/hal_i2c.c
    ../../hardware/stm32/nucleo-f401re/hal/hal_adc.c
    ../../hardware/stm32/nucleo-f401re/hal/hal_timer.c
    ../../hardware/stm32/nucleo-f401re/startup/startup_stm32f401xe.c
)

# ------------------------------------------------------------------------------
# Executable
# ------------------------------------------------------------------------------
add_executable(firmware.elf 
    ../part3-execution-models/superloop/main.c 
)

target_link_libraries(firmware.elf PRIVATE 
    core_firmware
    hardware_hal
)

# ------------------------------------------------------------------------------
# Linker Configuration
# ------------------------------------------------------------------------------
if(CMAKE_CROSSCOMPILING)
    set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../../hardware/stm32/nucleo-f401re/linker/stm32f401xe.ld)
    target_link_options(firmware.elf PRIVATE
        "-Wl,--gc-sections"
        "-T${LINKER_SCRIPT}"
        "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/firmware.map"
        "-Wl,--print-memory-usage"
    )
endif()

# ------------------------------------------------------------------------------
# Post-Build Steps
# ------------------------------------------------------------------------------
if(CMAKE_CROSSCOMPILING)
    add_custom_command(TARGET firmware.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:firmware.elf> ${CMAKE_CURRENT_BINARY_DIR}/firmware.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:firmware.elf> ${CMAKE_CURRENT_BINARY_DIR}/firmware.bin
        COMMENT "Generating .hex and .bin files for flashing"
    )
endif()
