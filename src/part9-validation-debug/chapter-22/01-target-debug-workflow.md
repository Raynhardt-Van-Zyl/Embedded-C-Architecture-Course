# 22.1 Target debug workflow

A professional target-debug workflow is a pipeline, not a heroic act. It should be possible for two engineers to reproduce the same debug session shape even if they use different hosts or debuggers.

## Minimum workflow

1. Confirm build identity and exact commit
2. Program the target with a known image
3. Halt at reset or a defined synchronization point
4. Capture reset reason and critical registers
5. Verify clocks, memory map assumptions, and peripheral-init state
6. Resume with trace, logging, or targeted breakpoints
7. Save artifacts before making the next experimental change

## Tooling expectations

- SWD/JTAG access must be documented per board
- OpenOCD or equivalent probe tooling must be scripted, not tribal knowledge
- GDB, probe CLI, or IDE launch settings must identify a known-good bring-up path
- Logging backends such as UART, RTT, or SWO should be treated as first-class debug channels

## Architecture implication

If debug depends on hidden manual steps, the team does not actually have a repeatable delivery process. It has a dependency on one engineer's memory.
