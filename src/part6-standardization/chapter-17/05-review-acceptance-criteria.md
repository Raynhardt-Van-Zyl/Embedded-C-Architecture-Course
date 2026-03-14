# 17.5 Review and acceptance criteria

Establishing codebase layouts, module templates, and dependency rules is useless if developers can simply ignore them and merge non-compliant code into the main branch. 

To transform a standard from a "suggestion" into a "framework," you must implement automated and manual gates. These gates are the **Review and Acceptance Criteria**. They ensure that every Pull Request (PR) adheres to the architecture before it becomes part of the product.

## 1. Automated Gates (Continuous Integration)

Humans are bad at catching missing `#include` guards, inconsistent indentation, or subtle MISRA C violations. These checks must be offloaded to machines. A CI server (like GitHub Actions, GitLab CI, or Jenkins) should automatically run on every PR.

If the CI pipeline fails, the PR cannot be merged. No exceptions.

**Mandatory CI Checks for the Standard Package:**

*   **Formatting and Style:** Run `clang-format` or `uncrustify` against the codebase. If the code deviates from the `.clang-format` standard, fail the build. Do not argue about bracket placement in PR reviews; let the tool reject it.
*   **Static Analysis (Linting):** Run tools like `cppcheck`, `clang-tidy`, or commercial tools like PC-lint. The framework should provide a standard configuration file (e.g., `.clang-tidy`) that enforces rules like "No dynamic memory allocation" and "Variables must be initialized."
*   **Unit Tests & Coverage:** 
    *   The code must compile for the host PC.
    *   All unit tests must pass.
    *   **Coverage Rule:** The framework should set a strict line/branch coverage minimum (e.g., 85%). If the PR drops the overall coverage below this threshold, it is rejected.
*   **Architectural Boundary Checks:** Use tools or custom Python scripts to verify `include` paths. For example, a script that scans `src/app/` and fails the build if it finds `#include "stm32f4xx.h"`.
*   **Cyclomatic Complexity:** Run metrics tools (like `lizard`) to ensure functions aren't too complex. Set a hard limit (e.g., a cyclomatic complexity of > 15 fails the build), forcing developers to break down massive functions.

## 2. Manual Gates (Peer Review)

Automation catches syntax and localized logic errors, but it cannot catch architectural violations or poor design. This requires human Peer Review.

To standardize the review process, the framework must provide a **Pull Request Template**. When a developer opens a PR, this template forces them to explicitly state how they complied with the architecture.

### Standard PR Checklist Template

```markdown
## Description
(Briefly describe what this PR does and why it is necessary)

## Architectural Compliance (Check all that apply)
- [ ] No new compiler warnings were introduced.
- [ ] Opaque pointers were used for new module state structs.
- [ ] No hardware-specific headers (BSP/HAL) are included in the `src/app` layer.
- [ ] All functions capable of failing return `sys_err_t` and are handled by the caller.
- [ ] All new blocking calls include a timeout parameter.
- [ ] Global variables are NOT used for cross-task communication (Queues/Notifications used instead).

## Testing Performed
- [ ] Unit tests written and passing locally.
- [ ] Hardware integration test performed on target (Specify target board).

## Reviewer Focus
(Are there specific areas of complex logic or concurrency the reviewer should look at closely?)
```

## 3. The Role of the Architect in Reviews

During the initial rollout of a new framework, the architect (or senior engineers driving the standard) must aggressively police the PRs. 

*   **Be ruthless about the interfaces:** If a developer proposes a PR that bypasses the HAL to toggle a GPIO directly in the application code "because it's faster," the architect must reject the PR and instruct them to use the standard dependency injection model.
*   **Be lenient on internal logic:** As discussed in Chapter 16, do not nitpick the internal `static` functions of a module unless there is a glaring bug. Focus on the public APIs and architectural boundaries.

## Conclusion

Acceptance criteria are the teeth of your framework. By automating the mundane checks (formatting, linting, tests) and using templates to focus the human reviews on architectural compliance (dependency injection, concurrency rules), you create a self-sustaining system of quality control. This guarantees the codebase remains clean and modular as it scales over years of development.