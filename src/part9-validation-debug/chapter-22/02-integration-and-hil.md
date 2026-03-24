# 22.2 Integration and hardware-in-the-loop testing

Different test layers answer different questions:

- Host tests prove that architecture and logic are decoupled enough to execute quickly and deterministically.
- Target tests prove that cross-compilation, startup code, linker configuration, and board assumptions are coherent.
- HIL tests prove that the firmware behaves correctly against real peripherals, timing, and physical interfaces.

## Minimum HIL suite

Every serious target should have a small smoke suite that answers:

- Does the image boot?
- Can the system report its version and health state?
- Do critical interrupts fire?
- Do the essential buses respond?
- Can the watchdog and recovery path be observed safely?

HIL should not attempt to replace every host test. Its job is to validate the real interfaces that host testing cannot prove.

## Artifact discipline

Each HIL run should capture:

- firmware version or build ID
- board identity
- test script version
- pass/fail result
- logs and any retained fault data

That turns lab runs into evidence rather than anecdotes.
