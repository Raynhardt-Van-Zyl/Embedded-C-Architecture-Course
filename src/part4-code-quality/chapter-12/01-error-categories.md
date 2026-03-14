# 12.1 Error Categories in Embedded Systems

## 1. The Anatomy of a Firmware Failure
Before an architect can design an error-handling strategy, they must explicitly define what an "error" is. The most pervasive mistake in embedded software engineering is treating all off-nominal behavior with the exact same mechanism. 

If a developer uses a `return ERROR_CODE;` to handle an I2C bus collision, but also uses `return ERROR_CODE;` to handle an out-of-bounds array index, the architecture is fundamentally broken. One is an expected physical reality of the hardware environment; the other is a catastrophic logic bug written by the programmer. 

To build resilient, safety-critical firmware, we must classify errors into three distinct categories: **Expected Anomalies**, **Recoverable Faults**, and **Fatal Logic Defects**.

### 1.1 Category 1: Expected Anomalies (The Environment)
Expected anomalies are events that occur routinely during normal operation. They are not "bugs." They are the physical realities of interacting with the analog world, wireless networks, and human users.

*   **Examples:** A dropped Wi-Fi packet due to interference, a user entering an invalid password on a keypad, an ADC reading fluctuating due to electrical noise, or a momentary loss of GPS lock.
*   **The Architectural Response:** These events must be handled gracefully through normal control flow. They should be resolved immediately at the point of occurrence, usually through retries, filtering, or simple `if/else` logic. They rarely require logging as "errors" unless they cross a statistical threshold (e.g., 10 dropped packets in a row becomes a Recoverable Fault).

### 1.2 Category 2: Recoverable Faults (Hardware & System Level)
Recoverable faults are genuine errors that prevent a specific operation from completing, but they do not corrupt the overall state of the application. They are typically caused by hardware malfunctions, subsystem resets, or external device failures.

*   **Examples:** An I2C EEPROM fails to acknowledge (NACK) a write command after 3 retries, a motor controller reports an over-current condition, or an SD card fails to mount because the file system is corrupted.
*   **The Architectural Response:** These faults cannot be handled by a simple `if` statement locally; they must be propagated up the call stack to a higher-level state machine. The system must explicitly decide how to recover (e.g., reset the I2C peripheral, disable the motor and blink a red LED, or format the SD card). 

These faults are handled exclusively via **Explicit Return Codes**. We use `status_t` enums to meticulously track the failure as it propagates upwards.

### 1.3 Category 3: Fatal Logic Defects (Programmer Error)
Fatal logic defects are bugs. They are impossible situations that only exist because the software engineer made a mathematical or architectural mistake. 

*   **Examples:** A null pointer passed into a function that requires a valid address, an array index that exceeds the buffer size, an unhandled `default` case in a state machine, or dividing by zero.
*   **The Architectural Response:** The system is now in an Undefined State. The memory might be corrupted. The internal variables no longer reflect the physical reality of the machine. **You cannot recover from a logic defect.** If you attempt to use a return code to handle a null pointer (e.g., `if (ptr == NULL) return ERROR;`), you are silently burying a catastrophic bug. The system will continue running, but it is now mathematically compromised, which can lead to runaway motors or silent data corruption.

Fatal logic defects are handled exclusively via **Assertions**. When an assertion fails, the system must immediately halt, log the exact location of the defect (Program Counter, Link Register, File, Line), and safely reboot.

## 2. Design by Contract (DbC) in C
The most effective way to separate Recoverable Faults (Return Codes) from Fatal Logic Defects (Assertions) is to implement **Design by Contract (DbC)**.

DbC treats every function as a mathematical contract between the Caller and the Callee.
1.  **Preconditions:** What must be true *before* the function executes? (e.g., Pointers must not be NULL, lengths must be > 0). The Caller is responsible for guaranteeing preconditions. If a precondition fails, it is a Fatal Logic Defect (Assertion).
2.  **Postconditions:** What must be true *after* the function executes? (e.g., The data was successfully written to hardware). If the hardware fails, the Callee cannot satisfy the postcondition. This is a Recoverable Fault (Return Code).
3.  **Invariants:** What must *always* be true regarding the internal state? (e.g., The linked list count must match the actual number of nodes). If an invariant fails, it is a Fatal Logic Defect (Assertion).

#### Concrete Anti-Pattern: Confusing Contracts
```c
/* ANTI-PATTERN: Burying a Logic Bug with a Return Code */
#include "flash_driver.h"

// The contract says we will write data to flash.
status_t flash_write_data(uint32_t address, const uint8_t *data, uint16_t length) {
    // 1. The developer checks for a NULL pointer (a Precondition violation).
    // Instead of halting the system to fix the bug, they return an error code.
    if (data == NULL) {
        return STATUS_ERROR_INVALID_ARGUMENT; // SILENT FAILURE!
    }
    
    // 2. The developer checks if the hardware actually wrote the data (a Postcondition failure).
    if (hardware_spi_write(address, data, length) != SPI_SUCCESS) {
        return STATUS_ERROR_HARDWARE_FAULT; // Correct use of return code.
    }
    
    return STATUS_OK;
}
```

**The Deep Technical Rationale:** If `data == NULL`, the *Caller* has a severe bug in their logic. Perhaps a malloc failed earlier, or a pointer was corrupted. By returning `STATUS_ERROR_INVALID_ARGUMENT`, the `flash_write_data` function expects the Caller to handle the error. But the Caller is the one who passed the NULL pointer in the first place! The Caller is already compromised. 

If the Caller ignores the return code (which happens frequently), the system continues executing with corrupted logic. The flash write silently failed, and the user's data is lost forever, but the system pretends everything is fine.

**The Production-Grade Solution:**
```c
/* PRODUCTION-GRADE: Enforcing Contracts with Asserts and Return Codes */
#include "flash_driver.h"
#include "system_assert.h"

status_t flash_write_data(uint32_t address, const uint8_t *data, uint16_t length) {
    // 1. Precondition Validation (Fatal Logic Defect).
    // If this fails, the system immediately halts and logs the bug.
    // We force the developer to fix the caller's code before shipping.
    ASSERT(data != NULL);
    ASSERT(length > 0);
    
    // 2. Hardware Operation (Recoverable Fault).
    // The hardware might fail due to physics (EMI, voltage sag).
    // We return an explicit code so the caller can retry or alert the user.
    if (hardware_spi_write(address, data, length) != SPI_SUCCESS) {
        return STATUS_ERROR_HARDWARE_FAULT;
    }
    
    return STATUS_OK;
}
```

## 3. Company Standard Rules

**Rule 12.1.1:** The architecture MUST strictly differentiate between Expected Anomalies, Recoverable Faults, and Fatal Logic Defects. These categories MUST NOT use the same error-handling mechanisms.
**Rule 12.1.2:** Function preconditions (e.g., pointer validation, array index bounds, valid enum ranges) MUST be validated using Assertions (`ASSERT()`). A function MUST NOT return an error code for a precondition violation caused by internal software logic.
**Rule 12.1.3:** Failures originating from the physical environment or hardware peripherals (e.g., I2C NACK, sensor timeout, EEPROM write failure) MUST be propagated using explicit Return Codes (`status_t`). They MUST NOT trigger an Assertion, as this would cause physical hardware anomalies to crash the firmware.
**Rule 12.1.4:** Developers MUST treat Assertions as proof of a logic bug. If an Assertion triggers during testing, the root cause in the caller's logic MUST be fixed. Assertions are not a crutch for poor design; they are a tripwire for broken contracts.