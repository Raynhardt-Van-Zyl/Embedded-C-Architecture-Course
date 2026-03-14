# Time Management

In bare-metal systems, Time Management is the heartbeat of the architecture. Without an RTOS to magically wake up threads at precise intervals, the application code must maintain a flawless grasp of time. Failing to understand the low-level silicon implementation of time leads to drift, jitter, and the dreaded 49.7-day rollover crash.

## 1. Deep Technical Rationale: The SysTick Timer

On ARM Cortex-M processors, time is standardized through the System Tick Timer (SysTick). This is a 24-bit down-counter built directly into the core silicon (NVIC). It runs off the processor clock and generates an interrupt when it reaches zero.

**Silicon Mechanics:**
If the CPU runs at 100 MHz, we program the SysTick reload register (`SYST_RVR`) to `100,000 - 1`. The counter decrements 100 million times a second. Every 100,000 counts (exactly 1 millisecond), the counter underflows, triggers the `SysTick_Handler` interrupt, and automatically reloads the value from `SYST_RVR`. This hardware auto-reload ensures exactly zero drift over time, even if interrupt latency delays the CPU entering the ISR.

### 1.1 The Global Timebase

The standard pattern is to use the SysTick ISR to increment a volatile 32-bit integer.

```c
volatile uint32_t g_system_ticks_ms = 0;

void SysTick_Handler(void) {
    g_system_ticks_ms++;
}
```

### 1.2 The Read Atomicity Problem
A critical, often-missed compiler detail occurs on 8-bit or 16-bit architectures (like AVR or MSP430). Reading a 32-bit variable requires multiple assembly instructions. 

```assembly
; Reading 32-bit variable on 8-bit AVR
LDS R16, ticks_low
LDS R17, ticks_mid1
; <-- If interrupt fires HERE, ticks_mid2 and ticks_high change!
LDS R18, ticks_mid2
LDS R19, ticks_high
```
If the SysTick interrupt fires halfway through the read, the main loop reads a corrupted, torn value. On 32-bit ARM architectures, a `LDR` instruction reads 32 bits atomically, so this specific bug doesn't occur. However, if using a 64-bit time variable (`uint64_t`) on a 32-bit CPU, read-tearing returns.

**Solution:** Use an atomic read function for variables larger than the CPU's native word size.

```c
uint64_t get_system_time_atomic(void) {
    uint64_t time;
    __disable_irq();     // Disable global interrupts
    time = g_system_time_64;
    __enable_irq();      // Re-enable interrupts
    return time;
}
```

## 2. Production-Grade Examples: Delta Timing

### 2.1 The 49.7 Day Rollover

A 32-bit integer incrementing every 1 millisecond will reach `0xFFFFFFFF` and overflow to `0` after 49.71 days. If timing code is written incorrectly, the device will hang or execute tasks rapidly upon rollover.

**The Golden Rule of Rollover:** Always use subtraction of unsigned integers (`current - previous >= timeout`). 

Because of the rules of two's complement binary arithmetic in C, subtracting unsigned integers works flawlessly across a rollover boundary.

```c
// Correct Non-blocking Delay Implementation
uint32_t start_time = g_system_ticks_ms;
uint32_t timeout_ms = 500;

// Correct: Two's complement handles the rollover
if ((g_system_ticks_ms - start_time) >= timeout_ms) {
    // Timeout reached
}
```

Let's prove it with 8-bit math (rollover at 255):
- `start_time` = 250
- `current_time` rolls over to 5.
- `5 - 250` in 8-bit unsigned math is `256 + 5 - 250 = 11`.
- The delta calculation perfectly represents that 11 ticks have passed!

## 3. Concrete Anti-Patterns

### Anti-Pattern 1: The Absolute Time Comparison

This is the most common time-related bug in embedded systems. It fails catastrophically when `g_system_ticks_ms + timeout` overflows.

```c
// [ANTI-PATTERN] Absolute Target Time
uint32_t target_time = g_system_ticks_ms + 500; // Bug: What if this overflows to 12?

while(1) {
    // If target_time overflowed to 12, but g_system_ticks_ms is 0xFFFFFFFA,
    // this condition is true IMMEDIATELY. The delay is skipped.
    if (g_system_ticks_ms >= target_time) {
        do_work();
    }
}
```

### Anti-Pattern 2: Resetting the Global Tick
Never allow application code to reset the system tick counter to zero.

```c
// [ANTI-PATTERN] Resetting the clock
void restart_sequence(void) {
    // FATAL: Any other task waiting on a timeout just had 
    // its delta calculation corrupted!
    g_system_ticks_ms = 0; 
}
```

## 4. Software Timers Architecture

For a scalable architecture, we implement a Software Timer module. Instead of scattering `start_time` variables everywhere, we use a structured API.

```c
typedef struct {
    uint32_t start_time;
    uint32_t duration;
    bool running;
} sw_timer_t;

void timer_start(sw_timer_t *t, uint32_t duration_ms) {
    t->start_time = g_system_ticks_ms;
    t->duration = duration_ms;
    t->running = true;
}

bool timer_is_expired(sw_timer_t *t) {
    if (!t->running) return false;
    
    // Proper unsigned delta calculation
    if ((g_system_ticks_ms - t->start_time) >= t->duration) {
        t->running = false; // Auto-stop
        return true;
    }
    return false;
}
```

## 5. Company Standard Rules: Time Management

1. **RULE-TM-01**: **Rollover-Safe Arithmetic:** All time comparisons MUST use subtraction of unsigned integers (`current_time - start_time >= timeout`). Absolute time target comparisons (`time >= target`) are strictly prohibited.
2. **RULE-TM-02**: **Volatile Timebase:** Global tick variables incremented in ISRs MUST be declared `volatile` to prevent compiler caching.
3. **RULE-TM-03**: **Read Atomicity:** If the time variable size exceeds the native processor word size (e.g., a 32-bit variable on an 8-bit CPU, or a 64-bit variable on a 32-bit CPU), reads MUST be protected by a critical section (interrupt masking) to prevent read-tearing.
4. **RULE-TM-04**: **Hardware Auto-Reload:** The periodic system tick (e.g., SysTick) MUST rely on hardware auto-reload mechanisms. Software reloading inside the ISR causes cumulative drift and is prohibited.
5. **RULE-TM-05**: **Immutable Timebase:** Application code SHALL NOT reset or modify the global system tick variable. It must only be read by the application and modified by the timer ISR.