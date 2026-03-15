# Chapter 11 — API and Header Hygiene
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


Welcome to Chapter 11. C does not have built-in package management or access modifiers like `public`, `private`, or `protected` found in languages like C++ or Java. Instead, the language relies on a primitive text-replacement mechanism (the preprocessor) and header files to declare what functionality is available across different source modules.

Because of this lack of native module support, **header files dictate the architecture of a C codebase.**

## The Importance of Header Hygiene

Poorly managed header files are the number one cause of long compile times, circular dependencies, and leaked implementation details. When an embedded engineer says a codebase is "spaghetti code," they are often referring to how the headers are included rather than the actual functions within the C files.

In a well-architected embedded system, a header file is a strict contract. It provides the minimum amount of information necessary for another module to use an API, and absolutely nothing more.

## Chapter Breakdown

1. **[Header File Responsibilities](01-header-file-responsibilities.md):** What belongs in a header file and what belongs in a C file? We will explore the separation of public interfaces from private implementations.
2. **[Include Discipline](02-include-discipline.md):** Mastering the `#include` directive. We will cover include guards, forward declarations, and how to prevent the dreaded "circular dependency" trap.
3. **[Macros, Inline Functions, and Constants](03-macros-inline-constants.md):** Moving away from unsafe `#define` macros. We will discuss why `inline` functions and `const` variables provide type safety and are superior architectural choices for modern embedded C.
4. **[Namespacing Conventions](04-namespacing-conventions.md):** Because C lacks namespaces, we must emulate them. We will establish rules for prefixing functions, variables, and macros to avoid symbol collisions in large codebases.

## The Goal

By the end of this chapter, you will understand how to structure your `.h` and `.c` files so that they represent highly cohesive, loosely coupled modules. You will learn to treat your header files as the "public face" of your modules, ruthlessly hiding internal state and logic to create robust and easily testable embedded software.
