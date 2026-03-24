# 21.4 Map files and post-mortem analysis

Map files and crash artifacts connect architecture to reality. They turn "the firmware faulted somewhere" into a workflow that can identify the code region, memory pressure, and binary composition involved.

## What to extract from a map file

- total flash and RAM usage
- largest contributors by object file
- location of vector table, retained sections, and DMA sections
- duplicate utility code or unexpected vendor pulls

## What to capture after a fault

- program counter
- link register
- stack pointer
- fault status registers
- reset reason
- firmware version and build ID

With that information, a team can use `addr2line`, map files, and retained-memory dumps to triage failures without reproducing them interactively on a bench every time.

## Engineering rules

- Crash artifacts must be versioned against the exact binary that produced them.
- Post-mortem data must survive at least one reset path.
- Release builds must preserve enough symbols or symbol artifacts to support address-to-line translation during support and incident response.
