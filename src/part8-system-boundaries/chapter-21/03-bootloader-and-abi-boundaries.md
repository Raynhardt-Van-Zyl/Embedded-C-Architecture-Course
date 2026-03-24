# 21.3 Bootloader and ABI boundaries

Many firmware failures are not logic bugs in the application. They are binary-interface mismatches between separately built components.

Typical embedded boundary examples:

- Bootloader calling fixed application entry points
- Application consuming bootloader services through a table of function pointers
- Retained SRAM structures shared across warm reset
- Dual-image update metadata stored at fixed addresses

## The architectural rule

If two binaries can be updated independently, their interface is an ABI boundary and must be treated as one.

That means:

- layout changes require versioning
- structure packing and alignment must be explicit
- endianness and width assumptions must be documented
- upgrade and rollback behavior must be defined before field deployment

## Stable boundary pattern

Prefer versioned interface blocks with:

- magic number
- version field
- size field
- reserved fields for future expansion
- explicit compatibility rules

This prevents accidental breakage when one side adds fields and assumes the other side was rebuilt.

## Review questions

- Can the application tolerate an older bootloader?
- Can the bootloader reject an incompatible image before jump?
- Are shared structs safe across compiler changes and optimization levels?
- Does rollback reuse the same retained metadata safely?
