# Chapter 20.1: Assessing the Current Codebase

Before writing a single line of a new, pristine internal framework, the architect must perform an exhaustive, forensic audit of the existing legacy codebase. The goal of this assessment is absolutely not to judge the past developers (who were almost certainly operating under extreme time pressure, shifting requirements, and inadequate tooling). Rather, the objective is to impartially map the technical debt, quantify the architectural entanglement, and identify the exact structural boundaries where a modernized framework can provide immediate, measurable value.

Attempting to implement a new architecture without first understanding the deep complexities of the system you are replacing is the primary cause of multi-million dollar software migration failures.

## The "Big Rewrite" Anti-Pattern (Reiterated)

As discussed previously, the most perilous conclusion an architect can reach during an assessment is: *"This codebase is completely unsalvageable garbage; we must throw it all away and rewrite it from scratch."*

**The Organizational Failure of Rewrites:**
1. **The Feature Embargo:** A massive rewrite stalls all new product feature development for months, or even years, causing the business to lose market share to competitors.
2. **The Dual-Codebase Tax:** The engineering team is forced to split its velocity—half the team builds the new utopia, while the other half endlessly patches the legacy system because the company still relies on it for revenue. 
3. **Loss of Operational History:** Legacy code is ugly because the physical world is ugly. The 10-year-old codebase contains hundreds of obscure, undocumented bug fixes (e.g., "we added a 5ms delay here because the specific batch of I2C EEPROMs from 2018 have a weird clock-stretching silicon bug"). Throwing away the code discards that hard-earned, field-tested operational knowledge. The "clean" rewrite will inevitably fail in the field because it lacks this historical scar tissue.

Therefore, the assessment is not about finding reasons to rewrite; it is about mapping the terrain for a surgical **Strangler Fig Migration** (Chapter 18.2).

## Forensic Assessment Techniques

Instead of rewriting, we assess. We rely on automated tooling and empirical data rather than subjective engineering opinions to map the codebase.

### 1. Dependency Graphing and "God Objects"
The very first step is mapping the physical `#include` dependencies of the C files to find structural coupling. You want to identify the **"God Objects"**—files that `#include` dozens of modules and are, in turn, `#included` by everything else. These files represent massive bottlenecks to testability.

*Tooling:* We utilize tools like `Doxygen` (configured with Graphviz/`dot` generation), `cscope`, or Clang-based AST visualizers to generate massive, graphical include dependency graphs.

**What to forensically look for:**
- **Layer Violations:** Does `ui_display_logic.c` directly `#include "spi_hardware_driver.h"`? This indicates that the UI is talking directly to the silicon, completely bypassing any abstraction layers.
- **Cyclic Dependencies:** Does `module_a.h` include `module_b.h`, which in turn includes `module_a.h`? This is a fatal structural flaw in C that typically requires `#pragma once` hacks or massive header reorganizations to untangle.
- **The Mega-Header:** Is there a `global_system_defs.h` file that contains 3,000 lines of unrelated struct definitions, shared by every file in the system? Any modification to this file forces a complete rebuild of the entire binary, destroying developer velocity.

### 2. The Global State Audit
Global variables are the enemy of unit testability and the primary cause of concurrency bugs in embedded systems. Run a script or a `grep`/`nm` search against the compiled binary for all global variables that are not explicitly marked `static` to their translation unit.

```bash
# Example bash command to find non-static globals via the ELF symbol table
# 'B' denotes uninitialized data section (.bss), 'D' denotes initialized data section (.data)
arm-none-eabi-nm legacy_firmware.elf | grep -i ' [BD] '
```

**What to forensically look for:**
- **Shared State Variables:** `extern int current_system_state;` shared across 15 different modules. If a bug occurs where the state transitions incorrectly, it is impossible to determine which of the 15 modules mutated the global memory.
- **Unprotected Async Buffers:** Global arrays accessed in both Hardware Interrupts (ISRs) and the main `while(1)` loop without `volatile` qualifiers, atomic guards, or RTOS mutexes. These are guaranteed, ticking-time-bomb data races.

### 3. Error Handling Inconsistency Review
Review the API boundaries of the existing legacy modules to determine how failures are currently communicated.

**What to forensically look for:**
- Does Driver A return `0` for success and `-1` for failure?
- Does Driver B return `true` for success and `false` for failure?
- Does Module C lack return values entirely, choosing instead to simply execute an infinite `while(1)` trap loop or trigger a silent software reset when an I2C timeout occurs?

This chaotic lack of standardization proves the immediate necessity of the framework's Unified Error Handling system (Chapter 16.1).

## Defining the "Seams" for Injection

Once the empirical assessment is complete, the architect must identify the **Seams**. As defined in Chapter 14.2, a seam is a place in the code where you can alter behavior without editing in that place. It is a boundary where we can cleanly sever the legacy code from the new framework.

**Example Seam Identification:**
During your dependency graph audit, you notice that `sensor_a.c`, `sensor_b.c`, and `eeprom_storage.c` all heavily rely on direct, hardcoded I2C register access via the vendor's `stm32_hal_i2c.h`.

*The Actionable Migration Plan:* The I2C subsystem is a perfect candidate for a Seam. We do not rewrite the sensors. Instead, we define a clean `i2c_interface_t` (Chapter 19.3) in the new framework, write the concrete HAL driver, and port *just* the sensor modules to use the new interface pointer, leaving the rest of the massive legacy application completely untouched.

---

## Company Standard Rules

**Rule 20.1.1:** **Rule of Objective Metrics.** Codebase modernization assessments must be based strictly on quantifiable, empirical data (cyclomatic complexity scores, dependency graph node count, global variable count derived from the `.elf` file), and never on subjective developer aesthetic opinions.

**Rule 20.1.2:** **Rule of Historical Preservation.** The architecture team must implicitly assume that bizarre or "ugly" legacy code exists for a valid physical reason until proven otherwise. All undocumented hardware quirks embedded in legacy logic must be exhaustively cataloged and covered by Characterization Tests before attempting to abstract them behind the new framework.

**Rule 20.1.3:** **Incremental Boundary Identification.** Before any new framework code is authored to replace legacy systems, the architect must identify and formally document at least three distinct "Seams" (interfaces) where the new framework can be injected without disrupting the execution flow of the global legacy super-loop.