# Chapter 12 — Error Handling and Defensive Design
## Who This Chapter Is For

- Embedded C engineers implementing or reviewing production firmware architecture
- Technical leads and architects defining team-wide standards

## Prerequisites

- Familiarity with C syntax and embedded build/debug workflows
- Completion of prior chapter topics in this curriculum (recommended)

## Learning Objectives

- Explain the core architectural principles covered in this chapter
- Apply the chapter rules to structure module boundaries and dependencies
- Evaluate existing code for architectural risks related to this chapter

## Key Terms

- Architecture boundary
- Module contract
- Dependency direction

## Practical Checkpoint

- Review one existing module and document 2 improvements based on this chapter's guidance
- Refactor one API or dependency edge to align with the chapter standards

## What to Read Next

- Continue with the next section in this chapter, then proceed to the next chapter in `src/SUMMARY.md`.


Welcome to Chapter 12. In the world of web development, an unhandled error might result in a 500 status code and a minor inconvenience for the user. In embedded systems, an unhandled error can result in a damaged engine, a deployed airbag, or a bricked satellite in orbit. 

Embedded software must be inherently defensive. It cannot assume that sensors will always return valid data, that memory allocations will always succeed, or that external flash memory won't occasionally become corrupted. 

## The Core Philosophy

Defensive design in C requires a structured, architectural approach to dealing with the unexpected. You cannot bolt error handling onto a system after the fact; it must be designed into the API boundaries and state machines from day one.

## Chapter Breakdown

1. **[Error Categories](01-error-categories.md):** Not all errors are created equal. We must distinguish between Expected Errors (e.g., a network timeout), Unexpected Errors (e.g., a corrupted configuration), and Fatal Faults (e.g., a division by zero or a hard fault).
2. **[Return Codes and Assertions](02-return-codes-assertions.md):** C does not have exceptions (`try/catch`). We will explore robust patterns for propagating errors up the call stack using standard return codes, and when to use `assert()` to catch unrecoverable logic errors during development.
3. **[Fail-Safe vs. Fail-Fast](03-fail-safe-vs-fail-fast.md):** How should your system react when the impossible happens? We will discuss the architectural choices between attempting to limp along (Fail-Safe) versus immediately halting the system and rebooting (Fail-Fast).
4. **[Observability and Diagnostics](04-observability-diagnostics.md):** When a device resets in the field, you need to know why. We will look at implementing lightweight telemetry, panic handlers, and persistent error logging across resets.

## The Goal

By the end of this chapter, you will understand how to build systems that anticipate failure. You will learn to write code that explicitly checks preconditions, rigorously propagates error states, and behaves predictably when faced with hardware failures or corrupted data. This is the hallmark of a mature embedded architecture.
