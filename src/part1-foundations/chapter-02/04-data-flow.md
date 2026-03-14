# 2.4 Data Flow in Embedded Systems

## Managing State and Concurrency

If Control Flow is the skeleton of the application, Data Flow is the circulatory system. How data is stored, shared, and passed between different contexts (ISRs, tasks, modules) is the source of the most insidious, hard-to-reproduce bugs in embedded systems: **Race Conditions** and **Memory Corruption**.

Architecting a safe data flow means establishing strict rules about *who owns* data and *how* it is safely handed off.

### The Problem of Shared State

Whenever two different execution contexts (e.g., main loop and an ISR, or two RTOS tasks) access the same variable, and at least one of them writes to it, you have a critical section.

#### ❌ Anti-Pattern: Unprotected Shared State

```c
// ANTI-PATTERN: Race condition on shared struct
typedef struct {
    uint16_t x;
    uint16_t y;
} SensorData_t;

SensorData_t global_sensor; // Shared state

// Context 1: Runs in an ISR
void Timer_ISR(void) {
    global_sensor.x = ReadHardwareX();
    global_sensor.y = ReadHardwareY();
}

// Context 2: Runs in Main Loop
void App_Task(void) {
    // RACE CONDITION!
    // What if the ISR fires right after reading x, but before reading y?
    ProcessCoordinates(global_sensor.x, global_sensor.y); 
}
```
**Rationale:** The `App_Task` might read an `x` from the previous sensor reading and a `y` from the *new* sensor reading, resulting in corrupted, invalid coordinate pairs being processed.

### Safe Data Flow Mechanisms

To architect a safe system, we must use appropriate data passing mechanisms based on the contexts involved.

#### 1. The `volatile` Keyword
The `volatile` keyword tells the compiler: "This variable can change at any time outside of the current code flow (e.g., by hardware or an ISR). Do not optimize out reads or writes to it."
- **Rule:** Variables shared between an ISR and a background task *must* be declared `volatile`.
- **Warning:** `volatile` does **NOT** guarantee atomicity. It does not protect against the struct race condition shown above. It only prevents compiler optimizations.

#### 2. Atomic Operations & Critical Sections (Bare Metal)
If you must share data in a bare-metal system, you must briefly disable interrupts to read/write the shared data safely.

```c
// GOOD: Using a critical section in bare metal
void App_Task(void) {
    SensorData_t local_copy;
    
    // CRITICAL SECTION START
    __disable_irq(); 
    local_copy.x = global_sensor.x;
    local_copy.y = global_sensor.y;
    __enable_irq();
    // CRITICAL SECTION END

    // Safe to use local_copy now
    ProcessCoordinates(local_copy.x, local_copy.y);
}
```

#### 3. Queues and Ring Buffers (The Best Approach)
The most architecturally sound way to move data is **Pass-by-Value through a Queue**. This eliminates shared state entirely. The producer writes into the queue, and the consumer reads from the queue.

```c
// GOOD: Data flow via Ring Buffer/Queue
#include "ring_buffer.h"

RingBuffer_t sensor_queue;

// Producer (ISR)
void Timer_ISR(void) {
    SensorData_t new_data;
    new_data.x = ReadHardwareX();
    new_data.y = ReadHardwareY();
    
    // Safely push to queue. ISR doesn't care who consumes it.
    RingBuffer_Push(&sensor_queue, &new_data); 
}

// Consumer (App Task)
void App_Task(void) {
    SensorData_t incoming_data;
    
    // Safely pop from queue.
    if (RingBuffer_Pop(&sensor_queue, &incoming_data)) {
        ProcessCoordinates(incoming_data.x, incoming_data.y);
    }
}
```

### Company Standards & Rules

1. **Ban Global Variables for Data Passing:** Global variables MUST NOT be used for ad-hoc data passing between modules. Modules must pass data explicitly via function parameters, queues, or accessor functions (getters/setters).
2. **Encapsulate Critical Sections:** If shared state is unavoidable, the protection mechanism (disabling interrupts or RTOS mutexes) MUST be encapsulated within the module that owns the data. The caller should never be responsible for managing locks.
3. **Producer-Consumer Over Shared State:** Whenever data streams from hardware to the application, it MUST flow through a First-In-First-Out (FIFO) queue or ring buffer. This decouples the timing of the hardware from the timing of the application logic.
4. **Appropriate use of `volatile`:** Any variable accessed within an ISR and evaluated in a polling loop MUST be declared `volatile`.