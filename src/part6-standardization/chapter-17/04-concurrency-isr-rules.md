# 17.4 Concurrency and ISR rules

In embedded systems, concurrency bugs—race conditions, deadlocks, priority inversions, and corrupted shared memory—are the most difficult bugs to track down. They often manifest sporadically, only under heavy load, and disappear entirely when stepping through code with a debugger.

Because the stakes are so high, the framework must be incredibly strict about how Interrupt Service Routines (ISRs) and Real-Time Operating System (RTOS) tasks interact. "Cowboy coding" in interrupts cannot be tolerated.

## Rule 1: The "Naked Shared State" Ban

The most common embedded bug is two threads (or a thread and an ISR) accessing a global variable without protection.

The framework must dictate: **No global variables may be read or modified by multiple contexts without explicit synchronization primitives.**

If a variable must be shared, it must be protected by:
*   A Mutex (if shared between two RTOS Tasks).
*   A Critical Section / Interrupt Disable (if shared between a Task and an ISR).
*   Atomic operations (if supported by the architecture).

Better yet, the framework should encourage avoiding shared state entirely in favor of message passing.

## Rule 2: Standardized ISR-to-Task Communication

ISRs must be as short and fast as possible. An ISR should never poll, never delay, and never execute complex business logic. Its sole purpose is to acknowledge the hardware trigger, capture the immediate data, and hand off processing to a background task.

The framework must standardize exactly how this handoff occurs.

**The Enforced Pattern: RTOS Message Queues**

Instead of using volatile global flags, the framework should mandate that ISRs communicate with tasks using non-blocking, interrupt-safe RTOS Queues or Task Notifications.

```c
// Standardized ISR Pattern

// 1. The hardware interrupt fires
void UART_RX_IRQHandler(void) {
    uint8_t byte_received = UART->DR; // Read hardware register

    // 2. Clear the interrupt flag
    UART->SR &= ~RXNE; 

    // 3. Post data to an RTOS Queue. MUST use the "FromISR" variant.
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(g_uart_rx_queue, &byte_received, &xHigherPriorityTaskWoken);

    // 4. Yield if posting the message woke a higher priority task
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

By standardizing this, developers don't have to invent their own ring buffers or debate how to safely trigger a background task.

## Rule 3: Mutex vs. Semaphore Clarity

A common source of confusion is the difference between a Mutex and a Binary Semaphore. The framework documentation must explicitly define when to use which:

*   **Mutexes are for Resource Protection:** Use a Mutex when multiple tasks need to access a shared hardware resource (e.g., an I2C bus or an SPI display). Mutexes have a concept of "ownership" and often include priority inheritance to prevent priority inversion. An ISR *cannot* take a Mutex.
*   **Semaphores are for Synchronization:** Use a Binary Semaphore to signal an event from an ISR to a Task. The ISR "gives" the semaphore, and the Task is blocked waiting to "take" it. Semaphores do not have ownership.

The framework's OSAL (OS Abstraction Layer) can enforce this by providing separate APIs that don't allow misuse (e.g., an OSAL Mutex API that simply panics if called from within an ISR context).

## Rule 4: Blocking APIs Must Accept Timeouts

A common cause of system lockups is a task waiting forever on a resource that never becomes available. 

The framework must mandate that **every blocking call** (Queue Receive, Mutex Take, HAL Transmit) must include a timeout parameter. 

```c
// Anti-Pattern: Can block forever
sys_mutex_lock(my_mutex, WAIT_FOREVER); 

// Framework Standard: Explicit timeout required
sys_err_t err = sys_mutex_lock(my_mutex, 100); // 100ms timeout
if (err == SYS_ERR_TIMEOUT) {
    LOG_ERROR("Mutex deadlock avoided, executing fallback.");
    // Handle error...
}
```

## Summary

Concurrency rules must be the most rigidly enforced part of your framework. By banning naked shared state, mandating Queue-based ISR handoffs, clarifying OS primitives, and requiring timeouts, you eliminate entire classes of catastrophic field failures before the code even compiles.