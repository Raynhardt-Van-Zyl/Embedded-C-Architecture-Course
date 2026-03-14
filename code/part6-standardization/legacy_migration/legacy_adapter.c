#include "legacy_adapter.h"
#include <string.h>

/**
 * @file legacy_adapter.c
 * @brief Implementation of the Strangler Fig Legacy Migration Pattern
 * 
 * Contains the concrete boundary logic that bridges the pristine modern RTOS/HAL
 * domain with the toxic legacy spaghetti code domain. It houses the Anti-Corruption
 * Layer (ACL) type conversions, Feature Toggles, and Execution Seams.
 */

/* ========================================================================= */
/*                        MOCK LEGACY SYSTEM DOMAIN                          */
/* ========================================================================= */
/* 
 * WARNING: The following variables and functions represent the legacy codebase.
 * In a real system, these would be extern'd from legacy headers. 
 * They embody anti-patterns: global mutable state, lack of const correctness,
 * implicit string lengths, and blocking behavior.
 */

/* The dreaded unprotected global state variable modified concurrently by everything. */
uint32_t system_state_flag = 0; 

/**
 * @brief Toxic Legacy Blocking Serial Transmission
 * @param str Implicitly null-terminated string (unsafe).
 */
void legacy_transmit_serial(char *str) {
    /* Mock: Blocking hardware manipulation that burns CPU cycles. */
    (void)str;
}

/**
 * @brief The Monolithic God Function
 * A massive 5,000-line function that interleaves UI rendering, comms parsing, 
 * motor control, and watchdog kicking without any separation of concerns.
 */
void legacy_do_everything(void) {
    /* Mock: Tangled execution of the old system. */
}


/* ========================================================================= */
/*                        MOCK MODERN SYSTEM DOMAIN                          */
/* ========================================================================= */
/*
 * The following represents the new, pristine RTOS and HAL layers.
 * They use explicit lengths, const correctness, and non-blocking semantics.
 */

typedef struct { int dummy; } rtos_queue_t;

/**
 * @brief New non-blocking DMA UART transmission (Fast & Clean).
 */
void hal_uart_send_dma(const uint8_t *data, uint16_t len) {
    /* Mock: Enqueues data into a DMA descriptor ring buffer. */
    (void)data;
    (void)len;
}

/**
 * @brief Modern RTOS queue enqueue function.
 */
bool rtos_queue_send(rtos_queue_t *q, const void *data, uint32_t timeout) {
    /* Mock: Thread-safe queue operation. */
    (void)q; (void)data; (void)timeout;
    return true;
}


/* ========================================================================= */
/*                   STRANGLER FIG ADAPTER IMPLEMENTATION                    */
/* ========================================================================= */

/**
 * @brief Feature Toggle for Canary Testing.
 * 
 * If true, traffic routes to the new DMA HAL. If false, gracefully falls back 
 * to the legacy blocking implementation. This allows risk-free deployment of 
 * the new HAL. If it fails in the field, a simple configuration over-the-air 
 * (OTA) can flip this boolean and restore the legacy pathway.
 */
static bool use_modern_uart_hal = false; 

void Modern_Uart_Transmit(const uint8_t *data, uint16_t length) {
    /*
     * STRANGLER PATTERN IN ACTION:
     * The application code is blind to the underlying infrastructure. 
     * We dynamically route the request based on the migration phase.
     */
    if (use_modern_uart_hal) {
        /* Route directly to the new, clean HAL. No overhead. */
        hal_uart_send_dma(data, length);
    } else {
        /* 
         * ANTI-CORRUPTION LAYER (ACL):
         * Map modern types (explicit uint8_t buffer and length) into 
         * legacy types (null-terminated char*). This isolates the unsafe
         * conversion entirely inside the adapter, keeping new code pristine.
         */
        char legacy_buffer[128];
        
        /* Enforce boundary safety before crossing into the unsafe legacy domain */
        if (length < (sizeof(legacy_buffer) - 1)) {
            memcpy(legacy_buffer, data, length);
            legacy_buffer[length] = '\0'; /* Force null-termination for legacy function */
            
            /* Warning: Casting away constness because legacy code is sloppy */
            legacy_transmit_serial(legacy_buffer);
        }
    }
}

void Modern_System_SetState(uint32_t new_state) {
    /*
     * STRANGLER PATTERN IN ACTION:
     * Prevent new code from participating in the toxic global variable anti-pattern.
     * We wrap the mutation here. When the legacy system is finally deleted,
     * this implementation will be replaced with `rtos_event_group_set_bits()`, 
     * and ZERO lines of application logic will need to change.
     */
    system_state_flag = new_state;
}

void LegacyAdapter_Tick(void) {
    /*
     * STRANGLER PATTERN SEAM:
     * This function is injected into the main() super-loop instead of the old
     * legacy_do_everything(). It allows us to insert modern background processing
     * into the execution stream while still servicing the legacy beast.
     */
     
    /* 1. Pre-process modern adapters (e.g., flush RTOS bridging queues) */
    /* ... bridge logic ... */

    /* 2. Execute the shrinking legacy monolith */
    legacy_do_everything();
    
    /* 3. Post-process legacy state changes and reflect them in the modern system */
    /* ... observation logic ... */
}
