# Summary

[Introduction](./introduction.md)

---

# Part I — Foundations of Embedded Codebase Design

- [Chapter 1 — What Firmware Architecture Actually Is](./part1-foundations/chapter-01/index.md)
  - [1.1 What "architecture" means in embedded systems](./part1-foundations/chapter-01/01-what-architecture-means.md)
  - [1.2 Architecture vs implementation](./part1-foundations/chapter-01/02-architecture-vs-implementation.md)
  - [1.3 Why codebase structure matters in embedded C](./part1-foundations/chapter-01/03-why-codebase-structure-matters.md)
  - [1.4 Standardization as an engineering multiplier](./part1-foundations/chapter-01/04-standardization-as-multiplier.md)

- [Chapter 2 — Understanding the Shape of an Embedded System](./part1-foundations/chapter-02/index.md)
  - [2.1 Typical firmware layers](./part1-foundations/chapter-02/01-typical-firmware-layers.md)
  - [2.2 Responsibilities of each layer](./part1-foundations/chapter-02/02-responsibilities-of-each-layer.md)
  - [2.3 Control flow through the system](./part1-foundations/chapter-02/03-control-flow.md)
  - [2.4 Data flow through the system](./part1-foundations/chapter-02/04-data-flow.md)

---

# Part II — Modular Design and Interfaces

- [Chapter 3 — Designing Modules in Embedded C](./part2-modular-design/chapter-03/index.md)
  - [3.1 What a module is](./part2-modular-design/chapter-03/01-what-is-a-module.md)
  - [3.2 Cohesion and coupling](./part2-modular-design/chapter-03/02-cohesion-and-coupling.md)
  - [3.3 Public API vs private implementation](./part2-modular-design/chapter-03/03-public-api-vs-private-implementation.md)
  - [3.4 Defining module contracts](./part2-modular-design/chapter-03/04-defining-module-contracts.md)

- [Chapter 4 — Interfaces, Abstractions, and HAL Design](./part2-modular-design/chapter-04/index.md)
  - [4.1 Why abstractions matter in firmware](./part2-modular-design/chapter-04/01-why-abstractions-matter.md)
  - [4.2 Hardware Abstraction Layers](./part2-modular-design/chapter-04/02-hardware-abstraction-layers.md)
  - [4.3 Board Support Packages](./part2-modular-design/chapter-04/03-board-support-packages.md)
  - [4.4 Interface design patterns in C](./part2-modular-design/chapter-04/04-interface-design-patterns.md)
  - [4.5 Stable interfaces over changing implementations](./part2-modular-design/chapter-04/05-stable-interfaces.md)

- [Chapter 5 — Dependency Management](./part2-modular-design/chapter-05/index.md)
  - [5.1 Dependency direction](./part2-modular-design/chapter-05/01-dependency-direction.md)
  - [5.2 Avoiding cyclic dependencies](./part2-modular-design/chapter-05/02-avoiding-cyclic-dependencies.md)
  - [5.3 Compile-time vs link-time dependencies](./part2-modular-design/chapter-05/03-compile-time-vs-link-time.md)
  - [5.4 Configuration without chaos](./part2-modular-design/chapter-05/04-configuration-without-chaos.md)

---

# Part III — Execution Models and Behavioral Structure

- [Chapter 6 — Bare-Metal Superloop Architecture](./part3-execution-models/chapter-06/index.md)
  - [6.1 The superloop model](./part3-execution-models/chapter-06/01-superloop-model.md)
  - [6.2 Cooperative scheduling patterns](./part3-execution-models/chapter-06/02-cooperative-scheduling.md)
  - [6.3 Time management and tick-based design](./part3-execution-models/chapter-06/03-time-management.md)
  - [6.4 Common superloop failure modes](./part3-execution-models/chapter-06/04-superloop-failure-modes.md)

- [Chapter 7 — Interrupt Architecture](./part3-execution-models/chapter-07/index.md)
  - [7.1 What belongs in an ISR](./part3-execution-models/chapter-07/01-what-belongs-in-isr.md)
  - [7.2 ISR-to-main or ISR-to-task handoff patterns](./part3-execution-models/chapter-07/02-isr-handoff-patterns.md)
  - [7.3 Shared data between interrupt and non-interrupt context](./part3-execution-models/chapter-07/03-shared-data.md)
  - [7.4 Standardizing interrupt behavior](./part3-execution-models/chapter-07/04-standardizing-interrupts.md)

- [Chapter 8 — RTOS-Based Architecture](./part3-execution-models/chapter-08/index.md)
  - [8.1 When an RTOS is appropriate](./part3-execution-models/chapter-08/01-when-rtos-is-appropriate.md)
  - [8.2 Task ownership and boundaries](./part3-execution-models/chapter-08/02-task-ownership.md)
  - [8.3 Queues, semaphores, mutexes, and event groups](./part3-execution-models/chapter-08/03-synchronization-primitives.md)
  - [8.4 Priority inversion, deadlock, and starvation](./part3-execution-models/chapter-08/04-concurrency-hazards.md)
  - [8.5 Standardizing task design](./part3-execution-models/chapter-08/05-standardizing-task-design.md)

- [Chapter 9 — Event-Driven and State-Machine Design](./part3-execution-models/chapter-09/index.md)
  - [9.1 Event-driven firmware structure](./part3-execution-models/chapter-09/01-event-driven-structure.md)
  - [9.2 Finite state machines](./part3-execution-models/chapter-09/02-finite-state-machines.md)
  - [9.3 Hierarchical state machines](./part3-execution-models/chapter-09/03-hierarchical-state-machines.md)
  - [9.4 Translating behavior into code](./part3-execution-models/chapter-09/04-translating-behavior-into-code.md)

---

# Part IV — Code Quality, Standards, and Reviewability

- [Chapter 10 — Coding Standards for Embedded C](./part4-code-quality/chapter-10/index.md)
  - [10.1 Why coding standards exist](./part4-code-quality/chapter-10/01-why-coding-standards-exist.md)
  - [10.2 Style vs correctness vs safety rules](./part4-code-quality/chapter-10/02-style-correctness-safety.md)
  - [10.3 MISRA, Barr-C, and project-specific rules](./part4-code-quality/chapter-10/03-misra-barr-c-project-rules.md)
  - [10.4 Writing rules that can be enforced](./part4-code-quality/chapter-10/04-enforceable-rules.md)

- [Chapter 11 — API and Header Hygiene](./part4-code-quality/chapter-11/index.md)
  - [11.1 Header file responsibilities](./part4-code-quality/chapter-11/01-header-file-responsibilities.md)
  - [11.2 Include discipline](./part4-code-quality/chapter-11/02-include-discipline.md)
  - [11.3 Macros, inline functions, and constants](./part4-code-quality/chapter-11/03-macros-inline-constants.md)
  - [11.4 Namespacing conventions](./part4-code-quality/chapter-11/04-namespacing-conventions.md)

- [Chapter 12 — Error Handling and Defensive Design](./part4-code-quality/chapter-12/index.md)
  - [12.1 Error categories in embedded systems](./part4-code-quality/chapter-12/01-error-categories.md)
  - [12.2 Return codes, assertions, and fault handlers](./part4-code-quality/chapter-12/02-return-codes-assertions.md)
  - [12.3 Fail-safe vs fail-fast behavior](./part4-code-quality/chapter-12/03-fail-safe-vs-fail-fast.md)
  - [12.4 Observability and diagnostics](./part4-code-quality/chapter-12/04-observability-diagnostics.md)

- [Chapter 13 — Reviewable Embedded C](./part4-code-quality/chapter-13/index.md)
  - [13.1 What makes code easy to review](./part4-code-quality/chapter-13/01-what-makes-code-reviewable.md)
  - [13.2 Review checklists](./part4-code-quality/chapter-13/02-review-checklists.md)
  - [13.3 Architectural smells in firmware](./part4-code-quality/chapter-13/03-architectural-smells.md)

---

# Part V — Testability and Verification

- [Chapter 14 — Designing for Testability](./part5-testability/chapter-14/index.md)
  - [14.1 Why embedded code is often hard to test](./part5-testability/chapter-14/01-why-hard-to-test.md)
  - [14.2 Test seams in C](./part5-testability/chapter-14/02-test-seams.md)
  - [14.3 Host-based testing vs target testing](./part5-testability/chapter-14/03-host-vs-target-testing.md)
  - [14.4 Standardizing test expectations](./part5-testability/chapter-14/04-standardizing-test-expectations.md)

- [Chapter 15 — Static Analysis and Compliance Automation](./part5-testability/chapter-15/index.md)
  - [15.1 The role of static analysis](./part5-testability/chapter-15/01-role-of-static-analysis.md)
  - [15.2 Mapping standards to tools](./part5-testability/chapter-15/02-mapping-standards-to-tools.md)
  - [15.3 Warning policies and technical debt management](./part5-testability/chapter-15/03-warning-policies.md)
  - [15.4 Measuring codebase health](./part5-testability/chapter-15/04-measuring-codebase-health.md)

---

# Part VI — Building the Standardization Framework

- [Chapter 16 — Defining the Framework Scope](./part6-standardization/chapter-16/index.md)
  - [16.1 What the framework should control](./part6-standardization/chapter-16/01-what-framework-should-control.md)
  - [16.2 What should remain flexible](./part6-standardization/chapter-16/02-what-should-remain-flexible.md)
  - [16.3 Organizational constraints](./part6-standardization/chapter-16/03-organizational-constraints.md)

- [Chapter 17 — Creating the Standard Package](./part6-standardization/chapter-17/index.md)
  - [17.1 Codebase layout standard](./part6-standardization/chapter-17/01-codebase-layout-standard.md)
  - [17.2 Module template standard](./part6-standardization/chapter-17/02-module-template-standard.md)
  - [17.3 Interface and dependency rules](./part6-standardization/chapter-17/03-interface-dependency-rules.md)
  - [17.4 Concurrency and ISR rules](./part6-standardization/chapter-17/04-concurrency-isr-rules.md)
  - [17.5 Review and acceptance criteria](./part6-standardization/chapter-17/05-review-acceptance-criteria.md)

- [Chapter 18 — Rolling Out the Framework](./part6-standardization/chapter-18/index.md)
  - [18.1 Adoption strategy](./part6-standardization/chapter-18/01-adoption-strategy.md)
  - [18.2 Legacy migration](./part6-standardization/chapter-18/02-legacy-migration.md)
  - [18.3 Governance and exceptions](./part6-standardization/chapter-18/03-governance-exceptions.md)
  - [18.4 Keeping the framework alive](./part6-standardization/chapter-18/04-keeping-framework-alive.md)

---

# Part VII — Applied Architecture Workshops

- [Chapter 19 — Example Architecture Patterns](./part7-workshops/chapter-19/index.md)
  - [19.1 Small bare-metal sensor node](./part7-workshops/chapter-19/01-bare-metal-sensor-node.md)
  - [19.2 RTOS-based connected device](./part7-workshops/chapter-19/02-rtos-connected-device.md)
  - [19.3 Driver stack with HAL and BSP separation](./part7-workshops/chapter-19/03-driver-stack-hal-bsp.md)
  - [19.4 Protocol stack organization](./part7-workshops/chapter-19/04-protocol-stack-organization.md)

- [Chapter 20 — Drafting Your Own Standardization Framework](./part7-workshops/chapter-20/index.md)
  - [20.1 Assessing your current codebase](./part7-workshops/chapter-20/01-assessing-current-codebase.md)
  - [20.2 Writing version 1 of the framework](./part7-workshops/chapter-20/02-writing-framework-v1.md)
  - [20.3 Trialing the framework on a real module](./part7-workshops/chapter-20/03-trialing-framework.md)
  - [20.4 Refining the framework based on review feedback](./part7-workshops/chapter-20/04-refining-framework.md)

---

# Appendix

- [Suggested Teaching Order](./appendix/suggested-teaching-order.md)
- [How to Use This Curriculum With LLMs](./appendix/how-to-use-with-llm.md)
- [Recommended Starting Point](./appendix/recommended-starting-point.md)
