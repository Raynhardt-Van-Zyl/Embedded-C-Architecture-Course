#ifndef LEGACY_ADAPTER_H
#define LEGACY_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file legacy_adapter.h
 * @brief The Strangler Fig Architectural Pattern for Legacy Code Migration
 *
 * The "Strangler Fig" pattern (coined by Martin Fowler) is a strategy for safely
 * and incrementally rewriting a legacy monolithic application. Rather than attempting
 * a high-risk "Big Bang" rewrite, new features are implemented in a modern architecture.
 * An "Adapter" or "Facade" layer acts as the Strangler Fig vine, wrapping the legacy
 * system. 
 *
 * Computer Science Rationale:
 * - Anti-Corruption Layer (ACL): Prevents the "toxic" legacy paradigms (e.g., global
 *   flags, blocking super-loops, coupled data structures) from leaking into the pristine
 *   modern codebase.
 * - Seam Injection: We intercept the legacy execution path (e.g., overriding the main
 *   super-loop call) to create a "seam". This seam allows modern background routing
 *   to execute transparently alongside the legacy code.
 * - Polymorphic Routing & Feature Toggling: Abstract interfaces in the modern system 
 *   can be dynamically routed to either the new HAL or the legacy drivers. This 
 *   facilitates Canary Releases, A/B testing, and instantaneous rollback if the modern
 *   implementation fails.
 */

/* ========================================================================= */
/*                   MODERN INTERFACES (CLEAN ABSTRACTIONS)                  */
/* ========================================================================= */

/**
 * @brief Send data over UART (Modern clean API).
 * 
 * New architectural modules will call this strictly typed, non-blocking interface.
 * Behind the scenes, the Adapter seamlessly routes the payload to the legacy 
 * serial driver until the modern DMA/RTOS HAL is fully validated.
 * 
 * @param data Pointer to the raw byte buffer to transmit.
 * @param length The exact length of the payload in bytes.
 */
void Modern_Uart_Transmit(const uint8_t *data, uint16_t length);

/**
 * @brief Update system state (Modern Event-Driven API).
 * 
 * Completely abstracts away the manipulation of the legacy 'system_state_flag'.
 * When the legacy system is finally fully strangled (deleted), this function's
 * implementation will gracefully migrate to dispatching an RTOS Event Group
 * or an HSM Signal without requiring a single line of code change in the calling modules.
 * 
 * @param new_state The strictly defined enum or flag representation of the new state.
 */
void Modern_System_SetState(uint32_t new_state);

/* ========================================================================= */
/*                     LEGACY INTERCEPTS (THE SEAMS)                         */
/* ========================================================================= */

/**
 * @brief Legacy Super-Loop Intercept / Migration Tick.
 * 
 * This function entirely replaces the raw call to `legacy_do_everything()` 
 * inside the legacy main() loop. It gives the modern RTOS environment a chance 
 * to execute its routing adapters, process queues, and handle HAL interrupts 
 * before yielding execution time back to the toxic legacy monolithic function.
 * 
 * As features are incrementally extracted from the legacy monolith into 
 * modern RTOS tasks, `legacy_do_everything()` will shrink, yielding more
 * CPU time to the modern system, until the legacy loop can be entirely removed.
 */
void LegacyAdapter_Tick(void);

#endif // LEGACY_ADAPTER_H
