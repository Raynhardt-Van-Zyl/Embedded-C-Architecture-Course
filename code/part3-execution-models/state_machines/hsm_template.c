#include "hsm_template.h"

/**
 * @file hsm_template.c
 * @brief Textbook Implementation of a Hierarchical State Machine (HSM)
 * 
 * This file contains the execution engine of the HSM. It manages the core
 * computer science algorithms required for a true hierarchical architecture:
 * 1. Event Propagation (Bubbling) up the topological graph.
 * 2. Deferred Transitions to satisfy the Run-To-Completion (RTC) semantic.
 * 3. Topological traversal to compute the Lowest Common Ancestor (LCA).
 * 4. Firing Exit/Entry execution paths across hierarchical boundaries.
 */

/* Maximum depth of the state hierarchy (to prevent stack allocation explosion) */
#define HSM_MAX_DEPTH 8

/* ========================================================================= */
/*                          CORE ENGINE METHODS                              */
/* ========================================================================= */

void* Hsm_TopState(Hsm *me, const HsmEvent *e) {
    (void)me;
    (void)e;
    /* The root state acts as the ultimate event sink and has no parent. */
    return NULL;
}

void Hsm_Ctor(Hsm *me, HsmStateHandler initial_state) {
    /* Step 1: Initialize the HSM structure. TopState is always the initial root. */
    me->current_state = Hsm_TopState;
    me->target_state = initial_state;
    
    HsmEvent empty_evt = { HSM_SIG_EMPTY };
    HsmEvent entry_evt = { HSM_SIG_ENTRY };
    HsmEvent init_evt  = { HSM_SIG_INIT  };
    
    /* Step 2: Dynamically discover the path from the target state up to TopState. */
    HsmStateHandler path[HSM_MAX_DEPTH];
    int depth = 0;
    HsmStateHandler temp = initial_state;
    
    /* Query the parent via HSM_SIG_EMPTY until we reach the root */
    while (temp != Hsm_TopState && temp != NULL && depth < HSM_MAX_DEPTH) {
        path[depth++] = temp;
        temp = (HsmStateHandler)temp(me, &empty_evt);
    }
    
    /* Step 3: Execute ENTRY actions top-down. 
     * By iterating backwards, we enter the most general super-state first,
     * down to the most specific sub-state. */
    for (int i = depth - 1; i >= 0; i--) {
        path[i](me, &entry_evt);
    }
    
    /* Step 4: Finalize the state pointer and execute the target's INIT action. */
    me->current_state = initial_state;
    me->target_state = NULL;
    
    me->current_state(me, &init_evt);
    
    /* Note: If the INIT action requested a transition to a sub-state (via Hsm_Transition),
     * it will be resolved on the very first Hsm_Dispatch call by the application.
     * This defers transition logic and prevents initial stack bloat. */
}

void Hsm_Transition(Hsm *me, HsmStateHandler target_state) {
    /* 
     * Run-To-Completion (RTC) Rationale:
     * A state handler must finish executing its logic before the actual
     * transition (EXIT/ENTRY sequences) occurs. By just recording the target,
     * we hand control back to the Dispatcher to gracefully orchestrate the change.
     */
    me->target_state = target_state;
}

void Hsm_Dispatch(Hsm *me, const HsmEvent *e) {
    HsmStateHandler s = me->current_state;
    me->target_state = NULL; /* Clear any lingering transition state */

    /* ===================================================================== */
    /* PHASE 1: EVENT BUBBLING                                               */
    /* ===================================================================== */
    /* 
     * We pass the event to the current state. If the state returns a pointer 
     * to its parent, it signals "I did not handle this event". We recursively
     * bubble up the tree until a super-state consumes the event (returns NULL).
     */
    while (s != NULL && s != Hsm_TopState) {
        HsmStateHandler parent = (HsmStateHandler)s(me, e);
        
        if (me->target_state != NULL) {
            /* A transition was fired during event handling. Stop bubbling. */
            break;
        }
        
        if (parent == NULL) {
            /* The event was fully handled without a transition. Stop bubbling. */
            break;
        }
        
        /* Proceed up the hierarchy chain. */
        s = parent;
    }

    /* ===================================================================== */
    /* PHASE 2: TRANSITION EXECUTION & LCA DISCOVERY                         */
    /* ===================================================================== */
    /* 
     * If me->target_state is non-NULL, a state transition was requested.
     * Because INIT actions can trigger nested transitions into sub-states,
     * we wrap the transition logic in a while loop to iteratively drain
     * all transition requests without risking deep recursive stack overflows.
     */
    while (me->target_state != NULL) {
        HsmStateHandler target = me->target_state;
        HsmStateHandler source = me->current_state;
        me->target_state = NULL; /* Clear to catch nested INIT transitions */
        
        HsmEvent empty_evt = { HSM_SIG_EMPTY };
        HsmEvent exit_evt  = { HSM_SIG_EXIT  };
        HsmEvent entry_evt = { HSM_SIG_ENTRY };

        HsmStateHandler src_path[HSM_MAX_DEPTH];
        HsmStateHandler tgt_path[HSM_MAX_DEPTH];
        int src_depth = 0;
        int tgt_depth = 0;

        /* Step A: Discover topological path to root for Source state */
        HsmStateHandler temp = source;
        while (temp != Hsm_TopState && temp != NULL && src_depth < HSM_MAX_DEPTH) {
            src_path[src_depth++] = temp;
            temp = (HsmStateHandler)temp(me, &empty_evt);
        }

        /* Step B: Discover topological path to root for Target state */
        temp = target;
        while (temp != Hsm_TopState && temp != NULL && tgt_depth < HSM_MAX_DEPTH) {
            tgt_path[tgt_depth++] = temp;
            temp = (HsmStateHandler)temp(me, &empty_evt);
        }

        /* Step C: Compute the Lowest Common Ancestor (LCA) */
        int lca_src_idx = -1;
        int lca_tgt_idx = -1;
        
        for (int i = 0; i < src_depth; i++) {
            for (int j = 0; j < tgt_depth; j++) {
                if (src_path[i] == tgt_path[j]) {
                    lca_src_idx = i;
                    lca_tgt_idx = j;
                    break;
                }
            }
            if (lca_src_idx != -1) break;
        }

        /* If no LCA was found (i.e. cross-tree transition), they share only TopState */
        if (lca_src_idx == -1) {
            lca_src_idx = src_depth;
            lca_tgt_idx = tgt_depth;
        }

        /* Step D: Execute EXIT actions from Source up to (but EXCLUDING) the LCA */
        for (int i = 0; i < lca_src_idx; i++) {
            src_path[i](me, &exit_evt);
        }

        /* Step E: Execute ENTRY actions from just below the LCA down to the Target */
        for (int i = lca_tgt_idx - 1; i >= 0; i--) {
            tgt_path[i](me, &entry_evt);
        }

        /* Step F: Commit the new state and fire the INIT action */
        me->current_state = target;
        
        HsmEvent init_evt = { HSM_SIG_INIT };
        me->current_state(me, &init_evt);
        
        /* 
         * If the INIT action called Hsm_Transition(), me->target_state will 
         * now be non-NULL, and the while loop will iterate again to resolve
         * the nested transition iteratively. 
         */
    }
}
