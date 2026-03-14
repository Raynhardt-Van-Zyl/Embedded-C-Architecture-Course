# Shared Data and Concurrency

The most insidious bugs in embedded software are concurrency bugs. These occur when two different execution threads (e.g., the Main Loop and an ISR, or two different RTOS tasks) attempt to access the same memory location simultaneously. Without an RTOS Mutex to protect the data, bare-metal developers must rely on silicon-level atomic operations.

## 1. Deep Technical Rationale: The Read-Modify-Write (RMW) Bug

To understand concurrency, we must examine assembly code. Consider a simple global counter:

```c
volatile uint32_t g_event_counter = 0;

// Main Loop calls this
void increment_counter(void) {
    g_event_counter++;
}
```

To a C programmer, `g_event_counter++` looks like a single, atomic operation. It is not. The compiler translates this into three distinct assembly instructions (Load, Add, Store):

```assembly
LDR R0, [g_event_counter]   ; 1. READ the value from RAM into Register 0
ADD R0, R0, #1              ; 2. MODIFY Register 0 by adding 1
STR R0, [g_event_counter]   ; 3. WRITE Register 0 back to RAM
```

**The Concurrency Disaster:**
Imagine the counter is `5`. The Main Loop executes the `LDR` instruction. `R0` now holds `5`. 
Suddenly, a Timer ISR fires. The CPU jumps to the ISR. 
The ISR *also* calls `increment_counter()`. It loads `5` into its register, adds `1`, and stores `6` to RAM. The ISR exits.
The Main Loop resumes exactly where it left off. It executes the `ADD` instruction (adding 1 to its `R0`, which still holds `5`). It then executes `STR`, storing `6` to RAM.

The counter was incremented twice, but the value changed from 5 to 6. We lost an event. This is the classic Read-Modify-Write (RMW) bug.

## 2. Production-Grade Solutions

To safely share data between an ISR and a main loop, we must guarantee that the Read, Modify, and Write sequence cannot be interrupted. We have three architectural solutions.

### 2.1 Solution 1: Interrupt Masking (The Critical Section)

The most robust and universally applicable solution is to temporarily disable interrupts around the RMW operation. 

```c
#include "core_cm4.h"

volatile uint32_t g_event_counter = 0;

void safe_increment(void) {
    // 1. Disable global interrupts. 
    // This executes the 'CPSID i' assembly instruction.
    __disable_irq(); 
    
    // 2. Perform the vulnerable RMW operation
    g_event_counter++;
    
    // 3. Re-enable global interrupts ('CPSIE i')
    __enable_irq();
}
```
**Warning:** While this guarantees atomicity, it increases interrupt latency (jitter). If a critical UART byte arrives while interrupts are disabled, it might be missed. Critical sections must be as short as humanly possible (a few instructions).

### 2.2 Solution 2: Bit-Banding (Hardware Atomic Bit Manipulation)

If the shared data is just a boolean flag or a single bit in a register, ARM Cortex-M3 and M4 processors offer a hardware feature called **Bit-Banding**. 

Bit-banding maps a single bit in a specific RAM or Peripheral region to a full 32-bit word in a separate "alias" memory region. Writing `1` or `0` to that 32-bit alias word automatically performs an atomic read-modify-write on the original bit in hardware.

```c
// Example: Atomically setting Bit 3 of a 32-bit variable at address 0x20000000
#define BITBAND_SRAM_BASE  0x22000000 // Alias region base
#define TARGET_ADDRESS     0x20000000 // Original variable address
#define BIT_NUMBER         3

// Calculate the alias address for the specific bit
#define ALIAS_ADDRESS (BITBAND_SRAM_BASE + ((TARGET_ADDRESS - 0x20000000) * 32) + (BIT_NUMBER * 4))

void atomic_set_flag(void) {
    // A single 32-bit store instruction (STR). 
    // The hardware handles the atomic bit modification. No interrupts disabled!
    *((volatile uint32_t *)ALIAS_ADDRESS) = 1; 
}
```
*(Note: Bit-banding was removed in Cortex-M7 and Cortex-M23/M33 in favor of LDREX/STREX).*

### 2.3 Solution 3: LDREX and STREX (Load/Store Exclusive)

Modern ARM architectures provide special instructions for lock-free atomic operations: `LDREX` (Load Register Exclusive) and `STREX` (Store Register Exclusive). 

`LDREX` reads a value and tags the memory address. `STREX` attempts to write a new value back. If an interrupt fired between the load and the store, the hardware clears the tag, and the `STREX` fails (returning a non-zero status). We simply put this in a loop until it succeeds.

```c
#include "core_cm4.h"

volatile uint32_t g_event_counter = 0;

void lock_free_increment(void) {
    uint32_t current_val;
    uint32_t status;

    do {
        // Exclusive Load: Read value and tag the memory address
        current_val = __LDREXW(&g_event_counter);
        
        // Modify
        current_val++;
        
        // Exclusive Store: Attempt to write back. 
        // If an ISR preempted us, status will be non-zero.
        status = __STREXW(current_val, &g_event_counter);
        
    } while (status != 0); // Loop until the write succeeds atomically
}
```
This is the holy grail: atomic RMW without ever disabling interrupts.

## 3. Concrete Anti-Patterns

### Anti-Pattern 1: The Bitwise OR on Peripheral Registers

A massive trap for junior developers is configuring peripheral registers outside of initialization. 

```c
// [ANTI-PATTERN] Silent corruption of peripheral registers
void toggle_led(void) {
    // This is an RMW operation!
    // If an ISR fires here and modifies GPIOA->ODR, the ISR's change 
    // will be overwritten and lost when this line finishes executing.
    GPIOA->ODR ^= (1 << 5); 
}
```

**The Fix:** Modern microcontrollers provide atomic Set/Clear registers (e.g., `BSRR` on STM32) specifically to avoid this bug.

```c
// [CORRECT] Atomic hardware write
void toggle_led(void) {
    // Writing a 1 to the Set/Reset register is a single hardware operation.
    // It is immune to RMW bugs.
    GPIOA->BSRR = (1 << 5); 
}
```

## 4. Company Standard Rules: Concurrency and Shared Data

1. **RULE-CON-01**: **Protect RMW Operations:** Any variable (larger than a single native machine word) that is written by an ISR and read by the main loop, OR modified by both, MUST be protected against Read-Modify-Write corruption.
2. **RULE-CON-02**: **Short Critical Sections:** When using interrupt masking (`__disable_irq()`) to create a critical section, the section SHALL NOT contain loops, function calls, or blocking delays. It must execute in less than 20 clock cycles.
3. **RULE-CON-03**: **Prefer Exclusive Access:** On architectures supporting it (Cortex-M3 and above), lock-free exclusive access instructions (`LDREX`/`STREX` via CMSIS intrinsics) SHOULD be used instead of global interrupt masking for simple counter increments and bit manipulations.
4. **RULE-CON-04**: **Hardware Set/Clear Registers:** Modifying GPIO or peripheral states during runtime MUST utilize the hardware's atomic Set/Clear registers (e.g., `BSRR`) rather than bitwise RMW operations (`|=`, `&=`) on the data registers.