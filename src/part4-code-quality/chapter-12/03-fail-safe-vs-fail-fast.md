# 12.3 Fail-Safe vs. Fail-Fast

## 1. The Philosophy of Catastrophe
When a critical error occurs in an embedded system—one that cannot be resolved through retries, or one triggered by an Assertion (a Fatal Logic Defect)—the architect must decide exactly how the system will react in the milliseconds before it loses control. 

This decision is governed by two competing philosophies: **Fail-Fast** and **Fail-Safe**. Choosing the wrong philosophy for your specific hardware domain can lead to destroyed equipment, safety hazards, or unacceptable system downtime.

### 1.1 Fail-Fast: The Reset Paradigm
The Fail-Fast philosophy argues that the moment the software detects a corrupted state (an assertion failure, a corrupted linked list, a HardFault), the safest action is to immediately destroy the current state and start over.

**The Domain:** Fail-Fast is the standard architecture for consumer electronics, IoT devices, network routers, and non-critical industrial sensors. If your smart thermostat encounters a null pointer, the user would much rather it reboot in 2 seconds than freeze indefinitely while trying to gracefully recover a corrupted memory pool.

**The Implementation:**
A true Fail-Fast architecture relies heavily on the Hardware Watchdog Timer (WDT) and the NVIC (Nested Vectored Interrupt Controller) system reset.

1.  **Immediate Logging:** The system quickly writes the fault reason (e.g., the Program Counter) to a non-volatile memory or a retained SRAM section.
2.  **Immediate Reset:** The system triggers a software reset (e.g., `NVIC_SystemReset()` on ARM Cortex-M) or simply enters an infinite loop and lets the hardware watchdog expire.

#### Concrete Anti-Pattern: The Silent Hang
The most dangerous anti-pattern in a Fail-Fast system is the "Silent Hang." This occurs when a developer writes an error handler that enters an infinite loop *without* configuring the watchdog timer or disabling the hardware outputs.

```c
/* ANTI-PATTERN: The Silent Hang */
void assert_failed(uint8_t* file, uint32_t line) {
    // We detected a bug!
    printf("Assert failed at %s:%d\n", file, line);
    
    // The developer thinks this is safe. It is not.
    // If a motor PWM was active, it will continue spinning forever!
    // If a heater was on, it will start a fire!
    while(1) {
        // System is frozen, hardware is still active.
    }
}
```

### 1.2 Fail-Safe: The Graceful Degradation Paradigm
The Fail-Safe philosophy argues that rebooting the system is sometimes more dangerous than running in a degraded state. If a system is in the middle of a critical physical operation, yanking the power to the CPU might cause catastrophic mechanical failure.

**The Domain:** Fail-Safe is mandated in medical devices, aviation, automotive braking systems (ISO 26262), and high-power motor control. If the infotainment system in a car crashes, it should Fail-Fast. If the Anti-lock Braking System (ABS) detects a sensor fault, it must Fail-Safe: it cannot reboot while the driver is braking; instead, it disables the ABS feature and reverts to standard hydraulic braking, illuminating a warning light.

**The Implementation:**
Fail-Safe architectures require deep integration between the hardware electronics and the software state machine.
1.  **The Safe State:** The hardware MUST be designed with default physical pull-down/pull-up resistors so that if the microcontroller pins go high-impedance (floating), the motors, lasers, or heaters physically turn off.
2.  **Graceful Degradation:** The software isolates the failing subsystem and attempts to continue executing the critical core loops.

#### Production-Grade C Example: The Fail-Safe State Machine
In a Fail-Safe architecture, the software must explicitly force the hardware into a known "Safe State" before making any decisions about resetting.

```c
/* PRODUCTION-GRADE: The Fail-Safe Handler */
#include "hardware_motors.h"
#include "hardware_heaters.h"
#include "system_alerts.h"

typedef enum {
    STATE_NORMAL_OPERATION,
    STATE_DEGRADED_MODE,
    STATE_CRITICAL_FAULT_SAFE
} system_state_t;

static system_state_t current_state = STATE_NORMAL_OPERATION;

// This function is called when a non-fatal but severe hardware fault occurs
// (e.g., one of the two redundant temperature sensors dies).
void handle_subsystem_failure(fault_code_t fault) {
    if (fault == FAULT_TEMP_SENSOR_PRIMARY_DEAD) {
        // We can survive this. We have a backup sensor.
        // We degrade gracefully. We do NOT reset.
        current_state = STATE_DEGRADED_MODE;
        system_alerts_turn_on_warning_led();
        log_fault_to_eeprom(fault);
        
        // Switch control loop to the secondary sensor
        thermal_control_use_backup_sensor(); 
    } 
    else if (fault == FAULT_MOTOR_OVERCURRENT) {
        // This is a critical physical fault. We must enter the Safe State immediately.
        current_state = STATE_CRITICAL_FAULT_SAFE;
        
        // 1. Physically disable the dangerous hardware.
        // This MUST be the very first action taken.
        hardware_motors_emergency_stop();
        hardware_heaters_disable();
        
        // 2. Alert the user/network.
        system_alerts_sound_alarm();
        log_fault_to_eeprom(fault);
        
        // 3. We do NOT reboot. We sit in the safe state and wait for human intervention.
        // If we rebooted, the system might try to spin the motor again, causing a fire.
        while (current_state == STATE_CRITICAL_FAULT_SAFE) {
            // Pet the watchdog so we don't reset, but refuse to execute normal logic.
            hardware_watchdog_feed();
            system_alerts_blink_sos();
        }
    }
}
```

## 2. The Architectural Watchdog
The Hardware Watchdog Timer (WDT) is the ultimate arbiter of system health. It is a dedicated silicon timer that counts down independently of the CPU core. If the software does not explicitly "feed" (reset) the timer before it reaches zero, the WDT physically pulls the CPU reset line, forcing a hard reboot.

**The Architectural Rule of the Watchdog:**
The watchdog MUST NOT be fed inside a hardware timer interrupt (ISR). If the main `while(1)` loop freezes because of a deadlock, the timer interrupt will continue firing in the background. If the ISR feeds the watchdog, the system will remain deadlocked forever, and the watchdog will never bite. 

The watchdog must be fed at the absolute lowest priority level of the system—typically at the bottom of the main `while(1)` loop, or in the idle task of an RTOS. This guarantees that if *any* higher-priority task or interrupt hangs the system, the watchdog will expire and reset the hardware.

## 3. Company Standard Rules

**Rule 12.3.1:** The Principal Architect MUST explicitly define whether the project operates under a Fail-Fast or Fail-Safe paradigm during the initial design phase. This decision dictates the behavior of the `ASSERT()` macro and all fault handlers.
**Rule 12.3.2:** In a Fail-Fast system, any Fatal Logic Defect (Assertion) MUST trigger an immediate software reset (`NVIC_SystemReset()`) after logging the fault. The system MUST NOT enter an infinite loop without the hardware watchdog enabled.
**Rule 12.3.3:** In a Fail-Safe system, any critical fault MUST immediately trigger the "Safe State" function, which mechanically and electrically disables all hazardous actuators (motors, heaters, high-voltage relays) before deciding whether to halt or degrade gracefully.
**Rule 12.3.4:** The Hardware Watchdog Timer (WDT) MUST be enabled in all production builds. It MUST be fed exclusively from the main execution thread or the RTOS idle task. Feeding the watchdog from within an Interrupt Service Routine (ISR) is strictly prohibited.