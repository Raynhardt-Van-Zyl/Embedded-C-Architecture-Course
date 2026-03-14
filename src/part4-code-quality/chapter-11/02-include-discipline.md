# 11.2 Include Discipline

## 1. The Catastrophe of the Kitchen Sink
As discussed in the previous section, the C preprocessor literally copies and pastes the text of included files. In a mature embedded project with hundreds of source files and complex hardware abstraction layers (HALs), poor include discipline leads to an exponential explosion in compilation time and an unmanageable web of dependencies.

The most notorious architectural failure in embedded C is the "Kitchen Sink" header file—often named `includes.h`, `globals.h`, or `system_types.h`. 

#### Concrete Anti-Pattern: The `includes.h` Megalith
Junior engineers, frustrated by constantly tracking down missing `#include` directives, often create a single header file that includes everything in the project. They then `#include "includes.h"` at the top of every `.c` file.

```c
/* ANTI-PATTERN: The "Kitchen Sink" Include File */
// file: project_includes.h
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_hal.h"
#include "uart_driver.h"
#include "spi_driver.h"
#include "i2c_driver.h"
#include "motor_control.h"
#include "ui_display.h"
```

**The Deep Technical Rationale:**
If you change one line of code in `motor_control.h` (perhaps adding a new enum value), you change the text of `motor_control.h`. Because `project_includes.h` includes it, the text of `project_includes.h` changes. Because every single `.c` file in your project includes `project_includes.h`, **the entire project must be recompiled from scratch.** 

What should have been a 200-millisecond incremental build of a single motor control file turns into a 5-minute full project rebuild. Over a year of development across a 10-person team, this costs thousands of hours of lost engineering time. Furthermore, it creates a massive circular dependency nightmare where the UI layer accidentally gains access to the low-level SPI registers.

### 1.2 Include Discipline: Include Only What You Use
The foundational rule of Include Discipline is: **A file must only include the headers that define the types and functions it explicitly uses.** Nothing more, nothing less. 

If `motor_control.c` uses `stdint.h` for `uint32_t` and `pwm_driver.h` to set the duty cycle, it includes those two files. It does *not* include `uart_driver.h` just because the rest of the system uses it.

### 1.3 Forward Declarations vs. `#include`
To minimize dependency trees, we must exploit a fundamental mechanism of the C compiler regarding memory size.

When the compiler parses a header file and sees a function prototype like this:
`void update_display(display_context_t context);`

The compiler must know the exact memory size of `display_context_t` because it needs to know how many bytes to push onto the stack (or place in CPU registers) to pass the parameter by value. To know the size, the compiler must see the full `struct` definition. Therefore, you **must** `#include "display.h"`.

However, if the function takes a pointer:
`void update_display(display_context_t *context);`

The compiler does *not* need to know the size of the struct. It only needs to know the size of a pointer (which is universally 4 bytes on a 32-bit ARM Cortex-M). Therefore, you do **not** need to include `display.h`. You only need to tell the compiler that the type exists using a forward declaration.

#### The Production-Grade Solution: Breaking the Chain
```c
/* PRODUCTION-GRADE: Using Forward Declarations to break include chains */
// file: system_monitor.h
#pragma once

#include <stdint.h>
// BAD: #include "power_supply.h" // Do not include this!
// BAD: #include "network_stack.h" // Do not include this!

// 1. Forward Declarations. We tell the compiler these structs exist.
struct power_supply_ctx; 
struct network_socket;

// 2. We use pointers in our interface. The compiler only needs to know
// that these are pointers (4 bytes). It doesn't need to know the internals.
void monitor_init(void);
void monitor_check_power(struct power_supply_ctx *pwr);
void monitor_send_alert(struct network_socket *sock, uint32_t error_code);
```

By using forward declarations in the header file, `system_monitor.h` does not depend on `power_supply.h` or `network_stack.h`. If the internal layout of the network socket changes, `system_monitor.c` does not need to be recompiled (unless it actually dereferences the struct internally, in which case `system_monitor.c` will `#include "network_stack.h"`, but the dependency is isolated to the `.c` file, not exposed via the `.h` file).

### 1.4 Ordering of Includes
The order in which you `#include` files matters deeply for verifying that your headers are self-contained (Rule 11.1.4). If `uart.h` accidentally relies on `<stdint.h>` but forgets to include it, it will compile successfully *if* the `main.c` file includes `<stdint.h>` *before* including `uart.h`. 

To force the compiler to prove that every header is self-contained, you must order your includes systematically in your `.c` files.

**The Production-Grade Ordering Convention:**
Inside `my_module.c`, the includes must appear in this strict order:
1.  The corresponding header for this module: `#include "my_module.h"` (This guarantees the header is self-contained).
2.  A blank line.
3.  Standard C library headers: `#include <stdint.h>`, `#include <string.h>`
4.  A blank line.
5.  Third-party library/RTOS headers: `#include "FreeRTOS.h"`
6.  A blank line.
7.  Other project-specific headers required by the implementation: `#include "hardware_timer.h"`

## 2. Company Standard Rules

**Rule 11.2.1:** The creation or use of "Kitchen Sink" header files (e.g., `includes.h`, `globals.h`) that agglomerate multiple unrelated headers into a single inclusion point is strictly forbidden. 
**Rule 11.2.2:** A header file MUST only `#include` the minimum set of header files strictly necessary to compile its own interface. It MUST NOT include headers solely for the benefit of the corresponding `.c` file or other modules.
**Rule 11.2.3:** Where a header file only references a pointer to a struct (and does not dereference it or pass it by value), the header MUST use a forward declaration (`struct my_type;`) instead of `#include`-ing the header file that defines the struct. This breaks dependency chains.
**Rule 11.2.4:** Inside a `.c` file, the very first `#include` directive MUST be the corresponding `.h` file for that module. This mechanically proves to the compiler that the header file is entirely self-contained and not secretly relying on previously included files.
**Rule 11.2.5:** Include directives MUST be grouped logically (Corresponding Header, System C Libs, 3rd Party/RTOS, Internal Project Headers) and sorted alphabetically within their groups to prevent merge conflicts and aid readability.