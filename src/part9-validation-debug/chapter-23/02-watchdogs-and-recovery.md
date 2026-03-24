# 23.2 Watchdogs and recovery policy

A watchdog is not a checkbox. It is a contract between the firmware and the system about what "healthy progress" means.

## Bad watchdog design

- feeding from one high-priority loop regardless of subsystem health
- disabling the watchdog during difficult debugging and forgetting to restore it
- resetting the product without preserving enough evidence to explain why

## Better watchdog policy

- define which subsystems must report health before a feed is allowed
- distinguish startup grace periods from steady-state health checks
- preserve reset reason and fault context
- define escalation behavior when repeated resets occur

Watchdogs should reinforce architecture. They should not hide bugs by endlessly rebooting without diagnosis.
