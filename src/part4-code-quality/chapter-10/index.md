# Chapter 10 — Coding Standards for Embedded C
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


Welcome to Chapter 10: Establishing a Coding Standard. In embedded systems development, the difference between a prototype and a robust, production-ready device often boils down to how strictly the codebase is governed by rules. Without a coding standard, even the best-designed architecture can devolve into an unmaintainable "Big Ball of Mud."

## The Purpose of This Chapter

In this chapter, we will explore why coding standards are an absolute necessity in embedded C, particularly focusing on how to set up corporate guidelines that are scalable, enforceable, and practical. We will address the common misconception that coding standards are purely about formatting, and instead explore how they fundamentally contribute to the safety, reliability, and security of your embedded software.

## Chapter Breakdown

1. **[Why Coding Standards Exist](01-why-coding-standards-exist.md):** The psychological and technical reasons behind enforcing codebase uniformity, covering cognitive load reduction and the mitigation of C's notorious footguns.
2. **[Style vs. Correctness vs. Safety](02-style-correctness-safety.md):** Breaking down the three pillars of a coding standard. We'll differentiate between formatting rules (like brace placement), correctness rules (preventing logical errors), and safety rules (mitigating catastrophic failures).
3. **[MISRA, Barr-C, and Project Rules](03-misra-barr-c-project-rules.md):** A practical guide to leveraging industry standards like MISRA C:2012 and Barr-C:2018. We'll discuss when to adopt them wholesale, when to cherry-pick rules, and how to create your own project-specific guidelines.
4. **[Enforceable Rules via Tooling](04-enforceable-rules.md):** Why a rule is useless if it requires human intervention to enforce. We'll look at static analysis, formatting tools like `clang-format`, and continuous integration (CI) pipelines.

## The Goal

By the end of this chapter, you should be equipped with the knowledge needed to draft, debate, and deploy a comprehensive coding standard across your embedded engineering team. You will understand that the best coding standards are those that are invisible—enforced automatically by tooling, allowing developers to focus on architecture and business logic rather than arguing over indentation.

Let's begin with understanding [Why Coding Standards Exist](01-why-coding-standards-exist.md).
