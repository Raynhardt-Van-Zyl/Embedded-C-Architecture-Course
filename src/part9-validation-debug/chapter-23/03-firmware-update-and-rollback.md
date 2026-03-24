# 23.3 Firmware update and rollback strategy

Update strategy becomes architecture the moment a device can change in the field.

## Questions to answer before shipping

- Is the update single-slot or dual-slot?
- What happens if power fails mid-update?
- Who validates image integrity and authenticity?
- Can the device roll back automatically?
- What metadata is shared between bootloader and application?

## Engineering rules

- Update state must be explicit and recoverable after reset
- Incompatible images must fail closed before jump
- Rollback conditions must be objective, not manual guesswork
- The build and release process must produce the metadata required by the boot path

An update mechanism that works only in the lab is not a production update mechanism.
