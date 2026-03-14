# Chapter 17.1: Codebase Layout Standard

The directory structure of an embedded project is its architectural map. It is the physical manifestation of your separation of concerns. When a new developer clones a repository for the first time, the folder layout should instantly and silently communicate exactly how the system is organized, where the hardware dependencies are buried, and where the pure business logic resides.

A chaotic directory layout—such as 50 `.c` and `.h` files dumped into a single root `src/` directory—actively encourages chaotic dependency management. If the PID controller logic lives in the same folder as the STM32 SPI driver, developers will inevitably and lazily `#include` the hardware headers directly into the algorithmic logic. 

To prevent this, the internal framework must dictate a strict, universally applied directory layout across all company projects, and the build system must ruthlessly enforce the boundaries between these directories.

## The Standard Embedded Project Layout

While the specific nomenclature can vary slightly, a robust, 20-year embedded C standard layout must enforce a strict separation of concerns, heavily inspired by "Layered Architecture" or Robert C. Martin's "Clean Architecture."

```text
company_project_template/
├── .clang-format           # Standardized code formatting rules
├── .clang-tidy             # Standardized static analysis rules
├── CMakeLists.txt          # Root build script orchestrating the subdirectories
├── README.md
│
├── src/                    # All production source code lives here
│   ├── app/                # Application & Business Logic (100% Hardware Independent)
│   │   ├── state_machine/
│   │   ├── pid_controller/
│   │   └── data_parser/
│   │
│   ├── middleware/         # Third-party stacks or internal core services
│   │   ├── rtos/           # FreeRTOS, Zephyr, etc. (Abstracted via OSAL)
│   │   ├── lwip/           # Networking stack
│   │   └── company_log/    # Framework: Standard asynchronous logging module
│   │
│   ├── hal/                # Hardware Abstraction Layer (The Interface Boundary)
│   │   ├── include/        # ONLY headers defining the interfaces (e.g., spi_if.h)
│   │   └── src/            # Generic HAL logic or interface wrappers
│   │
│   ├── bsp/                # Board Support Package (100% Hardware Dependent)
│   │   ├── stm32g4_core/   # Vendor SDKs and specific implementations of HAL interfaces
│   │   └── custom_pcb_v2/  # Pin mappings and board-specific initialization
│   │
│   └── main.c              # Application entry point, system composition, dependency injection
│
├── tests/                  # Unit and Integration Tests (Compiles natively on Host PC)
│   ├── unit/               # Tests for app logic, mirroring the src/app structure
│   ├── mocks/              # CMock/FFF generated test doubles for HAL interfaces
│   └── CMakeLists.txt      # Test build script (uses Host compiler, links Sanitizers)
│
├── tools/                  # Scripts for CI/CD, flashing, debugging, linting
│   ├── flash_target.py
│   └── run_static_analysis.sh
│
└── docs/                   # Architectural diagrams (Mermaid), state machine documentation
```

## Deep Technical Rationale and Enforced Rules

This layout is not merely an aesthetic preference; it is a structural necessity designed to enforce the Dual-Target testing strategy discussed in Chapter 14.

### 1. The `app/` Isolation Rule
Code inside the `src/app/` directory contains the core intellectual property of the company. It is strictly forbidden from including any headers from `src/bsp/` or from vendor-specific SDK folders. The business logic must be completely agnostic to the fact that it is running on a microcontroller. It should compile natively on a Windows, Mac, or Linux machine.

### 2. The `hal/include/` Interface Contract
The `src/hal/include/` folder represents the critical architectural boundary—the "Seam." 
- Application code (`src/app/`) includes these headers to command the hardware (e.g., calling `spi->transmit()`).
- BSP code (`src/bsp/`) includes these headers to implement the hardware drivers (e.g., writing the ST-specific register manipulations that fulfill the `transmit` contract).
This is **Dependency Inversion** materialized in the file system.

### 3. Vendor SDK Quarantine
Silicon vendor-provided code (e.g., STM32 HAL, NXP MCUXpresso, Microchip Harmony) is notoriously bloated and deeply coupled to specific compiler versions. This code must be quarantined entirely within the `src/bsp/` directory. It must never leak into the `middleware/` or `app/` layers.

## Enforcing the Layout via CMake Boundaries

A layout standard is entirely useless if it relies on human discipline. It must be enforced by the build system. Modern CMake provides the perfect mechanism for this: **Target Include Directories with `PRIVATE` scopes.**

When you configure CMake, you define libraries for each layer and explicitly restrict what directories they are allowed to look at for `#include` resolution.

**CMake Enforcement Example (`src/app/CMakeLists.txt`):**
```cmake
# Create a static library for the application logic
add_library(app_logic STATIC 
    pid_controller/pid.c 
    state_machine/main_fsm.c
)

# CRITICAL: App logic is ONLY allowed to include its own headers, 
# the HAL interfaces, and standard C libraries.
target_include_directories(app_logic PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}           # Can see itself
    ${CMAKE_SOURCE_DIR}/src/hal/include   # Can see the hardware interfaces
    ${CMAKE_SOURCE_DIR}/src/middleware    # Can see the logging/OSAL framework
)

# Note: ${CMAKE_SOURCE_DIR}/src/bsp is explicitly omitted. 
# If an engineer writes #include "stm32f4xx.h" inside pid.c, 
# the CMake host compiler will instantly throw a "File not found" fatal error.
```

By institutionalizing this exact folder structure via a template repository (e.g., a Git repository named `firmware-project-template` that developers must fork to start any new project), you instantly elevate the architectural baseline of the entire company. You make doing the wrong thing (coupling logic to hardware) physically difficult, and doing the right thing (dependency injection) the default path of least resistance.

---

## Company Standard Rules

**Rule 17.1.1:** **Mandatory Template Inheritance.** All new embedded firmware projects must be initialized by cloning the official `firmware-project-template` repository. Deviations from the standard root directory layout (`src/app`, `src/hal`, `src/bsp`, `tests/`) require approval from the Architecture Review Board.

**Rule 17.1.2:** **Build System Boundary Enforcement.** The build system (CMake) must strictly enforce include path boundaries. The `src/app/` library targets must be configured such that they physically cannot resolve include paths pointing to `src/bsp/` or vendor SDK directories.

**Rule 17.1.3:** **The HAL Inclusion Rule.** The Hardware Abstraction Layer (`src/hal/include/`) shall contain only abstract interface definitions (C-structs with function pointers and enumerations). It must contain absolutely zero silicon-vendor specific data types, pragmas, or register definitions.

**Rule 17.1.4:** **Host Compilability of the App Layer.** The entire contents of the `src/app/` directory must be fully compilable into a static library or executable using a standard desktop compiler (GCC, Clang, MSVC) without requiring a cross-compilation toolchain.