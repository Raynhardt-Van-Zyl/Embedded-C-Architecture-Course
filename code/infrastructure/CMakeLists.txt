# ==============================================================================
# Master CMakeLists.txt for Embedded Infrastructure
# ==============================================================================
# This is the gold-standard build configuration for a safety-critical embedded C 
# project. It enforces strict separation of concerns, uncompromising compiler 
# warnings, and clean abstractions between the host architecture and the target 
# cross-compilation environment.
# ==============================================================================

cmake_minimum_required(VERSION 3.20)

# ------------------------------------------------------------------------------
# Project Definition
# ------------------------------------------------------------------------------
# Note: Languages are explicitly omitted here (NONE) until the toolchain file 
# is loaded, ensuring CMake doesn't try to use the host compiler (gcc/msvc) 
# to configure tests for the target architecture.
project(EmbeddedInfrastructure VERSION 1.0.0 LANGUAGES C ASM)

# ------------------------------------------------------------------------------
# Strict Compiler Flags (The "Safety Net")
# ------------------------------------------------------------------------------
# These flags are non-negotiable for safety-critical systems. They turn the 
# compiler into a rigorous static analysis tool.
#
# -Wall -Wextra: Enables all standard and extra warnings.
# -Werror: Treats all warnings as build failures. Zero tolerance for ambiguity.
# -Wconversion: Catches implicit casting that can lead to data loss (e.g., 
#               truncating a 32-bit sensor value into a 16-bit variable).
# -pedantic: Enforces strict ISO C standard compliance, rejecting compiler extensions.
# -std=c11: Locks the standard to C11, a modern but stable embedded baseline.
# -Wshadow: Prevents variable shadowing, a common source of logic errors.
# -Wdouble-promotion: Warns if float is implicitly promoted to double (very costly
#                     on Cortex-M4F which only has single-precision FPU).
# -fno-common: Enforces strict symbol definition (no tentative definitions).
# -ffunction-sections / -fdata-sections: Allows the linker to garbage collect 
#                                        unused code and data.
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

# Apply these flags globally to all targets created in this directory and below
add_compile_options(${COMMON_COMPILER_FLAGS})

# ------------------------------------------------------------------------------
# Architecture Split: Library vs. Executable
# ------------------------------------------------------------------------------
# In a rigorous architecture, the core business logic (drivers, middleware) 
# is compiled as a static library. This ensures strict dependency boundaries 
# and allows the library to be unit-tested on the host (with a different CMake 
# configuration) by mocking the hardware interface.

# 1. Define the Core Library Target
# This contains the hardware-agnostic logic and HAL interfaces.
add_library(core_firmware STATIC)

# Specify sources explicitly. Globbing (file(GLOB...)) is forbidden as it 
# breaks CMake's dependency tracking and reproducible builds.
# (Populate with actual source files later)
# target_sources(core_firmware PRIVATE src/hal_gpio.c src/sensor_processing.c)

# Create an empty dummy file just to satisfy CMake configuration before real files exist
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_lib.c "void __dummy_lib(void) {}")
target_sources(core_firmware PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/dummy_lib.c)

# Specify include directories. 
target_include_directories(core_firmware PUBLIC include)

# 2. Define the Executable Target
# This is the final binary that will be flashed to the microcontroller.
# It contains the main() loop and specific hardware instantiation.
add_executable(firmware.elf)

# target_sources(firmware.elf PRIVATE src/main.c src/startup_arm.c)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_exe.c "int main(void) { return 0; }")
target_sources(firmware.elf PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/dummy_exe.c)

# Link the core library to the executable
target_link_libraries(firmware.elf PRIVATE core_firmware)

# ------------------------------------------------------------------------------
# Linker Configuration (Target Specific)
# ------------------------------------------------------------------------------
# -Wl,--gc-sections: Instructs the linker to discard unused functions/data 
#                    sections, drastically reducing the final ROM footprint.
# -T linker_script.ld: Applies our custom memory map.
# -Wl,-Map: Generates a map file for memory analysis and auditing.
# ------------------------------------------------------------------------------
if(CMAKE_CROSSCOMPILING)
    target_link_options(firmware.elf PRIVATE
        "-Wl,--gc-sections"
        "-T${CMAKE_CURRENT_SOURCE_DIR}/linker_script.ld"
        "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/firmware.map"
        "-Wl,--print-memory-usage" # Prints FLASH/RAM utilization summary at build end
    )
endif()

# ------------------------------------------------------------------------------
# Post-Build Steps: Generating Hex/Bin
# ------------------------------------------------------------------------------
# Embedded programming requires raw binary or Intel Hex formats for flashing,
# not just ELF files.
if(CMAKE_CROSSCOMPILING)
    add_custom_command(TARGET firmware.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:firmware.elf> ${CMAKE_CURRENT_BINARY_DIR}/firmware.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:firmware.elf> ${CMAKE_CURRENT_BINARY_DIR}/firmware.bin
        COMMENT "Generating .hex and .bin files for flashing"
    )
endif()
