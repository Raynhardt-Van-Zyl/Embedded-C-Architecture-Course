# Embedded C Course Architectural Rules

The following rules must be strictly adhered to when generating, refactoring, or reviewing code for this course.

## 1. Core Language & Types
*   **Use `<stdint.h>` types exclusively:** `uint32_t`, `int8_t`, etc., instead of `int`, `long`, `char`.
*   **No Dynamic Memory:** Never use `malloc`, `calloc`, or `free`. All memory must be statically allocated or stack-allocated (with strict depth control).
*   **Bracing:** Always use braces `{}` for all control structures (`if`, `for`, `while`), even for single-line statements.
*   **Const Correctness:** Use `const` for any pointer or variable that should not be modified.
    *   Example: `void HAL_UART_Send(const uint8_t *data, uint16_t len);`

## 2. Modularity & State Management
*   **Encapsulation:** All module state must be encapsulated using `static` variables (if singleton) or opaque context structs (if multi-instance).
*   **Global Variables:** Avoid global variables. If necessary, mark them `static` to limit scope to the file.
*   **Opaque Pointers (PIMPL):** Hide implementation details in the `.c` file; the public header should only expose a `typedef struct Module_s* ModuleHandle_t;`.

## 3. Hardware Abstraction (HAL)
*   **Function Pointers:** Hardware interactions should be abstracted via structs of function pointers to allow for mocking and portability.
*   **Registers:** Access hardware registers only within the `hardware/` directory or specific BSP/HAL implementation files.
*   **Volatile:** Use `volatile` for all hardware registers and variables shared with ISRs.

## 4. Error Handling
*   **Return Status:** Functions that can fail must return a status enum (e.g., `Status_t` or `err_t`).
*   **Output Parameters:** Return data via pointer arguments, not as the function return value.
*   **No Assertions in Production:** Use `APP_ASSERT` macros that can be compiled out or redirected to a safe fault handler, not `assert.h`.

## 5. Coding Style
*   **Function Length:** Keep functions under 50 lines. Break complex logic into static helper functions.
*   **Comments:** Explain the *why*, not the *what*. Document architectural decisions (e.g., "Using polling here to avoid interrupt latency jitter").
*   **Naming:**
    *   **Modules:** PascalCase (e.g., `HalGpio_Init`).
    *   **Variables:** snake_case (e.g., `rx_buffer_len`).
    *   **Constants/Macros:** UPPER_SNAKE_CASE (e.g., `MAX_RX_BUFFER_SIZE`).

## 6. Safety Patterns
*   **Input Validation:** Check all pointer arguments for `NULL` at the start of public APIs.
*   **Bounded Loops:** All `while` loops waiting for hardware flags must have a timeout counter.
*   **Critical Sections:** Use `ENTER_CRITICAL()` / `EXIT_CRITICAL()` macros around shared data modification, keeping the critical section as short as possible.
