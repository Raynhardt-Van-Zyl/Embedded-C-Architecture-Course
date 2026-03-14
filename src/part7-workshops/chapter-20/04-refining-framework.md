# Refining the Framework

A framework that is never refined based on reality will become a source of resentment rather than a tool for acceleration. After the initial trial phase (the Strangler Fig integration of Framework V1), you will inevitably encounter friction. The interfaces you designed in an ivory tower will clash with the harsh realities of the silicon.

This chapter details how to manage the lifecycle of the internal framework, handle feedback, and refine the architecture.

## The "Dictator" Anti-Pattern

**Anti-pattern:** The Software Architect designs the framework, forces it upon the embedded developers, and refuses to change the API when the developers complain that it is inefficient or fundamentally broken for a specific edge case.

**Rationale:** If the framework is too restrictive or inefficient (e.g., an SPI interface that only supports blocking reads when the team desperately needs DMA for a high-speed display), developers will simply hack around it. They will access registers directly to bypass the framework, defeating its entire purpose.

## Establishing the Architecture Review Board (ARB)

To prevent the framework from stagnating or becoming tyrannical, establish a lightweight Architecture Review Board.

This isn't a massive corporate bureaucracy. It is a bi-weekly 45-minute meeting with the senior developers who are *actually using the framework*.

**ARB Responsibilities:**
- Reviewing pull requests that modify core framework APIs.
- Discussing developer friction points.
- Approving the transition of a custom project module into a generalized framework module.

## Refining Interfaces (Real-World Example)

During Framework V1, you designed a simple SPI interface:

```c
// V1 Framework SPI
struct fw_spi_interface {
    fw_result_t (*transmit)(fw_spi_interface_t *self, const uint8_t *tx, uint16_t len);
};
```

**Feedback from the Team:** "We are implementing an e-ink display driver. The display requires sending 50KB of image data. The V1 `transmit` function is blocking. It stalls the CPU for milliseconds, causing us to miss BLE connection events. We need DMA."

**Refining to V2:** The ARB agrees. The interface must evolve to support asynchronous, non-blocking transfers.

```c
// V2 Framework SPI (Refined)
typedef void (*fw_spi_callback_t)(void *context, fw_result_t status);

struct fw_spi_interface {
    // Legacy blocking call preserved for simple sensors
    fw_result_t (*transmit_blocking)(fw_spi_interface_t *self, const uint8_t *tx, uint16_t len);
    
    // New asynchronous call using DMA/Interrupts
    fw_result_t (*transmit_async)(fw_spi_interface_t *self, const uint8_t *tx, uint16_t len, fw_spi_callback_t cb, void *ctx);
};
```

## Versioning the Framework

Once an internal framework is shared across multiple product lines, you cannot simply push breaking changes to the `main` branch. A change to the SPI interface might fix Product A's display but completely break Product B's build.

### Semantic Versioning (SemVer)

The framework must be treated as a software product in its own right, using Semantic Versioning (`MAJOR.MINOR.PATCH`).

- **PATCH (1.0.1):** Bug fixes inside a framework module (e.g., fixing a math error in a filter utility). Does not change any APIs. Safe to upgrade.
- **MINOR (1.1.0):** Adding new features in a backwards-compatible manner (e.g., adding a new `fw_i2c_scan()` function to the interface). Existing code still compiles.
- **MAJOR (2.0.0):** Breaking API changes (e.g., changing the signature of the core logging macro, or renaming `fw_result_t` errors). Products must plan a migration to upgrade.

## Architectural Rules for Refinement

1. **Rule of Upward Generalization:** Do not build a complex driver in the framework *first*. Build it in the specific product repository. Once a second product needs the same driver, extract it, generalize it, and promote it into the framework. (The Rule of Three).
2. **Rule of Non-Breaking Evolution:** Whenever possible, refine interfaces by adding *new* asynchronous or advanced function pointers to an interface struct, rather than modifying the signatures of existing, heavily used V1 functions.
3. **Rule of Version Pinning:** Every product repository must pin its framework dependency to a specific version tag (e.g., via Git Submodules `git checkout v1.2.0` or CMake FetchContent). "Tracking `main`" across multiple products is strictly prohibited as it guarantees broken builds.