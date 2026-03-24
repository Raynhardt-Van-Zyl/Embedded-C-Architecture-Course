# 21.1 Reset flow and startup code

Every embedded system has a hidden control path that executes before application code sees a single line of C. That path begins with reset handling, vector-table selection, stack-pointer initialization, and early runtime setup.

For architecture work, the important point is simple: startup code is not "toolchain boilerplate" that can be ignored. It defines which memory gets initialized, which fault handlers exist, when clocks become valid, and what assumptions the rest of the system is allowed to make.

## The minimum startup mental model

1. The CPU fetches the initial stack pointer and reset handler from the vector table.
2. The reset handler copies initialized data from flash into RAM.
3. The reset handler zeroes the `.bss` region.
4. Optional early silicon setup occurs: clocks, FPU enablement, watchdog handling, memory-protection setup.
5. The C runtime calls constructors if the language/runtime requires them.
6. Control finally reaches `main()`.

If any of those steps are wrong, the architecture above them does not matter. A bad `.bss` clear policy or a stale vector-table address will break a beautifully modular firmware just as effectively as a race condition.

## Engineering rules

- Startup code must be version-controlled, reviewed, and treated as product code.
- Any RAM region that must survive reset, such as crash logs or retained state, must be explicitly excluded from generic zeroing.
- The reset path must document what is safe before scheduler start, before clock configuration, and before peripheral initialization.

## Practical exercise

Take one existing target and produce a one-page boot trace: reset source, vector table location, reset handler symbol, data copy region, `.bss` clear region, no-init region, and the exact handoff into `main()`.
