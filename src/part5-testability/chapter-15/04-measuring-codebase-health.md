# Chapter 15.4: Measuring Codebase Health

Architecture is not a static artifact created during a high-level design meeting at the beginning of a project; it is a living, breathing structure that must be actively, aggressively maintained. Without objective, quantifiable measurement, "software architecture" quickly devolves into subjective opinions and religious wars over styling preferences. 

If you do not measure the health of the codebase, entropy guarantees that its quality will degrade over time. To prevent this architectural decay, a 20-year embedded organization must systematically track quantitative metrics that reflect the true complexity, testability, and debt density of the system. 

This section details the critical metrics we track, the deep technical rationale behind why they matter at the execution level, how we visualize them, and how they drive empirical engineering decisions.

## 1. Core Health Metrics: The vital signs of Firmware

We rely exclusively on automated tools integrated into our CI pipeline to continuously track these fundamental health indicators.

### Cyclomatic Complexity (McCabe's Complexity)
Cyclomatic complexity, developed by Thomas J. McCabe in 1976, is a software metric used to indicate the complexity of a program. It mathematically measures the number of linearly independent paths through a function's source code. 
- **The Calculation:** Every `if`, `while`, `for`, `case`, `&&`, and `||` operator increments the complexity score by one. A straight-line function has a complexity of 1.
- **The Silicon Reality (Branch Predictor Thrashing):** High complexity doesn't just make code hard to read; it has physical performance implications. An ARM Cortex-M7 or Cortex-M33 utilizes speculative execution and branch prediction. A function with a cyclomatic complexity of 45 (a massive switch statement with nested loops) contains so many divergent branch targets that it actively thrashes the CPU's branch predictor, leading to frequent pipeline flushes and cache misses, destroying real-time performance predictability.
- **The Testability Reality:** A function with a complexity of 15 requires a minimum of 15 distinct unit test cases just to achieve Branch Coverage. A function with a complexity of 50 requires thousands of permutation tests to prove it is logically sound.
- **Action:** Our CI pipeline strictly enforces a maximum complexity score per function.

### Code Coverage Trends (The Ratchet)
As established in Chapter 14.4, we measure Statement (C0) and Branch (C1) coverage. 
- **The Metric:** We do not simply track the raw percentage (e.g., 85%). We track the *delta* (the change) of coverage over time on a per-commit basis.
- **The Rationale:** In a massive legacy codebase (see Chapter 18), reaching 90% coverage immediately is impossible. The goal is to stop the bleeding. 
- **Action:** The CI pipeline enforces a "Coverage Ratchet." Overall coverage percentage is mathematically prohibited from decreasing. Every new Pull Request must maintain or improve the baseline coverage of the `main` branch.

### Static Analysis Violation Density
We track the number of static analysis warnings (MISRA, `clang-tidy`, `cppcheck`) normalized per 1,000 Lines of Code (KLOC).
- **The Rationale:** While all *new* code must meet the strict Zero-Warning policy (Chapter 15.3), a legacy codebase often contains thousands of grandfathered violations. Tracking the density helps engineering leadership monitor the progress in cleaning up historical technical debt.
- **Action:** Violation density must trend downward month-over-month. Spikes in density indicate a catastrophic failure in the PR review process, developers bypassing the CI pipeline, or a misconfigured runner.

## 2. Dashboarding and Engineering Visibility

Metrics are completely useless if developers cannot see them, or if they have to dig through 500 lines of Jenkins console output to find them. 

**The Engineering Dashboard:**
We utilize centralized code quality platforms like SonarQube or custom Grafana dashboards fed by CI artifacts, directly integrated with our repository manager (GitLab/GitHub/Bitbucket).
- Every project repository features a mandatory health badge on its `README.md` indicating the current build status, branch coverage percentage, and the static analysis grade (A through F).
- Nightly builds execute deep, whole-program analysis reports showing trend lines for cyclomatic complexity, technical debt estimation (hours required to fix), and duplicated code blocks over the last 90 days. 

## 3. The Danger of Gamification

While metrics are crucial for objective measurement, they must be applied with profound architectural wisdom. When metrics become the absolute goal rather than an indicator of health, developers will engage in the "Gamification of Metrics."

### Anti-Pattern 1: The Meaningless Test
If a developer is forced to hit 90% coverage but is under extreme deadline pressure, they will write tests that invoke the function, provide mock inputs, and simply return, without ever asserting a single output or state change. The coverage tool records 100% execution, but the test provides zero safety net against regressions. 
- **The Defense:** Code review must scrutinize the *assertions* within a test just as heavily as the production code itself.

### Anti-Pattern 2: Artificial Fragmentation
If the CI pipeline fails a build because a function exceeds a cyclomatic complexity of 15, a frustrated developer might simply cut the function in half and create `process_data_part1()` and `process_data_part2()`. 
- **The Rationale:** This satisfies the automated complexity checker, but it actually *increases* the cognitive load on the next developer who has to understand why these two highly cohesive chunks of logic were arbitrarily split.
- **The Defense:** Refactoring for complexity requires extracting cohesive, independent sub-routines (e.g., extracting a state transition calculation into its own pure function), not simply slicing code with a chainsaw.

### Company Guidelines for Healthy Measurement:
1. **Metrics Inform, Humans Decide:** If a function inherently requires a high complexity—for example, a massive, auto-generated hardware dispatch switch statement handling 50 different CAN bus message IDs—the developer can request a formal exemption during the PR review. The architecture team must prioritize pragmatic engineering over dogmatic metric worship.
2. **Focus on the Trend, Not Just the Number:** Moving a chaotic, 10-year-old legacy codebase from 15% test coverage to 45% coverage over a year is a massive architectural triumph and should be celebrated, even if the ultimate goal is 90%.
3. **Quality over Quantity:** A codebase with 70% coverage featuring rigorous assertions, bounds checking, and mocked hardware failure injections (e.g., simulating an I2C bus lockup) is infinitely healthier and more robust than a codebase with 100% coverage comprised entirely of "happy-path" execution tests.

## Conclusion

Measuring codebase health provides the objective feedback loop necessary to validate our architectural decisions. By enforcing complexity limits, strictly tracking test coverage trends, and making static analysis a highly visible, uncompromising standard, we ensure that the firmware remains testable, maintainable, and robust throughout its entire 20-year lifecycle.

---

## Company Standard Rules

**Rule 15.4.1:** **Maximum Cyclomatic Complexity.** The maximum permitted cyclomatic complexity for any single C function is strictly capped at **15**. The CI pipeline shall automatically fail any Pull Request that introduces a new function, or modifies an existing one, resulting in a score exceeding this limit without a documented architectural exemption.

**Rule 15.4.2:** **The Trend Line Mandate.** Technical Debt (measured as static analysis violation density per KLOC) and Code Coverage percentage must be continuously tracked and visualized on the project dashboard. The CI pipeline must enforce that new merges never increase the debt density and never decrease the coverage percentage.

**Rule 15.4.3:** **Meaningful Test Assertions.** Reviewers must reject unit tests that achieve code coverage without asserting functional correctness. A unit test without a `TEST_ASSERT()` (or equivalent framework macro) verifying the output state, return value, or side effects is considered an invalid test.

**Rule 15.4.4:** **Exemption Documentation.** If a specific algorithm or hardware dispatcher genuinely requires exceeding the complexity threshold (e.g., an RTOS context switch handler written in assembly/C), it must be annotated with a specific suppression comment detailing why breaking the function into smaller pieces would degrade real-time performance or obfuscate the hardware intent.