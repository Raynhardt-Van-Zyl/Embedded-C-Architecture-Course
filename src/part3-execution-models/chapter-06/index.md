# Chapter 6 — Bare-Metal Superloop Architecture
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


The most fundamental execution model in embedded systems is the **Superloop**, often referred to as bare-metal programming. It forms the backbone of countless embedded devices, from simple sensor nodes to complex, single-purpose controllers.

In this execution model, the system initializes its hardware and software components, then enters an infinite loop (`while(1)` or `for(;;)`) where it continuously polls for events, processes data, and updates outputs. There is no underlying Operating System or Real-Time Operating System (RTOS) managing threads or context switching.

## Why Study the Superloop?

While modern, complex embedded systems often rely on an RTOS or Linux, understanding the superloop is non-negotiable for an embedded software architect:

1.  **Foundation for Advanced Architectures:** Even within an RTOS task, the fundamental structure is often a superloop. The RTOS merely manages multiple superloops concurrently.
2.  **Resource Constraints:** Many microcontrollers (e.g., 8-bit AVR, ultra-low-power ARM Cortex-M0+) lack the RAM and flash memory to justify the overhead of an RTOS.
3.  **Deterministic Simplicity:** For systems with minimal asynchronous events, a well-architected superloop can provide highly deterministic and predictable behavior that is sometimes harder to achieve with an RTOS due to scheduler jitter.
4.  **Hardware Proximity:** Working in a superloop forces a deep understanding of hardware timers, interrupts, and polling mechanisms without the abstraction layer of an OS.

## Chapter Objectives

In this chapter, we will dissect the superloop architecture to establish robust company standards for bare-metal development:

*   **[The Basic Superloop Model](./01-superloop-model.md):** Formalizing the structure of the `while(1)` loop, separating initialization from execution, and identifying common anti-patterns like blocking code.
*   **[Cooperative Scheduling](./02-cooperative-scheduling.md):** Evolving the basic superloop into a cooperative scheduler using state machines to handle concurrent tasks without true preemption.
*   **[Time Management](./03-time-management.md):** Establishing standard practices for tracking time, implementing non-blocking delays, and executing periodic tasks using hardware timers like SysTick.
*   **[Superloop Failure Modes](./04-superloop-failure-modes.md):** Identifying the limitations of the superloop, such as priority inversion, starvation, and jitter, to know when this model is no longer sufficient.

## Establishing the Standard

A poorly designed superloop quickly devolves into "spaghetti code," where tasks block each other, real-time deadlines are missed, and adding new features becomes impossible. 

The goal of our architectural standards is to ensure that every bare-metal application written within the company adheres to strict rules regarding non-blocking execution, modular task design, and predictable timing. This ensures that even our simplest devices are robust, maintainable, and scalable.
