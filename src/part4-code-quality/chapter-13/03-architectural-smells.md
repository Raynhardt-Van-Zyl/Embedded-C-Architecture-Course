# 13.3 Architectural Smells in Firmware

## 1. The Big Ball of Mud
In software engineering, a "code smell" is a surface indication of a deeper tactical problem (e.g., a function that is 300 lines long). An **Architectural Smell**, however, indicates that the fundamental structural integrity of the system is failing. 

If you ignore code smells, you get bugs. If you ignore Architectural Smells, you get the "Big Ball of Mud"—a monolithic, deeply intertwined system where modifying a simple UI menu somehow breaks the motor control loop. In a Big Ball of Mud, testing is impossible, onboarding new engineers takes months, and the fear of changing the code paralyzes development.

As a software architect, your primary job is to hunt down and eradicate Architectural Smells before they calcify.

### 1.1 Smell 1: The "God Object" (SystemManager.c)
The most common smell in embedded systems is the "God Object" or the "God Module." This is typically a file named `SystemManager.c`, `MainLogic.c`, or `AppCore.c`.

**The Symptom:** This file is usually over 2,000 lines long. It `#include`s almost every other header file in the project. It contains the main state machine, initializes the hardware, handles the USB interrupts, and prints debug logs.

**The Deep Technical Rationale:**
The God Object is a massive violation of the Single Responsibility Principle (SRP) at the module level. It centralizes all control flow, meaning every other module in the system is just a dumb data container. This creates an un-testable monolith. You cannot write a unit test for the motor control logic because it is physically entangled with the USB parsing logic inside the God Object.

**The Production-Grade Cure: Decentralized State Machines**
The architecture must be refactored into loosely coupled, autonomous modules. Instead of a God Object reading the USB and telling the motor to move, we use an RTOS Queue or an Event Bus (Publish/Subscribe pattern).

```c
/* PRODUCTION-GRADE: The Decoupled Event Architecture */
// file: usb_driver.c
void usb_receive_isr(void) {
    packet_t rx_packet;
    usb_read_hardware(&rx_packet);
    
    // The USB driver does NOT know about motors. It simply publishes an event.
    event_bus_publish(EVENT_NEW_COMMAND, &rx_packet);
}

// file: motor_controller.c
void motor_task(void *params) {
    packet_t cmd;
    while(1) {
        // The motor task waits for a command. It doesn't care if it came from
        // USB, UART, or BLE. The logic is completely decoupled and testable.
        if (event_bus_receive(EVENT_NEW_COMMAND, &cmd, portMAX_DELAY)) {
            motor_set_speed(cmd.payload);
        }
    }
}
```

### 1.2 Smell 2: Shotgun Surgery (The `#define` Web)
**The Symptom:** A developer needs to add a new sensor to the system. To do this, they have to modify 7 different files: they add an enum to `sensors.h`, add an initialization call in `main.c`, add an `extern` variable in `globals.h`, add a new case to a massive `switch` statement in `data_logger.c`, and update a `#define NUM_SENSORS` macro in `config.h`. 

This is called "Shotgun Surgery" because changing one logical concept requires scattering tiny changes across the entire codebase.

**The Deep Technical Rationale:**
Shotgun surgery proves that the system's data and the operations on that data are separated. It is a failure of encapsulation. 

**The Production-Grade Cure: Data-Driven Design (Tables and Structs)**
Instead of scattering logic, we encapsulate the concept into a single array of structures (a data-driven table). The code loops over the table. To add a new sensor, the developer modifies exactly *one* file: they add one row to the table.

```c
/* PRODUCTION-GRADE: Data-Driven Sensor Management */
// file: sensor_manager.c

typedef struct {
    const char *name;
    uint8_t i2c_addr;
    status_t (*init_func)(uint8_t addr);
    float (*read_func)(uint8_t addr);
} sensor_descriptor_t;

// The single source of truth. Add a new sensor by adding one row here.
static const sensor_descriptor_t system_sensors[] = {
    {"TEMP_PRIMARY", 0x4A, bme280_init, bme280_read_temp},
    {"TEMP_BACKUP",  0x4B, bme280_init, bme280_read_temp},
    {"PRESSURE",     0x77, bmp388_init, bmp388_read_pressure}
};

static const uint32_t NUM_SENSORS = sizeof(system_sensors) / sizeof(system_sensors[0]);

// No massive switch statements. No shotgun surgery.
void init_all_sensors(void) {
    for (uint32_t i = 0; i < NUM_SENSORS; i++) {
        system_sensors[i].init_func(system_sensors[i].i2c_addr);
    }
}
```

### 1.3 Smell 3: The `extern` Contagion (Global Variables)
**The Symptom:** You open a header file and see `extern uint32_t system_ticks;` or `extern bool motor_running;`. You search the codebase, and 14 different `.c` files are directly reading and writing to this variable.

**The Deep Technical Rationale:**
Global variables destroy thread-safety. If `motor_running` is a global boolean, any task can change it at any time. There is no way to enforce atomic access, no way to log when the state changes, and no way to set a hardware breakpoint on the *modification* of the variable without catching every single read access.

**The Production-Grade Cure: The Getter/Setter API**
State must be hidden behind an interface. The variable becomes `static` inside `motor.c`, and we expose `motor_is_running()` and `motor_set_state()`.

```c
/* PRODUCTION-GRADE: Encapsulated State */
// file: motor.c
#include <stdbool.h>

// Hidden from the linker. Protected from unauthorized modification.
static bool internal_motor_running = false;

bool motor_is_running(void) {
    return internal_motor_running;
}

void motor_set_state(bool run) {
    // We can now enforce thread-safety, validate inputs, and log state changes!
    __disable_irq(); // Or use a Mutex
    internal_motor_running = run;
    __enable_irq();
    
    system_log_state_change("Motor", run);
}
```

By hiding the state, we regain control over the architecture. The compiler enforces that developers use our API, which means we can guarantee thread-safety and observability for the entire 20-year lifespan of the product.

## 2. Company Standard Rules

**Rule 13.3.1:** The creation of "God Objects" (modules that control disparate, logically unrelated subsystems) is strictly forbidden. Subsystems MUST be decoupled using Event Buses, RTOS Queues, or Callback interfaces to ensure independent testability.
**Rule 13.3.2:** The `extern` keyword MUST NOT be used to expose variables across module boundaries. All module state MUST be declared `static` and accessed exclusively via Getter and Setter functions to enforce thread-safety, data validation, and encapsulation.
**Rule 13.3.3:** When iterating over multiple similar hardware peripherals or logical objects, the architecture MUST employ Data-Driven Design (arrays of function pointers or descriptor structs) to eliminate massive `switch` statements and prevent Shotgun Surgery.
**Rule 13.3.4:** If the implementation of a new feature requires modifying more than three distinct files (excluding the new feature files themselves), the architect MUST be notified to evaluate the codebase for the Shotgun Surgery smell and initiate a refactoring pass.