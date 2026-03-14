# Chapter 14.3: Host vs. Target Testing (Dual-Target Strategy)

A cornerstone of the 20-year embedded software architecture is the **Dual-Target Strategy**. In this paradigm, the codebase is structurally engineered, rigorously configured, and consistently built to execute on two fundamentally distinct hardware platforms:
1. **The Host:** The developer's machine and the Continuous Integration (CI) servers (typically Windows, Linux, or macOS running on x86_64 or ARM64 architectures).
2. **The Target:** The physical embedded hardware (e.g., an ARM Cortex-M4, a RISC-V RV32IMC, or an automotive PowerPC chip).

Failure to adopt and enforce this dual-target approach results in the infamous "Hardware Bottleneck." In a hardware-bottlenecked organization, every line of logic—no matter how mathematically pure or isolated—must be flashed to a physical board, often connected via a flaky JTAG probe, simply to verify if a state machine transitions correctly. This is an unsustainable, deeply flawed engineering process that destroys developer velocity and guarantees low-quality firmware.

## The Host Environment: The Domain of Fast Feedback

The primary objective of host testing is to verify the **business logic**, **complex algorithms**, **state machines**, and **data parsing** of the application.

### Why Host Testing is Non-Negotiable
1. **Compilation and Execution Speed:** Cross-compiling for an MCU, linking, launching a GDB server, flashing over SWD, and executing tests takes anywhere from 10 seconds to 5 minutes. Conversely, a host build compiling natively with GCC or Clang can execute a suite of 5,000 unit tests in under 500 milliseconds. This near-instantaneous feedback loop is the foundation of Test-Driven Development (TDD).
2. **Architectural Isolation Enforcement:** Because the host PC does not possess I2C buses, hardware timers, or memory-mapped ADC registers, compiling code for the host *forces* the developer to write modular, decoupled code using the seams defined in Chapter 14.2. If code compiles on the host, it is empirically proven to be decoupled from the hardware.
3. **Advanced Dynamic Analysis Tooling:** Running code natively on a PC unlocks the usage of enterprise-grade dynamic analysis tools that are largely impossible to run on resource-constrained embedded targets.

### Deep Dive: Sanitizers (ASan and UBSan)
When we execute our unit tests on the host, we do not simply build standard executables. We leverage compiler instrumentation specifically designed to catch devastating C errors at runtime.

**AddressSanitizer (ASan):** By passing `-fsanitize=address` to Clang/GCC on the host, the compiler instruments every memory access. It surrounds allocations (stack, heap, and globals) with "redzones." If your embedded C code calculates a buffer index incorrectly and writes one byte past the end of an array, ASan will immediately intercept the memory access, halt the unit test, and print a forensic stack trace detailing the exact line of code causing the buffer overflow. On a target MCU, this same bug would silently corrupt adjacent memory, leading to an impossible-to-debug hard fault hours later.

**UndefinedBehaviorSanitizer (UBSan):** C is riddled with Undefined Behavior (UB)—signed integer overflows, shifting by negative amounts, division by zero. UBSan (`-fsanitize=undefined`) instruments the binary to catch these mathematical impossibilities at runtime during your unit tests, guaranteeing that your core logic algorithms are mathematically sound before they ever touch physical silicon.

## The Target Environment: Integration and Reality Check

If host testing verifies that the *logic* is correct, target testing verifies that the *system* actually works in the physical universe. Host tests are blind to hardware timing, interrupt preemption, compiler quirks on the target architecture, and actual electron flow.

Target testing proves that the cross-compiler, the custom linker script, the RTOS configuration, the HAL drivers, and the physical silicon all cooperate harmoniously.

### What MUST be tested on the Target?
- **Driver Integration (BSP Testing):** Does the SPI driver actually toggle the MOSI pin? Can we successfully read the WHO_AM_I register from the physical IMU sensor?
- **Timing and Deadlines:** Verifying that a high-priority hardware interrupt completes its execution within the strict 15-microsecond deadline required by the mechanical actuators.
- **Resource Constraints:** Profiling stack usage under maximum interrupt load, verifying heap fragmentation over a 48-hour burn-in period, and measuring CPU utilization.

## Real-World Implementation: CMake Dual-Targeting

To orchestrate this strategy, the build system (we mandate CMake) must be elegantly designed to pivot between host and target builds seamlessly. 

The mechanism relies on CMake Toolchain files. A toolchain file overrides CMake's default host compiler search and points it to the embedded cross-compiler.

**Example `toolchain-arm-gcc.cmake`:**
```cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Point to the cross-compiler
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

# Target-specific silicon flags
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CPU_FLAGS} -ffunction-sections -fdata-sections" CACHE INTERNAL "c compiler flags")
```

**The Application CMakeLists.txt:**
The project's root `CMakeLists.txt` intelligently decides what to build based on the active toolchain.

```cmake
cmake_minimum_required(VERSION 3.20)
project(CompanyFirmware C)

# Add the hardware-agnostic business logic
add_subdirectory(src/app)

# Conditional compilation based on the target system
if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    message(STATUS "TARGET BUILD: Compiling HAL and BSP")
    add_subdirectory(src/hal)
    add_subdirectory(src/bsp)
    
    # Link final target executable
    add_executable(firmware.elf src/main.c)
    target_link_libraries(firmware.elf app_logic bsp_drivers)
else()
    message(STATUS "HOST BUILD: Compiling Unit Tests with Sanitizers")
    # Enable AddressSanitizer for host builds
    add_compile_options(-fsanitize=address,undefined -g -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
    
    add_subdirectory(tests/unit)
endif()
```

By structuring the build system this way, a developer simply runs `cmake -B build-host` to build the tests natively, and `cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-arm-gcc.cmake -B build-target` to build the firmware for the MCU.

## Hardware-in-the-Loop (HIL) Automation

Manual target testing via JTAG debuggers is unscalable and cannot be trusted for regression testing. The company standard dictates that target testing must be automated via HIL setups.

A modern HIL pipeline consists of:
1. A CI Runner (GitLab/Jenkins) triggered by a pull request.
2. The Runner compiles the target `.elf` and `.hex` files.
3. The Runner executes a Python script using tools like `PyOCD` or `JLinkExe` to silently flash the binary onto a real PCB connected via USB.
4. The target firmware is built with an embedded test framework (like Unity) linked in. Upon boot, the target executes the driver test suite.
5. The Python script parses the UART/RTT output from the target, reads the XML test results, and passes/fails the CI pipeline.

### The Testing Pyramid
This architecture solidifies the Embedded Testing Pyramid:
- **Base (80% of volume):** Host Unit Tests. Lightning fast, heavily mocked, fully covering algorithmic logic, executed on every local save and commit.
- **Middle (15% of volume):** Target Integration Tests (HIL). Slower, tests real silicon drivers and RTOS behavior, executed automatically on Pull Requests.
- **Top (5% of volume):** Manual System Validation. Physical UX testing, EMC/EMI emissions testing, environmental chamber testing.

---

## Company Standard Rules

**Rule 14.3.1:** **Dual-Target Build Capability.** The primary build system (CMake) must fully support executing two distinct build targets: a native host build for unit testing and an embedded cross-compilation build for the final firmware. The host build must succeed on at least one major desktop OS (Linux, Windows via WSL, or macOS).

**Rule 14.3.2:** **Host Sanitizer Enforcement.** All host-based unit test builds must be compiled and linked with AddressSanitizer (`-fsanitize=address`) and UndefinedBehaviorSanitizer (`-fsanitize=undefined`) enabled. A sanitizer trap during a test run is considered a hard failure and must block the CI pipeline, identical to a test assertion failure.

**Rule 14.3.3:** **The 80/20 Test Distribution.** At minimum, 80% of the total test cases written for the system must be host-based unit tests testing the application layer. No more than 20% of the automated tests should require physical execution on the target hardware.

**Rule 14.3.4:** **No Interactive Debugging as Verification.** Manual step-through debugging via JTAG/SWD is a tool for investigation, not verification. "It worked when I stepped through it on my dev board" is an unacceptable justification for merging code. All behavioral requirements must be proven via automated tests.