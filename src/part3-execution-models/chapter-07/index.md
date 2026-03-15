# Chapter 7 — Interrupt Architecture
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


While the superloop handles the steady, predictable progression of tasks, **Interrupts** are how an embedded system responds immediately to the unpredictable, asynchronous events of the physical world. 

An Interrupt Service Routine (ISR) is a special function triggered directly by the hardware (e.g., a pin changing state, a timer expiring, a UART byte arriving). When an interrupt occurs, the CPU instantly stops executing the main program (preemption), saves its current state, jumps to the ISR, executes it, and then resumes the main program exactly where it left off.

## The Dual-Edged Sword

Interrupts are powerful. They provide near-instantaneous response times (low latency) without requiring the main loop to constantly poll the hardware. However, interrupts are the single greatest source of complex, hard-to-reproduce bugs in embedded systems: **Race Conditions**.

Because an ISR preempts the main execution thread at an unpredictable moment, if both the ISR and the main loop modify the same memory (shared data), the data can be corrupted in ways that only happen once in a million iterations.

## Chapter Objectives

This chapter establishes strict architectural rules for writing, managing, and interacting with interrupts. Our goal is to harness the responsiveness of interrupts while mathematically eliminating the possibility of race conditions and system lockups.

*   **[What Belongs in an ISR](./01-what-belongs-in-isr.md):** Defining the "Keep It Short and Simple" (KISS) principle for interrupt handlers and identifying operations strictly forbidden inside an ISR.
*   **[ISR Handoff Patterns](./02-isr-handoff-patterns.md):** Establishing standard methods for an ISR to signal the main application that work needs to be done, deferring heavy processing to the main loop.
*   **[Shared Data & Concurrency](./03-shared-data.md):** The most critical section. How to safely share variables between the main loop and an ISR using `volatile`, atomicity, and critical sections.
*   **[Standardizing Interrupts](./04-standardizing-interrupts.md):** Company guidelines on interrupt priorities, naming conventions, and abstracting hardware vector tables.

## The Prime Directive

The overarching philosophy for interrupt architecture is **Deferred Processing**. 

An ISR should do the absolute bare minimum required by the hardware to clear the interrupt flag and capture volatile data. All subsequent processing, math, state changes, and UI updates must be deferred to the main execution context (the superloop or an RTOS task). 

Mastering this separation of concerns is what separates amateur embedded code from professional, production-ready architecture.
