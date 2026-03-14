# 18.4 Keeping the framework alive

An embedded architecture framework is not a "fire and forget" project. It is a living, breathing product whose primary customers are the developers in your organization. If you stop maintaining it, it will quickly become obsolete, developers will fork it, and you will end up right back where you started: a fragmented ecosystem of custom scripts and incompatible modules.

To ensure the long-term survival and relevance of your company standards, you must treat the framework with the same rigor and continuous improvement as the products you sell to external customers.

## 1. Versioning and Releases

The framework (the core standard headers, HAL interfaces, OSAL, and templates) must be strictly versioned using **Semantic Versioning (SemVer)**.

*   `v1.2.0` -> `v1.2.1`: Bug fixes in the standard logging module. Completely safe for projects to upgrade.
*   `v1.2.0` -> `v1.3.0`: Added a new SPI DMA interface. Backward compatible; existing code won't break.
*   `v1.0.0` -> `v2.0.0`: Changed the signature of the primary `sys_err_t` error handler. **Breaking change.**

Projects should depend on specific versions of the framework (e.g., via Git submodules or a package manager like Conan). This prevents the dreaded scenario where an update to the core framework breaks the build of a 3-year-old product currently in maintenance mode.

## 2. The Feedback Loop (Post-Mortems)

The architecture should evolve based on reality, not theory. The best mechanism for evolving the framework is the **Project Post-Mortem**.

When a project ships (or fails), the team should review how the architecture performed:
*   Did the HAL abstraction leak too much hardware detail?
*   Was the strict ISR queue rule too slow for the motor control team?
*   Did the unit test framework actually catch bugs, or was it just a chore?

If a pattern emerges across multiple projects—for example, every team is struggling to use the standard I2C interface because it lacks support for repeated-starts—the ARB must prioritize updating the framework in the next minor release.

## 3. Deprecation Strategies

As the framework evolves, old ways of doing things will be replaced by better patterns. You cannot simply delete old interfaces in a `v2.0` release without warning, as this will blindside product teams.

Use formal deprecation cycles:
1.  **Mark as Deprecated:** Use compiler attributes (`__attribute__((deprecated("Use sys_timer_v2 instead")))`) to warn developers that an API is being phased out.
2.  **Provide Migration Guides:** Write clear documentation on how to transition from the old API to the new API.
3.  **Removal:** Only remove the old API in a major version bump, giving teams ample time to schedule the refactoring work.

## 4. Living Documentation

Documentation is the lifeblood of a framework. If a developer cannot figure out how to initialize the standard RTOS wrapper within 10 minutes, they will just call `xTaskCreate` directly and bypass the standard entirely.

Documentation must live alongside the code (e.g., Markdown files in the framework repository, generated into a static site using MkDocs or Sphinx). 
It must include:
*   **The "Why":** Explaining the reasoning behind the rules (e.g., "Why we ban dynamic memory").
*   **API References:** Doxygen-generated API specs.
*   **Examples:** Complete, compilable examples of how to set up a project, write a driver, and write a unit test.

## Conclusion

Building an expert-level embedded C architecture is an exercise in balancing opposing forces. You must balance the need for hardware-hugging performance with the need for platform-agnostic testability. You must balance rigid safety standards with developer velocity.

By understanding decoupling, exploiting dependency injection, rigorously testing on the host, and rolling out these concepts through a carefully governed internal framework, you elevate embedded software development from an artisanal, chaotic craft into a predictable, scalable engineering discipline.

This concludes the Embedded C Architecture course. You now have the blueprints to build robust, maintainable systems that will survive the test of time and changing hardware landscapes.