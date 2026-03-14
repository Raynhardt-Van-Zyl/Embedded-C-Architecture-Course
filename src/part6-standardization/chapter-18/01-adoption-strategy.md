# 18.1 Adoption strategy

Introducing a new software architecture to an established team is a delicate operation. Engineers are naturally skeptical of change, especially when that change involves adopting new rules, learning new abstraction layers, or writing more boilerplate code for the sake of "testability."

A successful rollout requires demonstrating immediate value rather than relying on authoritative mandates. 

## 1. Avoid Boiling the Ocean

The most common failure mode of an architectural rollout is attempting to refactor the entire company's codebase simultaneously. This leads to months of downtime where no new product features are shipped, executives get angry, and the architecture team gets fired.

**Rule:** Do not halt product development to adopt the framework.

Instead, the framework must be introduced incrementally. 

## 2. The Pilot Project

The best way to prove the value of the new standard is through a carefully selected **Pilot Project**. 

Choose a project with the following characteristics:
*   **Low to Medium Risk:** Do not choose the company's flagship, multi-million dollar product critical path as the testing ground for a brand-new architecture. Choose a secondary product or an internal tool.
*   **Greenfield (Mostly):** It is much easier to prove the concept on a new codebase than to immediately fight the friction of legacy code.
*   **Visible Scope:** The project should be complex enough to actually benefit from the framework (e.g., requires an RTOS, uses multiple peripherals, needs networking), so the benefits of the HAL and OSAL are obvious.

Execute this project strictly using the new framework. Measure the results: Did unit testing reduce time spent debugging on hardware? Did the standard HAL make it easier to write the drivers? Document these wins.

## 3. Creating Framework Champions

An architecture shouldn't be enforced solely by one "Chief Architect." It needs advocates embedded within the development teams. These are your **Champions**.

*   Identify senior developers who have historically expressed frustration with the old, messy codebase.
*   Involve them early in the framework design process. If they help design the module templates and dependency rules, they will feel ownership.
*   Assign these champions to the Pilot Project.
*   When the framework is rolled out to other teams, these champions act as mentors, code reviewers, and evangelists, explaining *why* the rules exist, rather than just acting as police.

## 4. Frictionless Onboarding

If setting up the new framework takes two days of fighting with CMake, compiler paths, and Python scripts, developers will hate it before they write a single line of C.

You must provide a "One-Click" onboarding experience.
*   Provide a **Template Repository** that contains the exact directory structure, a basic `main.c`, pre-configured CMake files, and an empty unit test setup.
*   Provide Docker containers or DevContainers that include the exact compiler versions, static analysis tools, and unit test frameworks required. The developer should only need to type `make` or `cmake --build` to see a successful, standard-compliant build immediately.

## 5. Documentation that Solves Problems

Do not write philosophical manifestos. Write practical guides.

Developers don't want to read a 10-page essay on Dependency Inversion. They want to search the wiki for "How do I add a new I2C sensor in the framework?"

Your documentation should consist primarily of **Cookbooks and Tutorials**:
*   "How to create a new Application Module."
*   "How to write an ISR safely."
*   "How to mock the SPI HAL for a unit test."

## Summary

Adoption is a sales process. You are selling your developers on a slightly slower, more verbose way of writing code (interfaces, mocks, initialization structs) in exchange for massive reductions in late-stage debugging, easier hardware migrations, and better sleep. Prove it works on a small scale, build advocates, and make the tooling frictionless.