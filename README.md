# Embedded C Codebase Architecture Course

An mdbook project for teaching embedded C architecture and codebase standardization.

## Building the Book

```bash
mdbook build
```

## Serving Locally

```bash
mdbook serve --open
```

## Curriculum Overview

This curriculum teaches **embedded C architecture and codebase standardization** in a practical sequence. The goal is to understand how to structure, scale, review, and govern an embedded C codebase so that multiple engineers can work in it safely and consistently.

### Part I — Foundations of Embedded Codebase Design
- Chapter 1: What Firmware Architecture Actually Is
- Chapter 2: Understanding the Shape of an Embedded System

### Part II — Modular Design and Interfaces
- Chapter 3: Designing Modules in Embedded C
- Chapter 4: Interfaces, Abstractions, and HAL Design
- Chapter 5: Dependency Management

### Part III — Execution Models and Behavioral Structure
- Chapter 6: Bare-Metal Superloop Architecture
- Chapter 7: Interrupt Architecture
- Chapter 8: RTOS-Based Architecture
- Chapter 9: Event-Driven and State-Machine Design

### Part IV — Code Quality, Standards, and Reviewability
- Chapter 10: Coding Standards for Embedded C
- Chapter 11: API and Header Hygiene
- Chapter 12: Error Handling and Defensive Design
- Chapter 13: Reviewable Embedded C

### Part V — Testability and Verification
- Chapter 14: Designing for Testability
- Chapter 15: Static Analysis and Compliance Automation

### Part VI — Building the Standardization Framework
- Chapter 16: Defining the Framework Scope
- Chapter 17: Creating the Standard Package
- Chapter 18: Rolling Out the Framework

### Part VII — Applied Architecture Workshops
- Chapter 19: Example Architecture Patterns
- Chapter 20: Drafting Your Own Standardization Framework

## License

MIT License
