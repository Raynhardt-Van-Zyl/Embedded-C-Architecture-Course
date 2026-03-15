# Chapter 5 — Dependency Management
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


In C, dependencies are formed every time you type `#include "some_header.h"`. While it seems like a simple mechanism to access a function or a variable, it is actually the most fundamental architectural decision you make. 

When Module A includes Module B, Module A is now dependent on Module B. If Module B changes, Module A might need to change. If Module B has a bug, Module A might crash. If you want to use Module A in a new project, you are forced to bring Module B along with it.

If dependencies are not carefully managed, your codebase will rapidly devolve into a "Big Ball of Mud"—a monolithic, tangled mess where everything depends on everything else.

## The Goal of Dependency Management

The ultimate goal of dependency management is to create an architecture that is:

1.  **Directed:** Dependencies flow in a single, logical direction (usually from high-level business logic down to low-level hardware drivers).
2.  **Acyclic:** There are no loops (Module A depends on Module B, which depends on Module A).
3.  **Loose:** Modules interact through narrow, stable interfaces rather than direct knowledge of internal implementations.

## What to Expect in This Chapter

In this chapter, we will learn how to tame dependencies in Embedded C:

*   **[Dependency Direction](./01-dependency-direction.md):** Applying the Dependency Inversion Principle to embedded systems to protect your high-level application logic from low-level hardware changes.
*   **[Avoiding Cyclic Dependencies](./02-avoiding-cyclic-dependencies.md):** Identifying deadly `#include` loops and learning practical refactoring techniques (callbacks, abstractions, and forward declarations) to break them.
*   **[Compile-Time vs. Link-Time Dependencies](./03-compile-time-vs-link-time.md):** Understanding the difference between static `#include` coupling and dynamic link-time techniques like weak symbols and function pointers.
*   **[Configuration Without Chaos](./04-configuration-without-chaos.md):** Banishing `#ifdef` spaghetti code. We will explore cleaner ways to handle board variants and configuration options without destroying the readability of your codebase.

By mastering these dependency management techniques, you will build systems that are infinitely easier to test, maintain, and scale.
