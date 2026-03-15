# Chapter 9 — Event-Driven and State-Machine Design
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


As embedded systems grow in complexity, whether running on a bare-metal superloop or a preemptive RTOS, the logic required to manage system behavior becomes increasingly difficult to track. 

If a system uses deep, nested `if-else` statements spread across multiple files to determine what it should be doing—e.g., checking if a button is pressed, *and* if the motor is currently running, *and* if the battery is not low, *then* execute an action—the codebase quickly becomes unmaintainable "spaghetti code."

The architectural solution to this complexity is **Event-Driven Design** and **Finite State Machines (FSMs)**.

## The Shift in Paradigm

Traditional procedural programming focuses on *what the system is doing right now* (e.g., executing a sequence of instructions).

Event-driven design focuses on *what state the system is currently in* and *how it reacts to external events* (e.g., a button press, a timer expiration, a network packet arrival).

This shift in paradigm allows us to decouple the source of an event (like a hardware interrupt) from the logic that handles it, leading to highly modular, testable, and robust firmware.

## Chapter Objectives

This chapter establishes the company standards for designing complex behavioral logic using state machines. By standardizing how state machines are implemented, we ensure that the control flow of our devices is mathematically verifiable, easy to debug, and visually documentable.

*   **[Event-Driven Structure](./01-event-driven-structure.md):** Defining what an "event" is, how events are generated, queued, and dispatched to the application logic.
*   **[Finite State Machines (FSMs)](./02-finite-state-machines.md):** The core architectural pattern. How to design, draw, and implement standard flat FSMs using `switch-case` statements or function pointer tables.
*   **[Hierarchical State Machines (HSMs)](./03-hierarchical-state-machines.md):** Managing extreme complexity. When a standard FSM explodes into too many states, we use HSMs to group states and handle common events cleanly without code duplication.
*   **[Translating Behavior into Code](./04-translating-behavior-into-code.md):** Company templates and best practices for converting UML Statecharts directly into robust, production-ready C code.

## The Promise of State Machines

The greatest advantage of standardizing on State Machine architecture is that the design phase and the implementation phase become tightly coupled. 

A software architect can draw a state diagram on a whiteboard, review it with the team to ensure all edge cases are handled, and then directly translate that drawing into C code with a 1-to-1 mapping. When a bug occurs, developers don't look at the C code first; they look at the state diagram. This visual approach to firmware design is the hallmark of a mature engineering team.
