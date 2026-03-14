# ==============================================================================
# CMake Toolchain File: ARM Cortex-M4F (GCC)
# ==============================================================================
# This file tells CMake that we are cross-compiling for a bare-metal ARM target.
# It explicitly defines the compiler paths, target architecture flags, and 
# runtime library specifications required for an embedded environment.
# 
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-arm-gcc.cmake -B build
# ==============================================================================

set(CMAKE_SYSTEM_NAME Generic)       # Indicates a bare-metal/RTOS system (no OS)
set(CMAKE_SYSTEM_PROCESSOR arm)

# ------------------------------------------------------------------------------
# Toolchain Executables
# ------------------------------------------------------------------------------
# Note: We assume the arm-none-eabi-* tools are in the system PATH. 
# For deterministic CI/CD, these could be hardcoded to specific absolute paths.
set(TOOLCHAIN_PREFIX arm-none-eabi-)

# Force CMake to use these compilers, skipping compiler checks which often fail 
# on bare-metal systems because they attempt to build and run a host executable.
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy CACHE INTERNAL "Objcopy tool")
set(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump CACHE INTERNAL "Objdump tool")
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size CACHE INTERNAL "Size tool")

# ------------------------------------------------------------------------------
# Target Architecture Flags (Cortex-M4F)
# ------------------------------------------------------------------------------
# -mcpu=cortex-m4: Generates instructions optimized for the Cortex-M4 architecture.
# -mthumb: Forces Thumb-2 instruction set (required for Cortex-M).
# -mfloat-abi=hard: Uses hardware floating-point registers (s0-s31) for passing 
#                   float arguments, significantly improving FPU performance.
# -mfpu=fpv4-sp-d16: Specifies the exact FPU variant present on the M4F (Single Precision).
# ------------------------------------------------------------------------------
set(TARGET_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")

# ------------------------------------------------------------------------------
# Runtime Library & Optimization Flags
# ------------------------------------------------------------------------------
# --specs=nano.specs: Links against Newlib-Nano instead of standard Newlib. 
#                     This drastically reduces the footprint of standard C library 
#                     functions (like printf) optimized for embedded systems.
# --specs=nosys.specs: Provides stub implementations for system calls (like _sbrk 
#                      for malloc, _write for printf) required by bare-metal.
# -flto: Link Time Optimization. Allows the compiler to inline functions and 
#        optimize across translation unit boundaries, reducing size and increasing speed.
# -ffat-lto-objects: Required when compiling static libraries with LTO enabled.
# ------------------------------------------------------------------------------
set(RUNTIME_FLAGS "--specs=nano.specs --specs=nosys.specs")
set(OPTIMIZATION_FLAGS "-flto -ffat-lto-objects")

# Combine all flags
set(CMAKE_C_FLAGS "${TARGET_FLAGS} ${RUNTIME_FLAGS} ${OPTIMIZATION_FLAGS}" CACHE INTERNAL "C Compiler flags")
set(CMAKE_CXX_FLAGS "${TARGET_FLAGS} ${RUNTIME_FLAGS} ${OPTIMIZATION_FLAGS} -fno-exceptions -fno-rtti" CACHE INTERNAL "C++ Compiler flags")
set(CMAKE_ASM_FLAGS "${TARGET_FLAGS}" CACHE INTERNAL "ASM Compiler flags")
set(CMAKE_EXE_LINKER_FLAGS "${TARGET_FLAGS} ${RUNTIME_FLAGS} ${OPTIMIZATION_FLAGS}" CACHE INTERNAL "Linker flags")

# ------------------------------------------------------------------------------
# CMake Find Behavior
# ------------------------------------------------------------------------------
# Tell CMake not to look for host environment paths when finding libraries/includes.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
