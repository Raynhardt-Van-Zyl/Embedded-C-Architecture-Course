# 21.2 Linker scripts and memory maps

Architecture in embedded systems is partly a memory-placement problem. Linker scripts decide where code lives, where stacks and heaps live, how DMA-visible buffers are placed, and which symbols define hardware-visible boundaries.

## What an architect must be able to read

- Flash and RAM region definitions
- Section placement for `.text`, `.rodata`, `.data`, `.bss`, and custom sections
- Stack/heap reservations
- No-init or retained-RAM sections
- Dedicated sections for DMA buffers, boot metadata, shared memory, or calibration data

## Why this matters architecturally

- A DMA buffer in cached RAM may create silent coherency faults.
- A retained crash record placed in normal `.bss` will be erased at boot.
- An oversized RTOS stack allocation may silently consume the RAM budget needed by another subsystem.
- A bootloader/application contract usually depends on fixed addresses or fixed sections that must not drift.

## Map files as review tools

Generated map files are not only for toolchain specialists. They are one of the fastest ways to answer architecture questions:

- Which object file pulled this symbol into flash?
- Why did RAM usage spike?
- Did a supposedly host-only helper leak into the target image?
- Is the interrupt vector table actually linked where the boot ROM expects it?

## Engineering rules

- Every release build should retain a map file as a build artifact.
- Custom sections must have one documented purpose each; never create anonymous linker magic.
- Large or safety-critical memory consumers should be reviewed using both source code and the final map file.
