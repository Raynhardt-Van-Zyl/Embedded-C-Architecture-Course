---
name: embedded-course-architect
description: Audits and scaffolds code for the Embedded C Architecture Course. Use when checking for missing implementations, validating naming conventions, or generating C stubs from headers.
---

# Embedded Course Architect

This skill helps maintain the "Embedded C Architecture Course" repository by automating audits and code generation.

## Audit Workflow

To check the codebase for missing files, empty directories, and naming violations:

```bash
python scripts/audit_course.py
```

This will output a list of issues found in `code/` and `hardware/`.

## Scaffolding Workflow

To generate a `.c` implementation file from an existing `.h` header:

```bash
python scripts/scaffold_module.py <path/to/header.h>
```

This script extracts function prototypes and generates empty stubs with standard doxygen comments.

## Architectural Rules

When writing or reviewing code, consult the [Course Rules](references/course_rules.md).

Key highlights:
*   **No malloc/free**.
*   **Static allocation only**.
*   **Opaque pointers** for module context.
*   **PascalCase** for module functions (`HalGpio_Init`).
*   **snake_case** for variables.
