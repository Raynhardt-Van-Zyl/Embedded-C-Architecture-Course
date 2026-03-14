#ifndef HSM_TEMPLATE_H
#define HSM_TEMPLATE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file hsm_template.h
 * @brief Textbook Implementation of a Hierarchical State Machine (HSM)
 *
 * A Hierarchical State Machine (HSM) is a powerful formalism for event-driven
 * systems. Unlike flat Finite State Machines (FSMs) which suffer from "state 
 * explosion" due to repetitive transitions, HSMs introduce behavioral inheritance 
 * through Super-states (Composite States) and Sub-states.
 *
 * Computer Science Rationale:
 * - Liskov Substitution Principle applied to states: A sub-state inherits
 *   the behavior (event handling and transitions) of its super-state. If the
 *   sub-state does not explicitly handle an event, it "bubbles" up the hierarchy.
 *   This is formally known as "Programming by Difference."
 * - Topological Discovery: Instead of maintaining complex static metadata tables
 *   to define the tree hierarchy, this implementation uses dynamic discovery. 
 *   By passing a designated HSM_SIG_EMPTY event, a state handler simply returns 
 *   its parent's function pointer. This creates a highly decoupled, pure-C topology
 *   that strictly enforces the Directed Acyclic Graph (DAG) constraints of an HSM.
 * - Lowest Common Ancestor (LCA): During a transition between two arbitrary nodes
 *   in the state tree, the algorithm mathematically determines the LCA. It dynamically
 *   fires EXIT actions from the source node up to (but excluding) the LCA, and ENTRY
 *   actions from the LCA down to the target node. This guarantees strict hierarchy
 *   invariant preservation.
 */

/* ========================================================================= */
/*                              EVENT SYSTEM                                 */
/* ========================================================================= */

/**
 * @brief Predefined System Signals for the HSM Engine.
 * 
 * These signals are generated internally by the HSM engine to orchestrate
 * topology discovery and execution of entry/exit actions across the hierarchy.
 */
typedef enum {
    HSM_SIG_EMPTY = 0, /*!< Used by the dispatcher to discover a state's parent. */
    HSM_SIG_ENTRY,     /*!< Commands a state to execute its ENTRY action. */
    HSM_SIG_EXIT,      /*!< Commands a state to execute its EXIT action. */
    HSM_SIG_INIT,      /*!< Commands a state to take its initial transition. */
    HSM_SIG_USER       /*!< Application-specific signals start from this value. */
} HsmSignal;

/**
 * @brief Base Event Structure.
 * 
 * All events passed through the system must begin with this structure.
 * Applications can inherit from this struct to pass complex event payloads.
 * Example: `typedef struct { HsmEvent super; uint8_t key_code; } KeyEvent;`
 */
typedef struct HsmEvent {
    uint16_t sig; /*!< The signal identifier (e.g., HSM_SIG_USER + 1). */
} HsmEvent;

/* ========================================================================= */
/*                            CORE HSM ENGINE                                */
/* ========================================================================= */

/* Forward declaration of the HSM base structure */
typedef struct Hsm Hsm;

/**
 * @brief State Handler Function Pointer Type.
 * 
 * In this architecture, a state is represented purely as a function pointer.
 * 
 * @param me Pointer to the HSM instance (polymorphic context).
 * @param e Pointer to the event being dispatched.
 * @return void* A pointer to the parent state handler (bubbling), 
 *               or NULL if the event was completely handled.
 */
typedef void* (*HsmStateHandler)(Hsm *me, const HsmEvent *e);

/**
 * @brief The Hierarchical State Machine base class.
 * 
 * Application state machines inherit from this base class by placing it
 * as the first member of their derived struct.
 */
struct Hsm {
    HsmStateHandler current_state; /*!< Pointer to the currently active state function. */
    HsmStateHandler target_state;  /*!< Temporary pointer used during transitions. */
};

/* ========================================================================= */
/*                               PUBLIC API                                  */
/* ========================================================================= */

/**
 * @brief The absolute root of all state trees. 
 * 
 * Acts as the ultimate sink for all unhandled events, silently dropping them.
 * 
 * @param me Pointer to the HSM instance.
 * @param e Pointer to the event.
 * @return void* Always returns NULL, as it has no parent.
 */
void* Hsm_TopState(Hsm *me, const HsmEvent *e);

/**
 * @brief Initializes the HSM to its starting configuration.
 * 
 * Executes the initialization transition, drilling down through the state
 * hierarchy from the Top State to the provided initial state, firing all
 * relevant ENTRY actions and the initial INIT action.
 * 
 * @param me Pointer to the HSM instance.
 * @param initial_state Pointer to the starting state handler.
 */
void Hsm_Ctor(Hsm *me, HsmStateHandler initial_state);

/**
 * @brief Dispatches an external event to the HSM.
 * 
 * The core engine function. Routes the event to the currently active state.
 * If unhandled, bubbles the event up the hierarchy. If a transition was
 * triggered during event processing, mathematically resolves the LCA and
 * executes the appropriate EXIT/ENTRY action chain.
 * 
 * @param me Pointer to the HSM instance.
 * @param e Pointer to the event to process.
 */
void Hsm_Dispatch(Hsm *me, const HsmEvent *e);

/**
 * @brief Requests a state transition from within a state handler.
 * 
 * Marks the target state for a transition. The transition is deferred and
 * safely executed by the Dispatcher once the current event handler returns,
 * preventing deep stack recursion and preserving the run-to-completion semantic.
 * 
 * @param me Pointer to the HSM instance.
 * @param target_state Pointer to the destination state handler.
 */
void Hsm_Transition(Hsm *me, HsmStateHandler target_state);

#endif // HSM_TEMPLATE_H
