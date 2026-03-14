# 11.1 Header File Responsibilities

## 1. The Preprocessor's Literal Copy-Paste
To master C architecture, you must fundamentally understand how the compiler reads your code. Unlike modern languages (Java, C#, Rust) which have integrated module systems, C relies on a primitive text-processing step called the **C Preprocessor**.

When you write `#include "uart_driver.h"` in your `main.c` file, the preprocessor literally opens `uart_driver.h`, copies every single byte of text within it, and pastes it into `main.c` before handing the resulting massive text blob (the Translation Unit) to the actual compiler. 

If your header file contains implementation details, variable definitions, or bloated nested includes, you are forcing the compiler to parse that identical code hundreds of times across your project. This destroys compilation speed and creates impossible-to-debug linker errors.

### 1.1 The Interface vs. The Implementation
The golden rule of header files in an embedded architecture is absolute separation of concerns. 
*   **The Header File (`.h`) is the Interface:** It defines *what* a module does. It contains function prototypes, public enums, public macros, and opaque types. It is a strict contract between the module and its callers.
*   **The Source File (`.c`) is the Implementation:** It defines *how* the module does it. It contains the actual C code, private static helper functions, private state variables, and the internal struct definitions.

#### Concrete Anti-Pattern: Leaking Implementation Details
The most common architectural violation in C is leaking the memory layout of a struct into the public header file when the caller only needs a handle.

```c
/* ANTI-PATTERN: Leaking the struct layout in the public header */
// file: flash_memory.h
#ifndef FLASH_MEMORY_H
#define FLASH_MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include "spi_driver.h" // We are forcing every caller to include SPI!

// The caller only wants to read/write flash. Why do they need to know
// about our internal SPI handle, our page size, or our internal state?
typedef struct {
    spi_handle_t spi_bus;
    uint32_t current_page;
    bool is_busy;
    uint8_t buffer[256];
} flash_ctx_t;

void flash_init(flash_ctx_t *ctx);
void flash_write(flash_ctx_t *ctx, uint32_t addr, const uint8_t *data, uint16_t len);

#endif // FLASH_MEMORY_H
```

**The Deep Technical Rationale (Coupling & Recompilation):**
If a developer decides to change the internal buffer size from `256` to `512` in the `flash_ctx_t` struct, they have changed the size of the struct. Because this struct is in the public header, every single `.c` file in the entire project that includes `flash_memory.h` must be recompiled, even if they never touch the `buffer` field. 

Furthermore, because `spi_handle_t` is inside the struct, `flash_memory.h` must `#include "spi_driver.h"`. Now, every file that uses the flash driver is implicitly coupled to the SPI driver. This is a "Big Ball of Mud" dependency graph.

**The Production-Grade Solution: Opaque Pointers (The PIMPL Idiom)**
In a robust embedded architecture, we hide the implementation using an opaque pointer (also known as a forward-declared struct or the Pointer to IMPLementation idiom).

```c
/* PRODUCTION-GRADE: Opaque Pointers in the Header */
// file: flash_memory.h
#ifndef FLASH_MEMORY_H
#define FLASH_MEMORY_H

#include <stdint.h>

// 1. Forward declaration. We tell the compiler: "A struct named 
// flash_ctx exists somewhere. I am not telling you its size or contents."
struct flash_ctx;

// 2. We define an opaque handle. Callers can pass this pointer around,
// but they cannot dereference it or see inside it.
typedef struct flash_ctx* flash_handle_t;

// 3. The public API only uses the opaque handle.
flash_handle_t flash_init(void);
void flash_write(flash_handle_t handle, uint32_t addr, const uint8_t *data, uint16_t len);

#endif // FLASH_MEMORY_H
```

Inside the corresponding `.c` file, we actually define the struct layout. The implementation is completely hidden from the rest of the application.

```c
/* file: flash_memory.c */
#include "flash_memory.h"
#include "spi_driver.h" // SPI is only included where it is actually used!
#include <stdlib.h>     // For memory allocation

// The compiler now knows the size and layout, but only within this single C file.
struct flash_ctx {
    spi_handle_t spi_bus;
    uint32_t current_page;
    uint8_t buffer[512]; // We can change this without recompiling the project!
};

flash_handle_t flash_init(void) {
    // In a bare-metal system, this might return a pointer to statically allocated memory.
    // In an RTOS, it might use malloc. The caller doesn't need to know!
    struct flash_ctx *new_ctx = allocate_context();
    new_ctx->current_page = 0;
    return new_ctx;
}
```

### 1.2 The Linker Perspective: Multiple Definition Errors
A header file must *never* contain a variable definition or a non-inline function body. 

If you put `int global_error_count = 0;` inside a header file, and three different `.c` files `#include` that header, the preprocessor will paste that definition into all three Translation Units. When the linker attempts to merge the object files (`.o`), it will find three identical symbols named `global_error_count`. Depending on the compiler flags (e.g., `-fno-common` which is default in GCC 10+), the linker will crash with a **Multiple Definition Error**.

If you must expose a global variable (which is generally an architectural smell), you must use the `extern` keyword in the header file (`extern int global_error_count;`) and define it exactly once in a single `.c` file.

### 1.3 The Guard Mechanism: `#ifndef` vs `#pragma once`
Because headers are copy-pasted, it is entirely possible for a file to be included twice. If `main.c` includes `sensor_a.h` and `sensor_b.h`, and both of those headers include `common_types.h`, the compiler will process `common_types.h` twice. This leads to redefinition errors for `struct` and `typedef`.

We prevent this using include guards. The legacy, ISO C standard way is the `#ifndef` wrapper:

```c
#ifndef MY_HEADER_H
#define MY_HEADER_H
// Header content...
#endif
```

However, modern compilers (GCC, Clang, MSVC, ARM) universally support the `#pragma once` directive. It achieves the exact same result but is processed natively by the preprocessor engine rather than relying on macro expansion. It is faster, less prone to copy-paste naming errors, and cleaner. In modern 20-year codebases, `#pragma once` is preferred, with `#ifndef` guards as a secondary fallback only if an obscure proprietary compiler requires it.

## 2. Company Standard Rules

**Rule 11.1.1:** Every header file (`.h`) MUST be protected against multiple inclusions using `#pragma once` at the very top of the file. Legacy `#ifndef` guards may be used as a supplementary fallback but are not required unless compiling for a legacy toolchain.
**Rule 11.1.2:** A header file MUST NOT contain variable definitions or non-inline function bodies. Global variables, if strictly necessary, MUST be declared as `extern` in the header and defined in exactly one `.c` file.
**Rule 11.1.3:** Header files MUST define the public interface of a module. Internal state structures, private helper functions, and hardware-specific handles MUST be hidden inside the corresponding `.c` file using the Opaque Pointer (PIMPL) idiom to enforce encapsulation and minimize recompilation.
**Rule 11.1.4:** A header file MUST be entirely self-contained. If a developer includes `my_module.h` in an empty `.c` file, it MUST compile without errors. It is the responsibility of the header file to `#include` only the specific dependencies it requires to parse its own prototypes (e.g., `<stdint.h>`, `<stdbool.h>`).