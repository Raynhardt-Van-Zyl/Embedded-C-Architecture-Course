# Embedded C Codebase Architecture Course

An `mdBook` project for teaching embedded C architecture, codebase standardization, and the practical system-boundary work that sits between clean code and shippable firmware.

## Building the Book

```bash
mdbook build
```

## Serving Locally

```bash
mdbook serve --open
```

## Curriculum Overview

This curriculum teaches embedded C architecture in a senior-engineer sequence. The goal is to help teams structure, scale, review, validate, and ship embedded firmware safely across multiple products and hardware revisions.

### Part I — Foundations of Embedded Codebase Design
- Chapter 1 — What Firmware Architecture Actually Is
- Chapter 2 — Understanding the Shape of an Embedded System

### Part II — Modular Design and Interfaces
- Chapter 3 — Designing Modules in Embedded C
- Chapter 4 — Interfaces, Abstractions, and HAL Design
- Chapter 5 — Dependency Management

### Part III — Execution Models and Behavioral Structure
- Chapter 6 — Bare-Metal Superloop Architecture
- Chapter 7 — Interrupt Architecture
- Chapter 8 — RTOS-Based Architecture
- Chapter 9 — Event-Driven and State-Machine Design

### Part IV — Code Quality, Standards, and Reviewability
- Chapter 10 — Coding Standards for Embedded C
- Chapter 11 — API and Header Hygiene
- Chapter 12 — Error Handling and Defensive Design
- Chapter 13 — Reviewable Embedded C

### Part V — Testability and Verification
- Chapter 14 — Designing for Testability
- Chapter 15 — Static Analysis and Compliance Automation

### Part VI — Building the Standardization Framework
- Chapter 16 — Defining the Framework Scope
- Chapter 17 — Creating the Standard Package
- Chapter 18 — Rolling Out the Framework

### Part VII — Capstone Workshops
- Chapter 19 — Example Architecture Patterns
- Chapter 20 — Capstone: Drafting and Trialing a Standardization Framework

### Part VIII — System Boundaries and Delivery
- Chapter 21 — Startup, Linker, and System Boundaries
- Chapter 22 — Target Debug, Integration, and HIL
- Chapter 23 — Production Firmware Lifecycle

## Maintenance Audit Tool

Run the course audit before publishing updates:

```bash
python tools/course_audit.py
```

## Companion Material

The repository still includes optional companion material such as the LLM guidance, but it is no longer part of the main book flow.

## License

MIT License
