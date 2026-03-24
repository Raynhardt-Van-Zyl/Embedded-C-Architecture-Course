# 22.3 Fault capture and triage

When a target faults, the first obligation is to preserve evidence before someone power-cycles the board and starts guessing.

## Triage order

1. Identify reset reason
2. Capture CPU state and fault status registers
3. Preserve logs, retained-memory fault data, and build identity
4. Determine whether the failure is startup, configuration, timing, memory, or external-interface related
5. Reproduce with the fewest moving parts possible

## Useful classification buckets

- startup and linker faults
- stack overflow or memory corruption
- ISR / RTOS interaction faults
- peripheral timeout or bus-lock faults
- bootloader / image-compatibility faults

Teams that classify faults consistently improve faster because they stop treating every target incident as a unique mystery.
