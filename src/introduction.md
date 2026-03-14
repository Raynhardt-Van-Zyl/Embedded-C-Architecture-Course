# Embedded C Codebase Architecture Curriculum

## Purpose

This curriculum is designed to teach **embedded C architecture and codebase standardization** in a practical sequence. The goal is not merely to write working firmware, but to understand how to structure, scale, review, and govern an embedded C codebase so that multiple engineers can work in it safely and consistently.

The curriculum is arranged so that each chapter can be taught independently, while still building toward a complete standardization framework for a professional firmware codebase.

## Who This Curriculum Is For

- Embedded software engineers moving from individual contributor roles to team lead or architect positions
- Engineering managers responsible for code quality and team productivity
- Teams establishing or improving their embedded development standards
- Anyone seeking to understand how professional embedded codebases are structured and maintained

## How to Use This Book

Each chapter builds on concepts from previous chapters, but can also be read independently. The suggested teaching order in the appendix provides a recommended path through the material.

Code examples are provided in the `code/` directory, organized by part and chapter. These examples demonstrate the concepts discussed and can serve as starting templates for your own projects.

## Part Overview

### Part I — Foundations of Embedded Codebase Design
Establishes the core concepts of firmware architecture, layering, and system structure.

### Part II — Modular Design and Interfaces
Covers module design, HAL/BSP abstraction, and dependency management.

### Part III — Execution Models and Behavioral Structure
Explores superloop, interrupt, RTOS, and state machine architectures.

### Part IV — Code Quality, Standards, and Reviewability
Addresses coding standards, API design, error handling, and code review practices.

### Part V — Testability and Verification
Focuses on designing for testability and using static analysis tools.

### Part VI — Building the Standardization Framework
Provides practical guidance on creating and rolling out organizational standards.

### Part VII — Applied Architecture Workshops
Offers hands-on examples and guidance for creating your own framework.

---

## References

- [Barr Group Embedded C Coding Standard](https://barrgroup.com/embedded-systems/books/embedded-c-coding-standard) - A practical coding standard developed to minimize bugs in firmware
- [MISRA C Guidelines](https://www.misra.org.uk) - The de facto standard for developing software in C where safety, security, and code quality are important
- [Embedded.com](https://embedded.com) - Industry news and technical articles on embedded systems development
