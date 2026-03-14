# Standardizing Task Design

A professional codebase should not look like it was written by five different engineers. Every RTOS task must follow a canonical, predictable structure. This predictability is what allows Principal Engineers to quickly audit code, analyze stack usage, and verify watchdog safety.

## 1. Deep Technical Rationale: The Canonical Task Structure

An RTOS task is functionally equivalent to the `main()` function in a bare-metal program, but operating within its own isolated context. It has an initialization phase (which runs exactly once) and an infinite loop phase.

### 1.1 The Initialization Phase

When `vTaskStartScheduler()` is called, all created tasks are placed in the "Ready" state. The highest priority task executes first. 
The initialization phase of a task must execute rapidly. It is the correct place to initialize local state variables, hardware specific to that task (if not done in `main`), and pre-fill Queues if necessary.

**Crucially, the initialization phase must not block.** If a high-priority task calls `xQueueReceive` during its initialization, it blocks immediately, handing control to a lower-priority task before the lower-priority task has finished its own initialization. This leads to chaotic boot sequences.

### 1.2 The Event-Driven Loop

The infinite loop must be fundamentally **Event-Driven**. A task must spend 99.9% of its life in the Blocked state, consuming zero CPU cycles, waiting for the RTOS to wake it up.

The standard trigger mechanism is an RTOS Queue or a Task Notification.

## 2. Production-Grade C Example: The Company Standard Task

This is the exact template that should be enforced across the entire engineering department.

```c
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Statically allocated structures
#define SENSOR_TASK_STACK_SIZE 256
static StackType_t xSensorStack[SENSOR_TASK_STACK_SIZE];
static StaticTask_t xSensorTCB;

#define QUEUE_LENGTH 10
static StaticQueue_t xSensorQueueStruct;
static uint8_t ucSensorQueueStorage[QUEUE_LENGTH * sizeof(sensor_event_t)];
QueueHandle_t xSensorQueue;

// The canonical task function
void vSensorTask(void *pvParameters) {
    // ---------------------------------------------------------
    // 1. INITIALIZATION PHASE (Runs Once)
    // ---------------------------------------------------------
    sensor_hardware_init();
    sensor_event_t current_event;
    
    // Register with the software watchdog system (explained below)
    watchdog_register_task(TASK_ID_SENSOR);

    // ---------------------------------------------------------
    // 2. INFINITE EVENT LOOP
    // ---------------------------------------------------------
    while (1) {
        // Block, waiting for an event. 
        // Use a 500ms timeout so the task wakes up periodically 
        // even if no events occur, allowing it to pet the watchdog.
        BaseType_t xStatus = xQueueReceive(
            xSensorQueue, 
            &current_event, 
            pdMS_TO_TICKS(500) // The "Tickle" timeout
        );

        if (xStatus == pdPASS) {
            // -----------------------------------------------------
            // 3. EVENT PROCESSING PHASE
            // -----------------------------------------------------
            switch (current_event.type) {
                case EVENT_DATA_READY:
                    process_sensor_data(current_event.payload);
                    break;
                case EVENT_FAULT:
                    handle_sensor_fault();
                    break;
                default:
                    break;
            }
        } else {
            // -----------------------------------------------------
            // 4. TIMEOUT PHASE (No events received in 500ms)
            // -----------------------------------------------------
            // This is completely normal. We use this wake-up to 
            // prove the task is not deadlocked.
        }

        // ---------------------------------------------------------
        // 5. HOUSEKEEPING (Runs every loop iteration)
        // ---------------------------------------------------------
        // Tell the supervisor task that we are still alive and looping.
        watchdog_check_in(TASK_ID_SENSOR);
    }
}
```

## 3. Deep Dive: RTOS Watchdog Integration

In a superloop, you pet the hardware watchdog in the main `while(1)`. 
In an RTOS, if you pet the hardware watchdog in Task A, but Task B is deadlocked, the hardware watchdog will never trigger because Task A is happily keeping it alive.

**The Solution: The Supervisor Task.**
1. You create a dedicated `Supervisor_Task` at the highest priority.
2. The `Supervisor_Task` is the *only* entity allowed to pet the physical hardware watchdog.
3. Every other task in the system must "check in" (by setting a bit in an Event Group) at least once per second.
4. The `Supervisor_Task` wakes up every 1 second. It checks if ALL registered tasks have checked in. If they have, it clears the flags and pets the hardware watchdog.
5. If Task B deadlocks, it misses its check-in. The `Supervisor_Task` detects this, logs the failure to Flash memory, and deliberately allows the hardware watchdog to reset the microcontroller.

This is why the `pdMS_TO_TICKS(500)` timeout in the Queue read is mandatory. Even if the sensor is completely silent for hours, the task must wake up every 500ms to check in with the Supervisor, proving its thread has not crashed or deadlocked.

## 4. Concrete Anti-Patterns

### Anti-Pattern 1: The "portMAX_DELAY" Black Hole

If a task uses `portMAX_DELAY` on a Queue, and the producer of that queue stops sending data (e.g., a hardware wire breaks), the task blocks forever. The Supervisor watchdog will flag the task as dead and reset the entire device, even though the task is technically "healthy" but just waiting.

**The Fix:** Tasks must always use finite block times to guarantee they maintain their heartbeat.

### Anti-Pattern 2: The Return from Task

A bare-metal `main()` function can theoretically return (into an infinite loop trap). An RTOS task function **MUST NEVER RETURN**.

```c
// [ANTI-PATTERN] Returning from a task
void BadTask(void *pvParams) {
    init();
    do_one_thing();
    // FATAL: The function hits the closing brace and returns!
    // The CPU attempts to pop the return address off the stack, 
    // but there is no caller. The RTOS kernel crashes instantly.
}
```
**The Fix:** If a task has truly finished its work and should never run again, it must explicitly delete itself using `vTaskDelete(NULL)`.

## 5. Company Standard Rules: Task Design

1. **RULE-TPL-01**: **The Canonical Structure:** All RTOS tasks MUST implement the canonical three-phase structure: Initialization, Event-Driven Loop, and Housekeeping/Heartbeat.
2. **RULE-TPL-02**: **No Task Returns:** Task functions SHALL NOT exit or return. If a task's lifecycle is complete, it must call `vTaskDelete(NULL)` or enter an infinite `vTaskDelay` loop.
3. **RULE-TPL-03**: **Finite Blocking:** Tasks SHALL NOT block indefinitely (`portMAX_DELAY`) on RTOS primitives. A finite timeout MUST be used to guarantee periodic execution of the task's housekeeping and watchdog check-in routines.
4. **RULE-TPL-04**: **Supervisor Watchdog:** The physical hardware watchdog MUST be serviced by a single, high-priority Supervisor Task. Worker tasks MUST NOT pet the hardware watchdog directly; they must report their health to the Supervisor.
5. **RULE-TPL-05**: **Non-Blocking Initialization:** The initialization phase of a task, executed prior to entering the infinite loop, SHALL NOT invoke blocking RTOS APIs (e.g., `vTaskDelay`, waiting on empty queues).

---

## 6. Reference Implementation

See the complete, production-ready RTOS task templates and implementations:
- **Task Template Header:** [`code/part3-execution-models/rtos_patterns/rtos_task_template.h`](../../../code/part3-execution-models/rtos_patterns/rtos_task_template.h)
- **Task Template Implementation:** [`code/part3-execution-models/rtos_patterns/rtos_task_template.c`](../../../code/part3-execution-models/rtos_patterns/rtos_task_template.c)
- **Thread-Safe Queue Wrapper:** [`code/part3-execution-models/rtos_patterns/safe_queue.h`](../../../code/part3-execution-models/rtos_patterns/safe_queue.h)
- **Complete Network Task Example:** [`code/part7-workshops/rtos_device/task_network.c`](../../../code/part7-workshops/rtos_device/task_network.c)