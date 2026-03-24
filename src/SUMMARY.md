# Summary

[Introduction](./introduction.md)

---

# Part I — Foundations of Embedded Codebase Design

- [What Firmware Architecture Actually Is](./part1-foundations/chapter-01/index.md)
  - [What "architecture" means in embedded systems](./part1-foundations/chapter-01/01-what-architecture-means.md)
  - [Architecture vs implementation](./part1-foundations/chapter-01/02-architecture-vs-implementation.md)
  - [Why codebase structure matters in embedded C](./part1-foundations/chapter-01/03-why-codebase-structure-matters.md)
  - [Standardization as an engineering multiplier](./part1-foundations/chapter-01/04-standardization-as-multiplier.md)

- [Understanding the Shape of an Embedded System](./part1-foundations/chapter-02/index.md)
  - [Typical firmware layers](./part1-foundations/chapter-02/01-typical-firmware-layers.md)
  - [Responsibilities of each layer](./part1-foundations/chapter-02/02-responsibilities-of-each-layer.md)
  - [Control flow through the system](./part1-foundations/chapter-02/03-control-flow.md)
  - [Data flow through the system](./part1-foundations/chapter-02/04-data-flow.md)

---

# Part II — Modular Design and Interfaces

- [Designing Modules in Embedded C](./part2-modular-design/chapter-03/index.md)
  - [What a module is](./part2-modular-design/chapter-03/01-what-is-a-module.md)
  - [Cohesion and coupling](./part2-modular-design/chapter-03/02-cohesion-and-coupling.md)
  - [Public API vs private implementation](./part2-modular-design/chapter-03/03-public-api-vs-private-implementation.md)
  - [Defining module contracts](./part2-modular-design/chapter-03/04-defining-module-contracts.md)

- [Interfaces, Abstractions, and HAL Design](./part2-modular-design/chapter-04/index.md)
  - [Why abstractions matter in firmware](./part2-modular-design/chapter-04/01-why-abstractions-matter.md)
  - [Hardware Abstraction Layers](./part2-modular-design/chapter-04/02-hardware-abstraction-layers.md)
  - [Board Support Packages](./part2-modular-design/chapter-04/03-board-support-packages.md)
  - [Interface design patterns in C](./part2-modular-design/chapter-04/04-interface-design-patterns.md)
  - [Stable interfaces over changing implementations](./part2-modular-design/chapter-04/05-stable-interfaces.md)

- [Dependency Management](./part2-modular-design/chapter-05/index.md)
  - [Dependency direction](./part2-modular-design/chapter-05/01-dependency-direction.md)
  - [Avoiding cyclic dependencies](./part2-modular-design/chapter-05/02-avoiding-cyclic-dependencies.md)
  - [Compile-time vs link-time dependencies](./part2-modular-design/chapter-05/03-compile-time-vs-link-time.md)
  - [Configuration without chaos](./part2-modular-design/chapter-05/04-configuration-without-chaos.md)

---

# Part III — Execution Models and Behavioral Structure

- [Bare-Metal Superloop Architecture](./part3-execution-models/chapter-06/index.md)
  - [The superloop model](./part3-execution-models/chapter-06/01-superloop-model.md)
  - [Cooperative scheduling patterns](./part3-execution-models/chapter-06/02-cooperative-scheduling.md)
  - [Time management and tick-based design](./part3-execution-models/chapter-06/03-time-management.md)
  - [Common superloop failure modes](./part3-execution-models/chapter-06/04-superloop-failure-modes.md)

- [Interrupt Architecture](./part3-execution-models/chapter-07/index.md)
  - [What belongs in an ISR](./part3-execution-models/chapter-07/01-what-belongs-in-isr.md)
  - [ISR-to-main or ISR-to-task handoff patterns](./part3-execution-models/chapter-07/02-isr-handoff-patterns.md)
  - [Shared data between interrupt and non-interrupt context](./part3-execution-models/chapter-07/03-shared-data.md)
  - [Standardizing interrupt behavior](./part3-execution-models/chapter-07/04-standardizing-interrupts.md)

- [RTOS-Based Architecture](./part3-execution-models/chapter-08/index.md)
  - [When an RTOS is appropriate](./part3-execution-models/chapter-08/01-when-rtos-is-appropriate.md)
  - [Task ownership and boundaries](./part3-execution-models/chapter-08/02-task-ownership.md)
  - [Queues, semaphores, mutexes, and event groups](./part3-execution-models/chapter-08/03-synchronization-primitives.md)
  - [Priority inversion, deadlock, and starvation](./part3-execution-models/chapter-08/04-concurrency-hazards.md)
  - [Standardizing task design](./part3-execution-models/chapter-08/05-standardizing-task-design.md)

- [Event-Driven and State-Machine Design](./part3-execution-models/chapter-09/index.md)
  - [Event-driven firmware structure](./part3-execution-models/chapter-09/01-event-driven-structure.md)
  - [Finite state machines](./part3-execution-models/chapter-09/02-finite-state-machines.md)
  - [Hierarchical state machines](./part3-execution-models/chapter-09/03-hierarchical-state-machines.md)
  - [Translating behavior into code](./part3-execution-models/chapter-09/04-translating-behavior-into-code.md)

---

# Part IV — Code Quality, Standards, and Reviewability

- [Coding Standards for Embedded C](./part4-code-quality/chapter-10/index.md)
  - [Why coding standards exist](./part4-code-quality/chapter-10/01-why-coding-standards-exist.md)
  - [Style vs correctness vs safety rules](./part4-code-quality/chapter-10/02-style-correctness-safety.md)
  - [MISRA, Barr-C, and project-specific rules](./part4-code-quality/chapter-10/03-misra-barr-c-project-rules.md)
  - [Writing rules that can be enforced](./part4-code-quality/chapter-10/04-enforceable-rules.md)

- [API and Header Hygiene](./part4-code-quality/chapter-11/index.md)
  - [Header file responsibilities](./part4-code-quality/chapter-11/01-header-file-responsibilities.md)
  - [Include discipline](./part4-code-quality/chapter-11/02-include-discipline.md)
  - [Macros, inline functions, and constants](./part4-code-quality/chapter-11/03-macros-inline-constants.md)
  - [Namespacing conventions](./part4-code-quality/chapter-11/04-namespacing-conventions.md)

- [Error Handling and Defensive Design](./part4-code-quality/chapter-12/index.md)
  - [Error categories in embedded systems](./part4-code-quality/chapter-12/01-error-categories.md)
  - [Return codes, assertions, and fault handlers](./part4-code-quality/chapter-12/02-return-codes-assertions.md)
  - [Fail-safe vs fail-fast behavior](./part4-code-quality/chapter-12/03-fail-safe-vs-fail-fast.md)
  - [Observability and diagnostics](./part4-code-quality/chapter-12/04-observability-diagnostics.md)

- [Reviewable Embedded C](./part4-code-quality/chapter-13/index.md)
  - [What makes code easy to review](./part4-code-quality/chapter-13/01-what-makes-code-reviewable.md)
  - [Review checklists](./part4-code-quality/chapter-13/02-review-checklists.md)
  - [Architectural smells in firmware](./part4-code-quality/chapter-13/03-architectural-smells.md)

---

# Part V — Testability and Verification

- [Designing for Testability](./part5-testability/chapter-14/index.md)
  - [Why embedded code is often hard to test](./part5-testability/chapter-14/01-why-hard-to-test.md)
  - [Test seams in C](./part5-testability/chapter-14/02-test-seams.md)
  - [Host-based testing vs target testing](./part5-testability/chapter-14/03-host-vs-target-testing.md)
  - [Standardizing test expectations](./part5-testability/chapter-14/04-standardizing-test-expectations.md)

- [Static Analysis and Compliance Automation](./part5-testability/chapter-15/index.md)
  - [The role of static analysis](./part5-testability/chapter-15/01-role-of-static-analysis.md)
  - [Mapping standards to tools](./part5-testability/chapter-15/02-mapping-standards-to-tools.md)
  - [Warning policies and technical debt management](./part5-testability/chapter-15/03-warning-policies.md)
  - [Measuring codebase health](./part5-testability/chapter-15/04-measuring-codebase-health.md)

---

# Part VI — Building the Standardization Framework

- [Defining the Framework Scope](./part6-standardization/chapter-16/index.md)
  - [What the framework should control](./part6-standardization/chapter-16/01-what-framework-should-control.md)
  - [What should remain flexible](./part6-standardization/chapter-16/02-what-should-remain-flexible.md)
  - [Organizational constraints](./part6-standardization/chapter-16/03-organizational-constraints.md)

- [Creating the Standard Package](./part6-standardization/chapter-17/index.md)
  - [Codebase layout standard](./part6-standardization/chapter-17/01-codebase-layout-standard.md)
  - [Module template standard](./part6-standardization/chapter-17/02-module-template-standard.md)
  - [Interface and dependency rules](./part6-standardization/chapter-17/03-interface-dependency-rules.md)
  - [Concurrency and ISR rules](./part6-standardization/chapter-17/04-concurrency-isr-rules.md)
  - [Review and acceptance criteria](./part6-standardization/chapter-17/05-review-acceptance-criteria.md)

- [Rolling Out the Framework](./part6-standardization/chapter-18/index.md)
  - [Adoption strategy](./part6-standardization/chapter-18/01-adoption-strategy.md)
  - [Legacy migration](./part6-standardization/chapter-18/02-legacy-migration.md)
  - [Governance and exceptions](./part6-standardization/chapter-18/03-governance-exceptions.md)
  - [Keeping the framework alive](./part6-standardization/chapter-18/04-keeping-framework-alive.md)

---

# Part VII — Capstone Workshops

- [Example Architecture Patterns](./part7-workshops/chapter-19/index.md)
  - [Small bare-metal sensor node](./part7-workshops/chapter-19/01-bare-metal-sensor-node.md)
  - [RTOS-based connected device](./part7-workshops/chapter-19/02-rtos-connected-device.md)
  - [Driver stack with HAL and BSP separation](./part7-workshops/chapter-19/03-driver-stack-hal-bsp.md)
  - [Protocol stack organization](./part7-workshops/chapter-19/04-protocol-stack-organization.md)

- [Capstone: Drafting and Trialing a Standardization Framework](./part7-workshops/chapter-20/index.md)
  - [Assessing your current codebase](./part7-workshops/chapter-20/01-assessing-current-codebase.md)
  - [Writing version 1 of the framework](./part7-workshops/chapter-20/02-writing-framework-v1.md)
  - [Trialing the framework on a real module](./part7-workshops/chapter-20/03-trialing-framework.md)
  - [Refining the framework based on review feedback](./part7-workshops/chapter-20/04-refining-framework.md)

---

# Part VIII — System Boundaries and Delivery

- [Startup, Linker, and System Boundaries](./part8-system-boundaries/chapter-21/index.md)
  - [Reset flow and startup code](./part8-system-boundaries/chapter-21/01-reset-and-startup.md)
  - [Linker scripts and memory maps](./part8-system-boundaries/chapter-21/02-linker-scripts-and-memory-map.md)
  - [Bootloader and ABI boundaries](./part8-system-boundaries/chapter-21/03-bootloader-and-abi-boundaries.md)
  - [Map files and post-mortem analysis](./part8-system-boundaries/chapter-21/04-map-files-and-postmortem-analysis.md)

- [Target Debug, Integration, and HIL](./part9-validation-debug/chapter-22/index.md)
  - [Target debug workflow](./part9-validation-debug/chapter-22/01-target-debug-workflow.md)
  - [Integration and hardware-in-the-loop testing](./part9-validation-debug/chapter-22/02-integration-and-hil.md)
  - [Fault capture and triage](./part9-validation-debug/chapter-22/03-fault-capture-and-triage.md)
  - [Release gates for real firmware](./part9-validation-debug/chapter-22/04-release-gates.md)

- [Production Firmware Lifecycle](./part9-validation-debug/chapter-23/index.md)
  - [Non-volatile storage strategy](./part9-validation-debug/chapter-23/01-nonvolatile-storage-strategy.md)
  - [Watchdogs and recovery policy](./part9-validation-debug/chapter-23/02-watchdogs-and-recovery.md)
  - [Firmware update and rollback strategy](./part9-validation-debug/chapter-23/03-firmware-update-and-rollback.md)
  - [Security basics for fielded devices](./part9-validation-debug/chapter-23/04-security-basics.md)

---

# Appendix

- [Suggested Teaching Order](./appendix/suggested-teaching-order.md)
- [Recommended Starting Point](./appendix/recommended-starting-point.md)
