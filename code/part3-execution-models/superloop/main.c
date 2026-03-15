/**
 * @file main.c
 * @brief Superloop Scheduler Example Application
 * 
 * APPLICATION OVERVIEW:
 * ====================
 * This example demonstrates a complete embedded application using the superloop
 * scheduler. It simulates a simple sensor monitoring and control system with:
 * 
 * - Temperature sensor reading (periodic task)
 * - LED blinking (periodic task)
 * - Button debouncing (periodic task)
 * - Command processing (event-driven task)
 * - Watchdog management (critical task)
 * 
 * This example is designed to run on a generic ARM Cortex-M microcontroller
 * but can be easily adapted to any platform by modifying the hardware
 * abstraction layer functions.
 * 
 * SAFETY DEMONSTRATIONS:
 * - Non-blocking task design
 * - Proper timeout handling
 * - Watchdog integration
 * - Critical section usage
 * - Health status reporting
 * 
 * @author Embedded C Architecture Course
 * @version 1.0
 */

/*============================================================================*/
/*                             INCLUDES                                        */
/*============================================================================*/

#include "superloop.h"
#include <stdio.h>
#include <string.h>

/*============================================================================*/
/*                             HARDWARE ABSTRACTION                            */
/*============================================================================*/

/**
 * HARDWARE ABSTRACTION LAYER (HAL)
 * ================================
 * In a real application, these functions would be implemented in separate
 * HAL files and would interface with actual hardware registers. For this
 * example, we provide simulated implementations that can run on a PC.
 * 
 * When porting to real hardware:
 * 1. Replace simulated data with actual register reads
 * 2. Add proper interrupt handling
 * 3. Configure clock and peripheral initialization
 */

/**
 * @brief Simulated system tick counter
 * 
 * DESIGN NOTE: In a real system, this would be a hardware timer that
 * generates periodic interrupts. The ISR would increment this counter.
 * The volatile qualifier is essential for correct behavior with ISRs.
 */
static volatile uint32_t g_simulatedTick = 0U;

/**
 * @brief Simulated microsecond timer
 */
static volatile uint32_t g_simulatedUs = 0U;

/**
 * @brief Simulated LED state
 */
static bool g_ledState = false;

/**
 * @brief Simulated button state (true = pressed)
 */
static volatile bool g_buttonState = false;
static volatile bool g_buttonStateChanged = false;

/**
 * @brief Simulated temperature value
 */
static volatile int16_t g_temperature = 25; /* Degrees Celsius */

/**
 * @brief Watchdog fed counter (for debugging)
 */
static uint32_t g_watchdogFedCount = 0U;

/*============================================================================*/
/*                             SIMULATED HAL FUNCTIONS                         */
/*============================================================================*/

/**
 * @brief Get system tick count (simulated)
 */
static uint32_t HalGetTick(void)
{
    return g_simulatedTick;
}

/**
 * @brief Get microsecond timer value (simulated)
 */
static uint32_t HalGetMicrosecond(void)
{
    return g_simulatedUs;
}

/**
 * @brief Feed the watchdog (simulated)
 */
static void HalFeedWatchdog(void)
{
    g_watchdogFedCount++;
    /* In real hardware: WDT->KR = WDT_KEY_REFRESH; */
}

/**
 * @brief Set LED state (simulated)
 */
static void HalSetLed(bool state)
{
    g_ledState = state;
    /* In real hardware: GPIO->ODR = (state) ? (GPIO->ODR | LED_PIN) : (GPIO->ODR & ~LED_PIN); */
}

/**
 * @brief Read button state (simulated)
 */
static bool HalReadButton(void)
{
    return g_buttonState;
}

/**
 * @brief Read temperature sensor (simulated)
 */
static int16_t HalReadTemperature(void)
{
    /* Simulate temperature fluctuation */
    static int16_t tempOffset = 0;
    tempOffset = (tempOffset + 1) % 10;
    return g_temperature + (tempOffset - 5);
}

/**
 * @brief Initialize hardware (simulated)
 */
static void HalInit(void)
{
    g_simulatedTick = 0U;
    g_simulatedUs = 0U;
    g_ledState = false;
    g_buttonState = false;
    g_buttonStateChanged = false;
    g_temperature = 25;
    g_watchdogFedCount = 0U;
}

/**
 * @brief Simulate time passing (for testing on PC)
 */
static void HalSimulateTick(void)
{
    g_simulatedTick++;
    g_simulatedUs += 1000U; /* 1ms = 1000us */
}

/*============================================================================*/
/*                             APPLICATION TASKS                               */
/*============================================================================*/

/**
 * TASK DESIGN PRINCIPLES
 * ======================
 * Each task follows these principles:
 * 
 * 1. NON-BLOCKING: Tasks never wait indefinitely. They check conditions
 *    and return immediately if work cannot be done.
 * 
 * 2. STATELESS OR STATE-MACHINE: Tasks either have no state, or maintain
 *    their state in static variables (for cooperative scheduling).
 * 
 * 3. HEALTH REPORTING: Tasks report their health status to enable
 *    watchdog integration.
 * 
 * 4. MINIMAL EXECUTION TIME: Tasks do a small chunk of work and return,
 *    allowing other tasks to run.
 */

/**
 * @brief Watchdog task - Critical priority
 * 
 * PURPOSE: Ensures the watchdog is fed regularly and monitors system health.
 * 
 * DESIGN NOTE: This is the highest priority task. It runs every iteration
 * to check system health. The actual watchdog feeding is done by the
 * scheduler only if all tasks report healthy status.
 * 
 * WCET ANALYSIS: This task has minimal work to do, so WCET is very low
 * (< 10 microseconds on most platforms).
 */
static TaskResult_t Task_Watchdog(void)
{
    /* 
     * This task primarily serves as a health check point.
     * The actual watchdog feeding is handled by the scheduler.
     * Here we can perform additional system health checks.
     */
    
    /* Report healthy status */
    Superloop_SetTaskHealth(0, TASK_HEALTH_OK); /* Task ID 0 is this task */
    
    return TASK_RESULT_OK;
}

/**
 * @brief LED blinking task - Normal priority
 * 
 * PURPOSE: Blinks an LED at a specified rate to indicate system is running.
 * 
 * DESIGN NOTE: This is a classic example of a periodic task. It uses
 * static variables to maintain state between calls (a common pattern
 * in cooperative scheduling).
 * 
 * STATE MACHINE:
 * - OFF -> ON after LED_OFF_TIME_MS
 * - ON -> OFF after LED_ON_TIME_MS
 */
#define LED_ON_TIME_MS      (500U)
#define LED_OFF_TIME_MS     (500U)

static TaskResult_t Task_LedBlink(void)
{
    static uint32_t lastToggleTime = 0U;
    static bool ledIsOn = false;
    uint32_t currentTime;
    uint32_t toggleInterval;
    
    currentTime = Superloop_GetTick();
    
    /* Determine toggle interval based on current state */
    toggleInterval = ledIsOn ? LED_ON_TIME_MS : LED_OFF_TIME_MS;
    
    /* Check if it's time to toggle */
    if (Superloop_IsTimeoutElapsed(lastToggleTime, toggleInterval))
    {
        /* Toggle LED state */
        ledIsOn = !ledIsOn;
        HalSetLed(ledIsOn);
        
        /* Update last toggle time */
        lastToggleTime = currentTime;
    }
    
    return TASK_RESULT_OK;
}

/**
 * @brief Temperature monitoring task - High priority
 * 
 * PURPOSE: Reads temperature sensor and checks for out-of-range conditions.
 * 
 * DESIGN NOTE: This is an example of a monitoring task that might need
 * to trigger safety actions. In a real system, this could:
 * - Shut down equipment if temperature exceeds limits
 * - Log temperature data for analysis
 * - Trigger cooling systems
 */
#define TEMP_MIN_LIMIT      (-10)   /* Minimum safe temperature */
#define TEMP_MAX_LIMIT      (85)    /* Maximum safe temperature */
#define TEMP_HYSTERESIS     (2)     /* Hysteresis for threshold crossing */

static TaskResult_t Task_TemperatureMonitor(void)
{
    static bool overTemperature = false;
    static bool underTemperature = false;
    int16_t temperature;
    
    /* Read temperature sensor */
    temperature = HalReadTemperature();
    
    /* Check for over-temperature condition */
    if (temperature > (TEMP_MAX_LIMIT + TEMP_HYSTERESIS))
    {
        if (!overTemperature)
        {
            overTemperature = true;
            /* In real system: trigger cooling, log event, etc. */
            printf("[ALERT] Over-temperature: %d C\n", temperature);
        }
        /* Report degraded health due to temperature */
        return TASK_RESULT_ERROR;
    }
    else if (temperature < (TEMP_MAX_LIMIT - TEMP_HYSTERESIS))
    {
        overTemperature = false;
    }
    
    /* Check for under-temperature condition */
    if (temperature < (TEMP_MIN_LIMIT - TEMP_HYSTERESIS))
    {
        if (!underTemperature)
        {
            underTemperature = true;
            printf("[ALERT] Under-temperature: %d C\n", temperature);
        }
        return TASK_RESULT_ERROR;
    }
    else if (temperature > (TEMP_MIN_LIMIT + TEMP_HYSTERESIS))
    {
        underTemperature = false;
    }
    
    return TASK_RESULT_OK;
}

/**
 * @brief Button handling task with debouncing - Normal priority
 * 
 * PURPOSE: Reads button input with software debouncing and detects
 * button press/release events.
 * 
 * DEBOUNCING ALGORITHM:
 * This uses a simple counter-based debounce algorithm:
 * 1. When raw input changes, start counting
 * 2. Only accept change after input is stable for DEBOUNCE_COUNT samples
 * 3. Reset counter if input changes during counting
 * 
 * DESIGN NOTE: Debouncing is essential for reliable button handling.
 * Mechanical switches can bounce for 1-50ms, causing multiple false
 * transitions.
 */
#define DEBOUNCE_COUNT      (20U)   /* Number of stable samples needed */
#define BUTTON_SAMPLE_MS    (1U)    /* Sample interval */

static TaskResult_t Task_ButtonDebounce(void)
{
    static uint32_t debounceCounter = 0U;
    static bool lastRawState = false;
    static bool debouncedState = false;
    bool rawState;
    
    /* Read raw button state */
    rawState = HalReadButton();
    
    /* Check if state has changed */
    if (rawState != lastRawState)
    {
        /* Reset debounce counter on change */
        debounceCounter = 0U;
        lastRawState = rawState;
    }
    else
    {
        /* State is stable - increment counter */
        if (debounceCounter < DEBOUNCE_COUNT)
        {
            debounceCounter++;
        }
        else if (debouncedState != rawState)
        {
            /* Debounced state has changed */
            debouncedState = rawState;
            
            if (debouncedState)
            {
                printf("[INFO] Button pressed\n");
                /* Handle button press event */
            }
            else
            {
                printf("[INFO] Button released\n");
                /* Handle button release event */
            }
        }
    }
    
    return TASK_RESULT_OK;
}

/**
 * @brief Command processing task - Low priority
 * 
 * PURPOSE: Processes commands from a communication interface (e.g., UART).
 * 
 * DESIGN NOTE: This is an event-driven task. It checks if there are
 * pending commands and processes them. If no commands are pending,
 * it returns immediately without blocking.
 * 
 * In a real system, commands would come from:
 * - UART receive buffer
 * - CAN message queue
 * - USB HID reports
 */

/* Simple command structure */
typedef struct {
    uint8_t commandId;
    uint8_t data[8];
    uint8_t dataLength;
} Command_t;

/* Simulated command queue */
#define CMD_QUEUE_SIZE  (4U)
static Command_t g_cmdQueue[CMD_QUEUE_SIZE];
static volatile uint8_t g_cmdQueueHead = 0U;
static volatile uint8_t g_cmdQueueTail = 0U;

static bool CmdQueue_IsEmpty(void)
{
    return (g_cmdQueueHead == g_cmdQueueTail);
}

static bool CmdQueue_Dequeue(Command_t *cmd)
{
    if (CmdQueue_IsEmpty() || (cmd == NULL))
    {
        return false;
    }
    
    *cmd = g_cmdQueue[g_cmdQueueTail];
    g_cmdQueueTail = (g_cmdQueueTail + 1U) % CMD_QUEUE_SIZE;
    return true;
}

static TaskResult_t Task_CommandProcessor(void)
{
    Command_t cmd;
    
    /* Check if there's a command to process */
    if (!CmdQueue_Dequeue(&cmd))
    {
        /* No commands pending */
        return TASK_RESULT_NO_ACTION;
    }
    
    /* Process command */
    switch (cmd.commandId)
    {
        case 0x01: /* Get temperature */
            printf("[CMD] Temperature: %d C\n", g_temperature);
            break;
            
        case 0x02: /* Set LED */
            if (cmd.dataLength > 0)
            {
                HalSetLed(cmd.data[0] != 0U);
                printf("[CMD] LED set to %s\n", cmd.data[0] ? "ON" : "OFF");
            }
            break;
            
        case 0x03: /* Get status */
            printf("[CMD] Status: LED=%s, Temp=%d, WD=%lu\n",
                   g_ledState ? "ON" : "OFF",
                   g_temperature,
                   (unsigned long)g_watchdogFedCount);
            break;
            
        default:
            printf("[CMD] Unknown command: 0x%02X\n", cmd.commandId);
            break;
    }
    
    return TASK_RESULT_OK;
}

/**
 * @brief Idle task - Lowest priority
 * 
 * PURPOSE: Runs when no other tasks need to execute. Can be used for:
 * - Low-power mode entry
 * - Background diagnostics
 * - Statistics logging
 * 
 * DESIGN NOTE: This task only runs when all other tasks have nothing to do.
 * It should be very lightweight to avoid starving other tasks when they
 * become ready.
 */
static TaskResult_t Task_Idle(void)
{
    /* Could enter low-power mode here */
    /* In real hardware: __WFI(); (Wait For Interrupt) */
    
    return TASK_RESULT_NO_ACTION;
}

/*============================================================================*/
/*                             MAIN APPLICATION                                */
/*============================================================================*/

/**
 * @brief Application initialization
 * 
 * DESIGN NOTE: This function initializes all application-specific resources
 * before the scheduler starts. It returns false if any initialization fails,
 * preventing the scheduler from starting.
 */
static bool App_Init(void)
{
    /* Initialize simulated command queue */
    memset(g_cmdQueue, 0, sizeof(g_cmdQueue));
    g_cmdQueueHead = 0U;
    g_cmdQueueTail = 0U;
    
    /* Add a test command to the queue */
    g_cmdQueue[0].commandId = 0x03; /* Get status */
    g_cmdQueue[0].dataLength = 0;
    g_cmdQueueHead = 1U;
    
    printf("[INIT] Application initialized\n");
    return true;
}

/**
 * @brief Main entry point
 * 
 * DESIGN NOTE: The main function follows this pattern:
 * 1. Initialize hardware
 * 2. Configure scheduler
 * 3. Register tasks
 * 4. Start scheduler (infinite loop)
 * 
 * The scheduler should never return under normal operation. If it does,
 * enter an infinite loop to trigger watchdog reset.
 */
int main(void)
{
    SchedulerConfig_t schedulerConfig;
    int32_t taskId;
    uint32_t simulationIterations = 0U;
    
    /* Initialize hardware abstraction layer */
    HalInit();
    
    /* Configure scheduler */
    schedulerConfig.enableWatchdog = true;
    schedulerConfig.enableStatistics = true;
    schedulerConfig.watchdogTimeoutMs = 1000U;
    schedulerConfig.watchdogFeedFn = HalFeedWatchdog;
    schedulerConfig.getTickFn = HalGetTick;
    schedulerConfig.getTimeUsFn = HalGetMicrosecond;
    
    /* Initialize scheduler */
    if (!Superloop_Init(&schedulerConfig))
    {
        printf("[ERROR] Scheduler initialization failed\n");
        return -1;
    }
    
    /* Register tasks in priority order */
    
    /* Task 0: Watchdog - Critical priority, run every iteration */
    taskId = Superloop_RegisterTask(
        "Watchdog",
        Task_Watchdog,
        NULL,                       /* No init function */
        TASK_PRIORITY_CRITICAL,
        0U,                         /* Run every iteration */
        10U                         /* 10us WCET */
    );
    if (taskId < 0)
    {
        printf("[ERROR] Failed to register Watchdog task\n");
        return -1;
    }
    
    /* Task 1: Temperature Monitor - High priority, 100ms period */
    taskId = Superloop_RegisterTask(
        "TempMon",
        Task_TemperatureMonitor,
        NULL,
        TASK_PRIORITY_HIGH,
        100U,                       /* 100ms period */
        100U                        /* 100us WCET */
    );
    if (taskId < 0)
    {
        printf("[ERROR] Failed to register Temperature Monitor task\n");
        return -1;
    }
    
    /* Task 2: LED Blink - Normal priority, 1ms period (actual toggle in task) */
    taskId = Superloop_RegisterTask(
        "LedBlink",
        Task_LedBlink,
        NULL,
        TASK_PRIORITY_NORMAL,
        1U,                         /* Check every 1ms */
        50U                         /* 50us WCET */
    );
    if (taskId < 0)
    {
        printf("[ERROR] Failed to register LED Blink task\n");
        return -1;
    }
    
    /* Task 3: Button Debounce - Normal priority, 1ms period */
    taskId = Superloop_RegisterTask(
        "Button",
        Task_ButtonDebounce,
        NULL,
        TASK_PRIORITY_NORMAL,
        BUTTON_SAMPLE_MS,
        50U
    );
    if (taskId < 0)
    {
        printf("[ERROR] Failed to register Button task\n");
        return -1;
    }
    
    /* Task 4: Command Processor - Low priority, 10ms period */
    taskId = Superloop_RegisterTask(
        "CmdProc",
        Task_CommandProcessor,
        NULL,
        TASK_PRIORITY_LOW,
        10U,
        500U                        /* 500us WCET - commands may be complex */
    );
    if (taskId < 0)
    {
        printf("[ERROR] Failed to register Command Processor task\n");
        return -1;
    }
    
    /* Task 5: Idle - Idle priority, run when nothing else to do */
    taskId = Superloop_RegisterTask(
        "Idle",
        Task_Idle,
        NULL,
        TASK_PRIORITY_IDLE,
        0U,                         /* Run every iteration if nothing else */
        10U
    );
    if (taskId < 0)
    {
        printf("[ERROR] Failed to register Idle task\n");
        return -1;
    }
    
    printf("[INIT] All tasks registered, starting scheduler...\n");
    
    /*
     * SIMULATION LOOP (for PC testing)
     * =================================
     * In a real embedded system, we would call Superloop_Run() which
     * never returns. For PC simulation, we run a limited number of
     * iterations.
     */
    
    /* For simulation, we manually process scheduler iterations */
    while (simulationIterations < 5000U)
    {
        /* Simulate time passing */
        HalSimulateTick();
        
        /* Simulate button press at iteration 1000 */
        if (simulationIterations == 1000U)
        {
            g_buttonState = true;
        }
        
        /* Simulate button release at iteration 1100 */
        if (simulationIterations == 1100U)
        {
            g_buttonState = false;
        }
        
        /* Simulate temperature rise at iteration 2000 */
        if (simulationIterations == 2000U)
        {
            g_temperature = 90; /* Above max limit */
        }
        
        /* Simulate temperature normal at iteration 2500 */
        if (simulationIterations == 2500U)
        {
            g_temperature = 25;
        }
        
        /* Process one scheduler iteration */
        Superloop_Process();
        
        simulationIterations++;
    }
    
    /* Print final statistics */
    SchedulerStats_t stats;
    if (Superloop_GetStats(&stats))
    {
        printf("\n[STATS] Final Statistics:\n");
        printf("  Total iterations: %lu\n", (unsigned long)stats.totalIterations);
        printf("  Total task runs: %lu\n", (unsigned long)stats.totalTaskRuns);
        printf("  Watchdog feeds: %lu\n", (unsigned long)stats.watchdogFeeds);
        printf("  Idle time (ms): %lu\n", (unsigned long)stats.idleTimeMs);
    }
    
    printf("[EXIT] Simulation complete\n");
    
    /*
     * REAL EMBEDDED SYSTEM MAIN LOOP:
     * ===============================
     * In a real system, replace the simulation loop above with:
     * 
     * Superloop_Run();
     * 
     * // Should never reach here
     * while (1)
     * {
     *     // If we get here, something went wrong
     *     // Watchdog will reset the system
     * }
     */
    
    return 0;
}
