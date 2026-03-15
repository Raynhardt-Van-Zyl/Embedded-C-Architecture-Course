# Chapter 3 — Designing Modules in Embedded C
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


In the realm of Embedded C, where resources are constrained and systems are intimately tied to hardware, the architecture of the software often dictates its long-term survival. As projects grow from a few thousand lines of code to hundreds of thousands or even millions, the ad-hoc approaches that worked for quick prototypes rapidly break down. This is where **Modular Design** becomes not just a best practice, but a critical necessity.

Modular design is the process of subdividing a system into smaller, manageable, and independent units—modules. A well-designed modular architecture brings order to chaos, enabling teams to build robust, scalable, and maintainable embedded systems.

## Why Modular Design Matters in Embedded Systems

1. **Parallel Development:** When a system is heavily intertwined, engineers step on each other's toes constantly. Modular design establishes clear boundaries, allowing multiple developers or teams to work on different parts of the system simultaneously without causing integration nightmares.
2. **Testability:** In embedded systems, testing on target hardware is slow and cumbersome. If your business logic is tightly coupled to hardware registers, unit testing becomes nearly impossible. Modular design allows you to isolate logic from hardware, enabling off-target unit testing and mock injection.
3. **Reusability across Platforms:** Hardware changes. Microcontrollers go end-of-life (EOL) or become unavailable due to supply chain shortages. A modular architecture ensures that replacing a microcontroller requires rewriting only the hardware-specific layers, not the entire application stack.
4. **Comprehensibility:** The human brain can only hold so much context at once. By breaking a system into focused modules with single responsibilities, we reduce cognitive load, making it easier for new engineers to understand and contribute to the codebase.

## What to Expect in This Chapter

In this chapter, we will lay the foundational rules for creating robust modules in standard C. We will cover:

*   **[What is a Module in C?](./01-what-is-a-module.md):** Defining the physical and logical boundaries of a C module, establishing namespaces, and avoiding common structural anti-patterns.
*   **[Cohesion and Coupling](./02-cohesion-and-coupling.md):** The twin pillars of software architecture. We will analyze how to maximize internal module cohesion while minimizing external coupling to other parts of the system.
*   **[Public API vs. Private Implementation](./03-public-api-vs-private-implementation.md):** Mastering the art of information hiding in C using the `static` keyword, incomplete types (opaque pointers), and strict header file discipline.
*   **[Defining Module Contracts](./04-defining-module-contracts.md):** Moving beyond function signatures to define behavioral contracts using preconditions, postconditions, assertions, and standardized error handling.

By the end of this chapter, you will have a strict set of guidelines for crafting Embedded C modules that are resilient, decoupled, and easy to maintain. These principles will form the bedrock for the more advanced architectural patterns we will explore in subsequent chapters.

---

**References:**
* Barr Group Embedded C Coding Standard
* MISRA C:2012 Guidelines
