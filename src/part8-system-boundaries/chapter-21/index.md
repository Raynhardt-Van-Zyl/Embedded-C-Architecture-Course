# Chapter 21 — Startup, Linker, and System Boundaries
## Who This Chapter Is For

- Embedded C engineers responsible for bring-up, memory layout, or boot flow
- Technical leads reviewing startup code, memory maps, and system interfaces

## Prerequisites

- Familiarity with C syntax and embedded build/debug workflows
- Comfortable reading linker scripts, startup code, and generated map files

## Learning Objectives

- Explain what happens before `main()` executes on an embedded target
- Evaluate linker-script and memory-map decisions as architectural boundaries
- Define stable interfaces between bootloaders, applications, and retained memory

## Key Terms

- Reset vector
- Linker script
- ABI boundary

## Practical Checkpoint

- Trace one target from reset vector to `main()` and document the key initialization steps
- Review one linker script or map file and identify two architectural risks or guarantees

## What to Read Next

- Continue through this chapter, then proceed to Chapter 22 for target debug and hardware validation workflows.


This chapter closes a common gap in architecture training: many engineers can reason about modularity and testability, but cannot explain the exact startup path, memory placement, and binary boundaries that make the firmware boot correctly on real hardware.

The goal here is not to turn every reader into a toolchain specialist. The goal is to make startup, linker, and boot boundaries visible enough that architectural decisions remain grounded in the physical system.
