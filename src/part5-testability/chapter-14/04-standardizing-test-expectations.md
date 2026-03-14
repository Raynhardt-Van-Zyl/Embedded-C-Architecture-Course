# Chapter 14.4: Standardizing Test Expectations

Writing unit tests is a technical exercise; establishing standard test expectations is an organizational imperative. A codebase may contain thousands of tests, but if the engineering organization lacks clear, enforceable, and technologically grounded standards regarding *what* gets tested, *how much* gets tested, and the *quality* of those tests, the test suite inevitably degrades. It becomes a patchwork of brittle assertions that developers learn to ignore, eventually leading to the abandonment of the testing culture entirely.

This section defines the rigorous expectations for test coverage, the application of Test-Driven Development (TDD), the precise guidelines for mocking boundaries, and the mandatory enforcement mechanisms within the Continuous Integration (CI) pipeline. These are not suggestions; they are the architectural laws of the 20-year company standard.

## 1. Code Coverage: Metrics and Compiler Mechanics

Code coverage is an essential diagnostic tool, provided it is used to discover *untested* code rather than serving as a gamified management target. To understand coverage, one must understand how tools like `gcov` (the GNU coverage testing tool) instrument the binary at the compiler level.

When you compile with `--coverage` (which implies `-fprofile-arcs -ftest-coverage` in GCC), the compiler alters the abstract syntax tree (AST) and inserts hidden counter variables into every basic block of the Control Flow Graph (CFG). As the test executes on the host, these counters increment. The compiler also generates `.gcno` files detailing the CFG structure, and runtime execution generates `.gcda` data files containing the counter values. Tools like `lcov` parse these to generate visual reports.

We standardize on three distinct levels of coverage based on module criticality:

### Statement Coverage (C0)
Statement coverage verifies that every individual line (or statement) of C code has been executed at least once during the test suite run.
- **The Metric:** 100% statement coverage is the absolute baseline expectation for all hardware-independent business logic (the host-testable `src/app/` domain).
- **The Reality Check:** Achieving 100% statement coverage does *not* prove the code is bug-free. It simply proves that no code is completely dead, and that the compiler's basic block instrumentation has triggered on every line. A function can have 100% statement coverage and still fail catastrophically on boundary conditions (e.g., passing `0` to a division operation).

### Branch Coverage (C1)
Branch coverage is far more rigorous. It measures whether every possible path of a control structure (e.g., both the `if (true)` and the `else (false)` branches) has been taken, including implicit branches.
- **The Compiler Reality:** In C, a simple `if (a && b)` contains hidden branches due to short-circuit evaluation. If `a` is false, `b` is never evaluated. Branch coverage instrumentation ensures that both the short-circuit path and the full evaluation path are executed.
- **The Metric:** A minimum of 90% branch coverage is mandated for application logic. For hardware drivers where physical error injection (e.g., forcing an I2C bus collision) is extremely difficult, 75% is acceptable, *provided* the missing coverage is explicitly documented via code comments as un-testable hardware fault paths.

### Modified Condition/Decision Coverage (MC/DC)
MC/DC is an exceptionally rigorous standard mandated by safety-critical certifications (ISO 26262 for automotive ASIL-D, DO-178C for avionics DAL-A). It ensures that every condition within a decision independently affects the decision's outcome.
- **The Standard:** MC/DC is mandatory *only* for modules explicitly designated as safety-critical functions. For standard commercial firmware, the computational overhead of generating MC/DC reports and the engineering time required to satisfy it is generally prohibitive. We enforce C1 Branch coverage as the standard proxy.

## 2. Test-Driven Development (TDD) as an Architectural Forcing Function

While Test-Driven Development (writing the failing test *before* the implementation) is often debated as a workflow preference, its architectural benefits in embedded C are undeniable. TDD is not merely a testing strategy; it is a **design strategy**.

### The Architectural Benefit
TDD forces the developer to become the *first user* of the code before they implement it. 
If an engineer attempts to write a test for a PID controller and realizes they cannot instantiate it without initializing the physical STM32 ADC registers, TDD exposes this tight coupling immediately. It forces the developer to design a "Test Seam" (an interface or function pointer array) *before* writing the tightly coupled code. 

**Anti-Pattern:** The "We will add tests later" excuse. When code is written without tests, it is almost always written tightly coupled to the hardware or global state. Writing tests "later" requires massive refactoring, which management rarely allocates time for. Therefore, the code remains untested forever.

## 3. Mocking Guidelines: The Perils of Over-Mocking

Mocking is the mechanism that enables host-based testing by faking hardware interactions, but it is a double-edged sword. Over-mocking leads to "brittle tests"—tests that break every time the internal implementation details of a function change, even if the external behavior remains correct.

### The "Echo Chamber" Anti-Pattern
Consider an application function that initializes three sensors. If a developer uses a framework like CMock to mock every single internal function call, the test looks like this:
```c
// ANTI-PATTERN: The Echo Chamber Test
void test_SensorInit(void) {
    MockSensorA_Init_ExpectAndReturn(true);
    MockSensorB_Init_ExpectAndReturn(true);
    MockSensorC_Init_ExpectAndReturn(true);
    
    System_InitSensors();
}
```
This test asserts nothing about the *behavior* or the *state* of the system. It merely echoes the C implementation line-by-line. If the developer decides to initialize Sensor C before Sensor B, the test fails, despite the system still functioning perfectly. This destroys developer trust in the test suite.

### The Rule of Boundary Mocking
**Mock interfaces, not internals.** 
If Module A (the App) calls Module B (a filter), and Module B calls Module C (the HAL), a unit test for Module A should ideally mock the boundary of the HAL (Module C). Do not mock Module B if it is a pure logic module. Compile A and B together and test them as a cohesive unit. You should only mock at the exact architectural boundaries where the code touches the physical world (the Hardware Abstraction Layer) or an external third-party closed-source library.

## 4. Continuous Integration (CI) Enforcement

Architectural standards are completely meaningless if they are not enforced automatically. Humans will bypass rules under the pressure of shipping deadlines. The Continuous Integration (CI) pipeline is the emotionless, ultimate arbiter of code quality.

**The Mandatory CI Pipeline Stages:**
1. **The Host Build:** The CI server must compile the entire `src/app/` directory and all tests natively for the host (x86_64) using GCC or Clang.
2. **Sanitizer Execution:** The compiled host binaries must execute with AddressSanitizer and UndefinedBehaviorSanitizer active. Any memory leak, out-of-bounds access, or undefined behavior immediately halts the build.
3. **Test Suite Execution:** Run the unit test framework (e.g., Unity, CppUTest). A single failing assertion fails the pipeline.
4. **The Coverage Ratchet:** The CI pipeline generates the `gcov`/`lcov` reports. The pipeline implements a "coverage ratchet"—it compares the new branch coverage percentage against the `main` branch. If the new Pull Request *lowers* the overall coverage percentage, the build **must fail**. This ensures coverage only ever moves in one direction: up.
5. **The Target Build:** Finally, the CI server cross-compiles the final `.elf` and `.hex` firmware for the target MCU using the embedded toolchain (e.g., `arm-none-eabi-gcc`) to verify that the application still links correctly against the physical BSP and HAL.

---

## Company Standard Rules

**Rule 14.4.1:** **The Branch Coverage Mandate.** All Pull Requests modifying code in the hardware-independent application domain (`src/app/`) must achieve a minimum of 90% C1 Branch Coverage on the modified lines before they can be merged.

**Rule 14.4.2:** **The Coverage Ratchet.** The Continuous Integration pipeline shall enforce a strict coverage ratchet mechanism. Any commit that decreases the overall branch coverage percentage of the codebase, even by 0.1%, shall trigger an automatic CI failure and block the merge.

**Rule 14.4.3:** **Atomic Commits for Tests and Code.** Unit tests must be committed in the exact same Pull Request as the implementation code they verify. "Test-only" follow-up PRs are prohibited. Code without tests is considered incomplete and unreleasable.

**Rule 14.4.4:** **Boundary Mocking Restriction.** Developers shall restrict the use of mock generation tools (CMock, FFF) strictly to the boundaries of the Hardware Abstraction Layer (HAL) or OS Abstraction Layer (OSAL). Mocking internal, pure-logic C functions within the application domain is an anti-pattern and is prohibited unless specifically justified by extreme algorithmic complexity.