# Chapter 22 — Target Debug, Integration, and HIL
## Who This Chapter Is For

- Embedded C engineers responsible for bring-up, integration, or hardware validation
- Technical leads defining debug, lab, and release workflows

## Prerequisites

- Familiarity with C syntax and embedded build/debug workflows
- Comfortable with host testing, static analysis, and layered firmware design

## Learning Objectives

- Build a repeatable target-debug workflow instead of relying on ad hoc bench debugging
- Distinguish host tests, target tests, and hardware-in-the-loop responsibilities
- Define release gates that acknowledge real hardware and system-integration risk

## Key Terms

- HIL
- Smoke test
- Fault triage

## Practical Checkpoint

- Define a minimum bring-up checklist and a minimum HIL smoke suite for one target
- Document the exact artifacts required when a firmware fault is escalated from the lab

## What to Read Next

- Continue through this chapter, then proceed to Chapter 23 for field-lifecycle concerns such as NVM, watchdogs, updates, and security.


Host-based testing is essential, but it is not enough. Real embedded products still fail at the boundaries between toolchain, silicon, board design, timing, and physical environment. This chapter covers the workflows that catch those failures early and diagnose them consistently.
