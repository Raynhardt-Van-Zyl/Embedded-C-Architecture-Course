# Chapter 18.2: Legacy Migration and The Strangler Fig Pattern

The reality of the embedded software industry is that most companies cannot simply abandon their legacy code. The chaotic, 10-year-old, bare-metal spaghetti code that currently runs the core product line is what generates the company's revenue. However, if left as-is indefinitely, this legacy codebase becomes a massive architectural anchor. It prevents the team from upgrading to modern silicon architectures, makes adding new features exponentially slower, and makes retaining modern software talent nearly impossible.

When tasked with modernizing this legacy code to adhere to the new 20-year framework, the architect must proceed with surgical precision. 

## The "Big Rewrite" Anti-Pattern

The most dangerous conclusion an engineering team can reach is: *"This codebase is garbage, we need to throw it out and rewrite it from scratch."*

**The Deep Rationale of Failure:** The "Big Rewrite" almost always ends in disaster. 
1. **The Feature Freeze:** A rewrite stalls new feature development for months or years. 
2. **The Dual Maintenance Nightmare:** Because the company still needs to support existing customers, the team is forced to maintain two entirely separate codebases simultaneously. 
3. **Chesterton's Fence:** Legacy code, no matter how ugly, usually contains years of hidden, undocumented bug fixes for incredibly specific edge-case hardware quirks (e.g., "we have to add a 2ms delay here because the specific batch of EEPROMs from 2018 have a weird clock-stretching bug"). Throwing away the code discards that hard-earned, field-tested operational knowledge. The new, "clean" code will inevitably fail in the field because it lacks this history.

## The Solution: The Strangler Fig Pattern in C

The Strangler Fig is a software design pattern popularized by Martin Fowler, named after a type of vine that seeds itself in the upper branches of a host tree. It slowly grows downwards, wrapping around the host tree until it roots in the soil. Over time, the vine grows thicker, eventually suffocating the host tree. The host dies and rots away, leaving only the tree-shaped Strangler Fig standing in its place.

In embedded software, this means incrementally replacing specific functionalities of the legacy system with new, framework-compliant modules, while the rest of the legacy system continues to operate normally.

### Step 1: Wrap the Legacy Core (The Adapter Layer)

Suppose you have a massive legacy file `super_loop_core.c` that directly accesses hardware registers, relies on 50 global variables, and handles all business logic.

You are tasked with writing a new feature (e.g., a new BLE telemetry module). You must write this new module using the pristine new framework rules (Dependency Injection, Opaque Pointers, 100% Host Testability). The new module cannot access legacy global variables, and the legacy code has no concept of your new interfaces.

You bridge the gap by creating an **Adapter Layer**.

```c
// legacy_bridge_adapter.c (The Quarantine Zone)

#include "new_ble_telemetry.h"  // The pristine new framework code
#include "legacy_globals.h"     // The messy old legacy code

// 1. Implement the new framework's interface using legacy data
static sys_err_t adapter_read_sensor_data(uint8_t* out_data) {
    // Accessing legacy global variables is allowed ONLY inside this adapter!
    // We are translating the chaotic global state into the clean API contract.
    *out_data = g_LegacySensorValue_ADC; 
    return SYS_OK;
}

// 2. The Initialization Bridge
void legacy_adapter_init(void) {
    // Create the framework dependency configuration
    ble_config_t config = {
        .sensor_read_cb = adapter_read_sensor_data
    };
    
    // Initialize the pristine new module, injecting the legacy behavior
    ble_telemetry_init(&g_ble_inst, &config);
}
```

The new `ble_telemetry` module is perfectly decoupled, completely unit-testable on a PC, and adheres to all framework rules. It has absolutely no idea it is communicating with a 10-year-old global variable. The architectural "rot" is successfully quarantined to the `legacy_bridge_adapter.c` file.

### Step 2: Intercepting the Main Loop

Next, you must carve out a spot in the legacy `main()` or super-loop to service the new framework.

```c
// legacy_main.c
extern void legacy_adapter_init(void);
extern void ble_telemetry_process(ble_telemetry_t* inst);

int main(void) {
    Legacy_HardwareInit();
    
    // Boot the new framework bridge
    legacy_adapter_init(); 

    while(1) {
        Legacy_SuperLoop_Tasks(); // The old world
        
        // Give execution time to the new framework components
        ble_telemetry_process(&g_ble_inst); // The new world
    }
}
```

### Step 3: Strangulation

Over the coming months and years, as bugs are discovered or features are expanded in the legacy code, you **do not fix them in the legacy files**. 

Instead, you extract that specific piece of logic (e.g., the PID temperature controller), rewrite it as a new, framework-compliant module in `src/app/`, write exhaustive unit tests for it on the host PC, and use the Adapter layer to plug it back into the legacy system. 

Slowly, line by line, sprint by sprint, the adapter layer consumes the legacy code. `super_loop_core.c` shrinks, while the new `src/app/` folder grows. Eventually, the legacy core is entirely replaced, the adapter layer is deleted, and the migration is complete. The host tree is gone; only the Strangler Fig remains.

---

## Company Standard Rules

**Rule 18.2.1:** **Prohibition of "Big Rewrites".** Proposals to entirely rewrite functional, field-deployed legacy systems from scratch are prohibited. Modernization efforts must utilize incremental migration patterns (like the Strangler Fig) that maintain a continuously deployable and functional target binary.

**Rule 18.2.2:** **The Dependency Flow Rule.** Newly authored framework-compliant modules (`src/app/`) are strictly forbidden from `#include`-ing any legacy header files or accessing legacy global variables. The dependency must flow outwards: The Adapter layer knows about both systems; the new code knows nothing about the legacy code.

**Rule 18.2.3:** **Characterization Testing.** Before extracting and rewriting a piece of legacy logic, engineers must write "Characterization Tests" (also known as Golden Master tests). These tests capture the exact inputs and outputs of the messy legacy function (including its bugs and quirks) to ensure the new, decoupled framework module behaves identically in the physical system.

**Rule 18.2.4:** **Quarantine Enforcement.** All interaction between the legacy system and the new framework must occur exclusively within designated Adapter source files. Mixing legacy global state access with new framework logic within the same function block is an architectural violation.