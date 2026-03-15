/**
 * @file isr_safe.c
 * @brief ISR-Safe Programming Patterns Implementation
 * 
 * @author Embedded C Architecture Course
 */

#include "isr_safe.h"
#include <string.h>

/*============================================================================*/
/*                             PRIVATE DATA                                    */
/*============================================================================*/

volatile uint32_t g_criticalNestingLevel = 0;
volatile uint32_t g_savedPrimask = 0;

/*============================================================================*/
/*                             PUBLIC API                                      */
/*============================================================================*/

void ISR_Init(void) {
    g_criticalNestingLevel = 0;
    g_savedPrimask = 0;
}

bool ISR_IsInInterrupt(void) {
#if defined(__arm__) || defined(__thumb__)
    uint32_t ipsr;
    __asm volatile ("MRS %0, ipsr" : "=r" (ipsr));
    return (ipsr != 0);
#else
    return false; // Cannot determine on host
#endif
}

/*----------------------------------------------------------------------------*/
/*                         RING BUFFER FUNCTIONS                               */
/*----------------------------------------------------------------------------*/

RingBufferResult_t RingBuffer_Init(RingBuffer_t *rb) {
    if (!rb) return RING_BUFFER_INVALID;
    
    memset((void*)rb->buffer, 0, sizeof(rb->buffer));
    rb->head = 0;
    rb->tail = 0;
    rb->size = ISR_RING_BUFFER_SIZE;
    rb->mask = ISR_RING_BUFFER_SIZE - 1;
    
    return RING_BUFFER_OK;
}

RingBufferResult_t RingBuffer_Write(RingBuffer_t *rb, uint8_t data) {
    uint32_t next_head = (rb->head + 1) & rb->mask;
    
    if (next_head == rb->tail) {
        return RING_BUFFER_FULL;
    }
    
    rb->buffer[rb->head] = data;
    ISR_COMPILER_BARRIER(); // Ensure data is written before head update
    rb->head = next_head;
    
    return RING_BUFFER_OK;
}

RingBufferResult_t RingBuffer_Read(RingBuffer_t *rb, uint8_t *data) {
    if (rb->head == rb->tail) {
        return RING_BUFFER_EMPTY;
    }
    
    *data = rb->buffer[rb->tail];
    ISR_COMPILER_BARRIER(); // Ensure data is read before tail update
    rb->tail = (rb->tail + 1) & rb->mask;
    
    return RING_BUFFER_OK;
}

uint32_t RingBuffer_GetCount(const RingBuffer_t *rb) {
    return (rb->head - rb->tail) & rb->mask;
}

bool RingBuffer_IsEmpty(const RingBuffer_t *rb) {
    return (rb->head == rb->tail);
}

bool RingBuffer_IsFull(const RingBuffer_t *rb) {
    return ((rb->head + 1) & rb->mask) == rb->tail;
}

void RingBuffer_Flush(RingBuffer_t *rb) {
    rb->tail = rb->head;
}

/*----------------------------------------------------------------------------*/
/*                         FLAG MANAGEMENT FUNCTIONS                           */
/*----------------------------------------------------------------------------*/

void Flag_Set(AtomicFlag_t *flag) {
    *flag = 1;
    ISR_MEMORY_BARRIER();
}

void Flag_Clear(AtomicFlag_t *flag) {
    *flag = 0;
    ISR_MEMORY_BARRIER();
}

bool Flag_TestAndClear(AtomicFlag_t *flag) {
    bool was_set = false;
    
    ISR_CRITICAL_ENTER();
    if (*flag) {
        was_set = true;
        *flag = 0;
    }
    ISR_CRITICAL_EXIT();
    
    return was_set;
}

FlagValue_t Flag_Get(const AtomicFlag_t *flag) {
    return (FlagValue_t)*flag;
}
