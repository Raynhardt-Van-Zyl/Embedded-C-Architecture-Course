# 11.4 Namespacing Conventions

## 1. The Catastrophe of the Flat Global Namespace
Modern high-level languages like C++, Python, and Rust feature built-in module systems and `namespace` keywords. These allow developers to isolate their code into logical domains. In C++, `std::vector` and `my_custom::vector` can coexist peacefully because the compiler keeps them separated in distinct namespaces.

The C language does not have this feature. **C possesses a single, flat, global namespace.**

When you compile a C project, the linker takes all the object files (`.o`) and attempts to resolve every global function name and every global variable name into a specific physical memory address. If the linker encounters two functions with the exact same name—even if they exist in completely different files or external libraries—it will crash with a "Multiple Definition" error, or worse, silently bind the call to the wrong function depending on linker flags.

### 1.1 The Inevitability of Collisions
In a small, 5-file project, naming a function `init()` or `process_data()` seems harmless. However, as an embedded codebase matures to hundreds of files and integrates third-party stacks (e.g., FreeRTOS, LwIP, vendor HALs, cryptography libraries), collisions are inevitable.

If you name your function `flash_init()` and your microcontroller vendor updates their HAL library to include a new internal function named `flash_init()`, your build will break. You are now tightly coupled to the naming conventions of external libraries you do not control.

#### Concrete Anti-Pattern: Generic Naming
```c
/* ANTI-PATTERN: Polluting the global namespace */
// file: eeprom_driver.h
#ifndef EEPROM_DRIVER_H
#define EEPROM_DRIVER_H

#include <stdint.h>

// 1. Generic enum names. If another driver defines STATUS_OK, the build fails.
typedef enum {
    STATUS_OK = 0,
    STATUS_ERROR = 1,
    STATUS_BUSY = 2
} status_t;

// 2. Generic struct names.
typedef struct {
    uint8_t buffer[256];
    uint32_t address;
} config_t;

// 3. Generic function names. What is being initialized?
void init(void);
status_t read_data(config_t *cfg);
status_t write_data(config_t *cfg);

#endif
```

## 2. Production-Grade Namespacing in C
Because the language does not provide namespaces, we must emulate them using strict naming conventions and linker directives. The solution is two-fold: Public Prefixing and Internal Linkage (`static`).

### 2.1 Public Symbols: The Module Prefix
Every public symbol (functions, enums, structs, macros) that is exposed in a header file **MUST** be prefixed with the name of the module it belongs to. This prefix acts as our artificial namespace.

If the module is `eeprom_driver.c`, the prefix is `EEPROM_` or `eeprom_`.

```c
/* PRODUCTION-GRADE: Emulating namespaces with prefixes */
// file: eeprom_driver.h
#pragma once

#include <stdint.h>

// 1. Prefixed enumerations. The type is prefixed, and the values are prefixed.
typedef enum {
    EEPROM_STATUS_OK = 0,
    EEPROM_STATUS_ERROR = 1,
    EEPROM_STATUS_BUSY = 2
} eeprom_status_t;

// 2. Prefixed structs.
typedef struct {
    uint8_t buffer[256];
    uint32_t address;
} eeprom_config_t;

// 3. Prefixed functions. The intent is instantly clear at the call site.
void eeprom_init(void);
eeprom_status_t eeprom_read_data(eeprom_config_t *cfg);
eeprom_status_t eeprom_write_data(eeprom_config_t *cfg);
```

When a developer in another part of the codebase writes `eeprom_init()`, they know exactly which module they are calling. When they search the codebase for `EEPROM_STATUS_ERROR`, `grep` will instantly find the exact definition without returning hundreds of false positives from other modules.

### 2.2 Private Symbols: The `static` Keyword
Prefixing public symbols solves the problem for the interface. But what about the implementation? Inside `eeprom_driver.c`, you might need helper functions like `calculate_crc()` or `wait_for_ready()`. If you don't prefix these, they pollute the global namespace just like public functions.

However, prefixing every single private helper function makes the code verbose and difficult to read. The C language provides a powerful mechanism to hide symbols from the linker: the `static` keyword.

**The Deep Technical Rationale (Internal vs. External Linkage):**
By default, all functions and variables declared at the file scope in C have **External Linkage**. This means the compiler places their names into the symbol table of the object file (`.o`), making them visible to the linker so they can be called from other files.

When you apply the `static` keyword to a file-scope function or variable, you change its linkage to **Internal Linkage**. The compiler still compiles the function, but it hides the name from the linker's global symbol table. The function can only be called from within the specific `.c` file where it is defined.

```c
/* PRODUCTION-GRADE: Hiding private symbols with 'static' */
/* file: eeprom_driver.c */
#include "eeprom_driver.h"
#include "hardware_i2c.h"

// 1. Private State Variable. Hidden from the linker.
// No other file can accidentally modify this variable.
static eeprom_config_t internal_config_cache;

// 2. Private Helper Function. Hidden from the linker.
// We can name this whatever we want (calculate_crc) because it will 
// never collide with another module's calculate_crc() function.
static uint16_t calculate_crc(const uint8_t *data, uint32_t length) {
    uint16_t crc = 0xFFFF;
    // ... calculate CRC ...
    return crc;
}

// 3. Public Function. Exposed via the header file.
eeprom_status_t eeprom_write_data(eeprom_config_t *cfg) {
    uint16_t checksum = calculate_crc(cfg->buffer, 256);
    // ... write to hardware ...
    return EEPROM_STATUS_OK;
}
```

The strict use of `static` is one of the most vital architectural disciplines in embedded C. It enforces encapsulation at the linker level. If a developer attempts to call `calculate_crc()` from another file using an `extern` declaration, the linker will aggressively reject it with an "Undefined Reference" error.

## 3. Company Standard Rules

**Rule 11.4.1:** The C global namespace MUST be protected. All public functions, enumerations, structures, `typedef` names, and exposed macros MUST be prefixed with the name or abbreviation of the module they belong to (e.g., `SPI_Transmit()`, `spi_config_t`).
**Rule 11.4.2:** Every variable and function defined within a `.c` file that is not explicitly exposed in the corresponding `.h` file interface MUST be declared with the `static` keyword. This enforces internal linkage and prevents linker symbol collisions.
**Rule 11.4.3:** Enumeration values MUST be prefixed with the module name or the enum type name to prevent collisions with other enums (e.g., `UART_PARITY_NONE`, not `NONE`).
**Rule 11.4.4:** The `extern` keyword MUST NOT be used in a `.c` file to manually link to a variable or function in another file. If a module requires access to another module's data or functionality, it MUST include that module's public header file.