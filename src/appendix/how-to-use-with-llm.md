# How to Use This Standard with LLMs

Large Language Models (LLMs) like GitHub Copilot, ChatGPT, Claude, and specialized coding assistants are rapidly becoming integral to embedded software development. However, LLMs are trained on a vast corpus of public code, which includes a significant amount of poor quality, non-MISRA-compliant, and poorly architected embedded C.

If left unguided, an LLM will often generate code that violates company standards (e.g., using `malloc`, ignoring `static`, or writing deeply nested logic). This document outlines how to encode our architecture guidelines into prompts and context to force LLMs to generate compliant code.

## 1. Establishing the "System Prompt"

When using conversational LLMs (like ChatGPT or Claude) for architectural planning or generating entire modules, you must establish a strong "System Prompt" or contextual preamble. This acts as a set of inviolable rules for the AI.

### Example Embedded C Master Prompt

Provide the following text to the LLM before asking it to write code:

> "You are an expert Embedded C firmware engineer. You write highly reliable, MISRA-C inspired, safety-critical code. Adhere strictly to the following rules:
> 1. Use `<stdint.h>` types exclusively (e.g., `uint32_t`, `int8_t`).
> 2. Never use dynamic memory allocation (`malloc`, `calloc`, `free`).
> 3. Encapsulate all module state using `static` variables or by passing an opaque context `struct`.
> 4. Do not use global variables unless absolutely necessary; if used, they must be `static` to the file.
> 5. Return status enums (e.g., `err_t`) from functions, returning data via pointer arguments.
> 6. All hardware interactions must be abstracted through a HAL interface (struct of function pointers).
> 7. Keep all functions under 50 lines of code.
> 8. Add `const` to pointers whenever the underlying data should not be modified.
> 9. Brace `{}` all control structures, even single-line statements.
> Generate clean, modular, and extensively commented code explaining the *why*, not the *what*."

## 2. Preventing Common LLM Hallucinations in Embedded C

LLMs often hallucinate or default to "desktop C" patterns. You must explicitly instruct them to avoid these.

### Anti-Pattern: Desktop Standard Library Usage
**LLM Tendency:** Using `<stdio.h>` (`printf`), `<string.h>` (unsafe `strcpy`), or `<time.h>`.
**Correction Prompt:** *"Do not use `printf`. If logging is required, use the project's custom `LOG_INFO()` macro. Use safe string bounds (e.g., `strncpy`, `snprintf`) and never assume null-termination."*

### Anti-Pattern: Ignorance of Hardware Volatility
**LLM Tendency:** Forgetting the `volatile` keyword when polling hardware registers or sharing variables with ISRs.
**Correction Prompt:** *"If defining hardware registers or variables shared with an Interrupt Service Routine, you must use the `volatile` keyword. Provide memory barrier instructions if applicable."*

## 3. Guiding Architectural Generation (The "Step-by-Step" Approach)

Do not ask an LLM to "Write an I2C sensor driver." It will likely generate a monolithic mess. Instead, guide it through the architectural phases defined in this standard.

**Step 1: Interface Design**
> "Design a C header file for an I2C temperature sensor driver. Define an opaque context struct, an initialization function that accepts an I2C HAL function pointer struct, and a read function that returns a status enum and outputs the temperature via a pointer."

**Step 2: Review and Refine**
Review the LLM's header. Ensure it matches your standards.

**Step 3: Implementation**
> "Now, write the `.c` file implementing this header. Remember to make all internal helper functions and state variables `static`. Do not allocate the context struct dynamically; assume the caller provides it."

## 4. Using LLMs for Refactoring and Code Review

LLMs are excellent at reviewing code against a standard. You can provide a snippet of legacy code and ask the LLM to modernize it.

**Refactoring Prompt Example:**
> "Review the following C code against MISRA-C guidelines and our company standard (no dynamic allocation, use stdint.h, encapsulate globals). Identify all violations. Then, rewrite the code to be compliant, improving modularity and using guard clauses to reduce nesting."

## Summary

LLMs are powerful accelerators, but they require a strict "firmware context" to be useful in embedded systems. By continuously injecting the rules from this architecture course into your prompts, you can force the AI to act as a compliant, expert senior embedded engineer rather than a generic C programmer.
