# 13.2 Review Checklists for Embedded C

## 1. The Futility of Manual Style Reviews
When establishing a formal Code Review process, the most common and destructive mistake engineering teams make is populating their review checklist with formatting rules. 

If your checklist contains items like:
*   "Are all variables camelCase?"
*   "Is the indentation 4 spaces?"
*   "Are the curly braces on the correct line?"

...you are wasting hundreds of thousands of dollars of engineering time. Humans are terrible at parsing syntax, and machines are perfect at it. The CI/CD pipeline, equipped with `clang-format` and `clang-tidy`, MUST handle all style and superficial correctness checks before the Pull Request is even allowed to be reviewed by a human.

### 1.1 The Human Reviewer's Mandate
The human reviewer is the final, most expensive, and most critical layer of defense. Their checklist must focus exclusively on the things a static analysis tool cannot understand: **Hardware Architecture, Concurrency, Intent, and Error Logic.**

A production-grade embedded C checklist is divided into four critical domains.

## 2. The Embedded C Code Review Checklist

### 2.1 Domain 1: Hardware Interaction & Data Layout
The compiler assumes it is running on a standard Von Neumann architecture with infinite memory. The reviewer must ensure the code respects the physical realities of the microcontroller (alignment, cache coherency, DMA, and peripherals).

*   [ ] **DMA Alignment:** Are buffers passed to a DMA controller aligned to the correct byte boundary (e.g., `__attribute__((aligned(4)))`)? Does the buffer reside in a memory region accessible by the DMA (e.g., some SRAM blocks on STM32 cannot be reached by DMA1)?
*   [ ] **Cache Coherency:** If the CPU has a data cache (Cortex-M7), are cache maintenance operations (`SCB_CleanDCache_by_Addr()`, `SCB_InvalidateDCache_by_Addr()`) performed *before* starting a DMA TX and *after* a DMA RX completes?
*   [ ] **Memory Barriers:** When interacting with complex peripherals, are `__DSB()`, `__DMB()`, or `__ISB()` instructions used to guarantee that memory writes are physically retired to the bus before the CPU executes the next instruction?
*   [ ] **Struct Packing:** If a struct is mapped to a hardware register or a network packet, is it packed correctly using `__attribute__((packed))`? 

#### Deep Technical Rationale: Struct Packing and Unaligned Access
Consider a developer designing a protocol packet.
```c
/* ANTI-PATTERN: The Implicit Padding Trap */
typedef struct {
    uint8_t packet_id;  // 1 byte
    uint32_t payload;   // 4 bytes
} network_packet_t;
```
A novice reviewer might assume `sizeof(network_packet_t)` is 5 bytes. A senior reviewer knows that the compiler will automatically insert 3 bytes of "padding" after `packet_id` to ensure `payload` is 4-byte aligned for the CPU. The struct is actually 8 bytes. If this is transmitted over UART, the receiver gets 3 bytes of garbage memory (a severe security vulnerability) and the payload is shifted.

```c
/* PRODUCTION-GRADE: Explicitly Packed Structs */
// GCC/Clang syntax for packing a struct, eliminating all padding.
typedef struct __attribute__((packed)) {
    uint8_t packet_id;
    uint32_t payload;
} network_packet_t;
```
The reviewer must also verify the *consequence* of packing: `payload` is now unaligned (it sits at offset 1). If the CPU is a Cortex-M0 (no unaligned hardware support), accessing `packet->payload` will trigger a HardFault. The reviewer must ensure the code uses `memcpy` or bitwise shifts to safely extract the unaligned data.

### 2.2 Domain 2: Concurrency & Interrupts (Safety)
The most difficult bugs in embedded systems are race conditions caused by Interrupt Service Routines (ISRs) or RTOS task preemption. 

*   [ ] **The `volatile` Keyword:** Is every variable that is written in an ISR and read in the main loop (or vice versa) marked `volatile`? 
*   [ ] **Atomic Access:** Does the code read or write variables larger than the CPU's native word size (e.g., a `uint64_t` on a 32-bit ARM) without an atomic block (`__disable_irq()` / `__enable_irq()`) or a mutex? If an interrupt fires halfway through reading the 64-bit value, the data is corrupted.
*   [ ] **Blocking in ISRs:** Does the ISR contain any `while` loops, polling delays (`delay_ms(5)`), or heavy math (floating-point division)? ISRs must be microscopic: read the hardware register, set a flag/post an RTOS semaphore, and exit.
*   [ ] **Deadlocks:** If using an RTOS, does the code lock Mutex A and then wait for Mutex B, while another task locks Mutex B and waits for Mutex A?

### 2.3 Domain 3: Error Handling & Contracts (DbC)
*   [ ] **Contract Violations:** Are all preconditions (pointers, array lengths) explicitly verified with `ASSERT()` before use?
*   [ ] **Return Code Propagation:** Are all hardware APIs returning a `status_t` explicitly checked? If a failure occurs, is it logged, and is the system safely degraded or halted?
*   [ ] **Resource Leaks:** If the code uses `malloc()` (highly discouraged) or acquires a Mutex/Semaphore, is it absolutely guaranteed to be freed/released in every single error path?

#### Concrete Anti-Pattern: The Early Return Leak
```c
/* ANTI-PATTERN: The Mutex Leak */
status_t write_to_shared_eeprom(uint8_t *data) {
    xSemaphoreTake(eeprom_mutex, portMAX_DELAY);
    
    if (data == NULL) {
        // FATAL FLAW: The reviewer MUST catch this. The function returns 
        // early without releasing the mutex. The entire system will deadlock
        // the next time any task tries to access the EEPROM.
        return STATUS_ERROR; 
    }
    
    i2c_write(EEPROM_ADDR, data, 10);
    xSemaphoreGive(eeprom_mutex);
    return STATUS_OK;
}
```

### 2.4 Domain 4: Architecture & Decoupling
*   [ ] **The Single Responsibility Principle:** Does the function do exactly what its name implies?
*   [ ] **The Opaque Pointer (PIMPL):** Does the `.h` file hide the internal struct definition?
*   [ ] **Global Variables:** Are there new `extern` global variables? Every new global variable is an architectural smell and should be aggressively challenged. State should be passed via parameters or hidden behind getter/setter APIs.

## 3. Company Standard Rules

**Rule 13.2.1:** Human Code Reviews MUST NOT include comments regarding code formatting, bracket placement, or variable casing. These issues MUST be resolved by the automated CI pipeline before a human review commences.
**Rule 13.2.2:** Every Pull Request MUST be evaluated against the four domains of the Embedded Code Review Checklist: Hardware Interaction, Concurrency, Error Handling, and Architecture.
**Rule 13.2.3:** Any Pull Request that introduces a new global variable (`extern` or exposed file-scope `static`) MUST provide explicit written justification in the PR description, and the reviewer MUST challenge it as a potential architectural defect.
**Rule 13.2.4:** The reviewer MUST explicitly verify the safe extraction of unaligned data from any struct marked `__attribute__((packed))`, particularly when transmitting data across hardware buses (UART, SPI, Ethernet).
**Rule 13.2.5:** The reviewer MUST verify that every single exit path (`return`, `break`, `goto`) within a function safely releases any acquired hardware locks, RTOS mutexes, or dynamically allocated memory.