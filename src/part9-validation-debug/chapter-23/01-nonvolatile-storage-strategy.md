# 23.1 Non-volatile storage strategy

Non-volatile storage is not just a driver concern. It is a policy concern.

Architectural questions include:

- What data is configuration, telemetry, calibration, or crash evidence?
- How often is each class written?
- What corruption detection exists?
- What is the recovery path if storage is partially written or exhausted?

## Recommended patterns

- Separate frequently updated data from rarely updated manufacturing data
- Use versioned records with CRC or equivalent integrity checks
- Define explicit invalid-data behavior instead of assuming storage always reads back cleanly
- Budget flash endurance at the architecture stage, not after field failures appear

If your design writes counters or state on every boot without endurance analysis, the architecture is incomplete.
