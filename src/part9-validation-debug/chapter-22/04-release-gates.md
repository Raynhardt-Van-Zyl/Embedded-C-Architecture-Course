# 22.4 Release gates for real firmware

Real firmware should not be released because `main` built and a few unit tests passed. Release gates must reflect both software quality and hardware reality.

## Recommended gate stack

- static analysis and warning-clean target build
- host test suite
- target build and startup sanity check
- HIL smoke suite on representative hardware
- retained-fault / crash-log path validated
- release artifact bundle containing binary, map file, symbols, and version metadata

## Why this matters

Embedded teams often over-index on one gate:

- only host tests, which miss integration faults
- only bench testing, which kills iteration speed
- only manual sign-off, which does not scale

A good release process uses all three: automation, target evidence, and engineering judgment.
