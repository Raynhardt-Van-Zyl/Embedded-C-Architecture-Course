# 12.4 Observability and Diagnostics

## 1. The Blindness of Field Deployment
When an embedded firmware developer is writing code at their desk, they have god-like visibility into the system. They have a $1,000 JTAG debugger connected to the microcontroller. They can set breakpoints, pause the CPU clock, inspect the exact values of SRAM variables, and trace the execution stack back to the original function call.

When that same firmware is deployed inside a pacemaker in a patient's chest, or bolted to an engine block vibrating at 6,000 RPM, that visibility vanishes entirely. If the device spontaneously reboots at 3:00 AM, the engineering team is completely blind. 

A firmware architecture that does not explicitly design for **Observability** is a liability. You cannot fix a bug if you cannot see it. We must build a "Black Box" flight recorder directly into the firmware.

### 1.1 The Crash Log: Preserving the CPU State
When a fatal error occurs—whether it's a failed `ASSERT()` or a CPU HardFault (e.g., an unaligned memory access, a division by zero, or a jump to an invalid memory address)—the system is about to reset. The moment the reset pin goes low, all the volatile data in the CPU registers (R0-R15) is destroyed.

The architecture must intercept the fault *before* the reset occurs, extract the critical CPU registers, and write them to a persistent storage medium (EEPROM, external SPI Flash, or a retained section of SRAM that survives soft resets).

#### Deep Technical Rationale: The ARM Cortex-M HardFault
On an ARM Cortex-M processor, when a catastrophic silicon-level fault occurs, the CPU hardware automatically pushes exactly 8 registers onto the current stack: `R0`, `R1`, `R2`, `R3`, `R12`, `LR` (Link Register), `PC` (Program Counter), and `xPSR` (Program Status Register).

The `PC` tells you exactly which assembly instruction caused the crash. The `LR` tells you which function called the crashing function. This is the Holy Grail of embedded debugging. 

However, you cannot write a HardFault handler in pure C to extract these registers, because the C compiler will push its own registers onto the stack as a prologue, burying the hardware's stacked registers. You must use a naked assembly trampoline.

#### Production-Grade C Example: The HardFault Trampoline
```c
/* PRODUCTION-GRADE: Capturing the ARM Cortex-M HardFault Stack */
// file: hardfault_handler.c

#include <stdint.h>
#include "crash_logger.h"

// 1. The C Handler. It receives a pointer to the stack frame.
// We disable optimizations so the compiler doesn't inline this and ruin the stack alignment.
__attribute__((optimize("O0")))
void hard_fault_handler_c(uint32_t *fault_stack) {
    // The hardware pushes the registers in this exact order:
    uint32_t r0  = fault_stack[0];
    uint32_t r1  = fault_stack[1];
    uint32_t r2  = fault_stack[2];
    uint32_t r3  = fault_stack[3];
    uint32_t r12 = fault_stack[4];
    uint32_t lr  = fault_stack[5]; // The return address
    uint32_t pc  = fault_stack[6]; // THE EXACT INSTRUCTION THAT CRASHED
    uint32_t psr = fault_stack[7];

    // 2. We now have the complete context of the crash!
    // We write this to our Non-Volatile Memory (EEPROM/Flash).
    crash_log_t log;
    log.magic_word = 0xDEADBEEF;
    log.fault_type = FAULT_TYPE_HARDFAULT;
    log.pc = pc;
    log.lr = lr;
    crash_logger_write_to_flash(&log);

    // 3. Now we can safely trigger the system reset.
    NVIC_SystemReset();
}

// 2. The Assembly Trampoline. This is the actual function mapped in the vector table.
// It determines which stack (Main Stack or Process Stack) was in use when the fault 
// occurred, puts that stack pointer into R0, and calls the C function above.
__attribute__((naked)) void HardFault_Handler(void) {
    __asm volatile(
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " b hard_fault_handler_c                                    \n"
    );
}
```

With this architecture, when the device reboots in the field, the bootloader or the main application can check the EEPROM for the `0xDEADBEEF` magic word. If it exists, the device can transmit the `PC` and `LR` values over its cellular or Wi-Fi connection to the company's cloud dashboard. The engineering team can then use `addr2line` (a GCC tool) to map that exact Program Counter address back to the exact line of C code that caused the crash. 

### 1.2 The Retained SRAM Strategy (No-Init)
Writing to external EEPROM or Flash during a HardFault is dangerous. The SPI peripheral might be in a corrupted state, or the interrupts required to run the DMA might be disabled. 

A safer, faster alternative is to use a specific section of the internal SRAM that is *not* initialized to zero by the C startup code. This is called a "No-Init" section. When you trigger a soft reset (`NVIC_SystemReset()`), the contents of the No-Init SRAM remain intact.

```c
/* PRODUCTION-GRADE: The No-Init Crash Log */
// We use a linker script attribute to place this struct in a specific RAM section
// that the startup code (crt0.s) explicitly ignores during the BSS zeroing phase.

__attribute__((section(".noinit"))) crash_log_t system_crash_log;

void log_crash_and_reset(uint32_t program_counter, uint32_t link_register) {
    system_crash_log.magic = 0xDEADBEEF;
    system_crash_log.pc = program_counter;
    system_crash_log.lr = link_register;
    
    // Reset the CPU. The SRAM retains the data!
    NVIC_SystemReset(); 
}

void main(void) {
    // Check upon boot if we just recovered from a crash
    if (system_crash_log.magic == 0xDEADBEEF) {
        transmit_telemetry_to_cloud(&system_crash_log);
        
        // Clear the magic word so we don't transmit it again on a normal reboot
        system_crash_log.magic = 0; 
    }
    
    // Normal startup...
}
```

### 1.3 Telemetry and Post-Mortem Debugging
Observability extends beyond crashes. A production device must continuously monitor its own health metrics (e.g., Stack High-Water Marks, heap fragmentation, CPU temperature, I2C bus error counters). These metrics should be periodically saved or transmitted. 

If a device is returned from the field via an RMA (Return Merchandise Authorization), the engineer should be able to plug it in and immediately dump the flight recorder, retrieving the history of the device's final hours.

## 2. Company Standard Rules

**Rule 12.4.1:** All production firmware MUST implement a global fault handler (e.g., HardFault, NMI, MemManage) that intercepts catastrophic silicon faults before the system resets.
**Rule 12.4.2:** The fault handler MUST extract the Program Counter (PC) and Link Register (LR) from the hardware stack and save them to a persistent storage medium (EEPROM, SPI Flash, or a strictly defined `.noinit` SRAM section) to create a Post-Mortem Crash Log.
**Rule 12.4.3:** The system boot sequence MUST check for the presence of a Crash Log in persistent storage. If a log exists, the system MUST attempt to transmit the log to a diagnostic interface (UART, BLE, Cloud API) before clearing the flag and resuming normal operation.
**Rule 12.4.4:** The system MUST track and periodically log the "High-Water Mark" of all RTOS task stacks to detect impending stack overflows before they corrupt memory.
**Rule 12.4.5:** The use of blocking functions (e.g., polling UART `printf`) inside a HardFault or Assertion handler is strictly forbidden if it relies on interrupts, as interrupts are disabled during a fault. The diagnostic logging mechanism MUST use synchronous, polling-based hardware writes or SRAM retention.