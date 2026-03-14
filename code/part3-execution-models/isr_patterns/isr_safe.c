/**
 * @file isr_safe.c
 * @brief Lock-free SPSC ring buffer for ISR-safe communication
 */

#include "isr_safe.h"

/*============================================================================*/
/* PUBLIC FUNCTION IMPLEMENTATIONS                                               */
/*============================================================================*/

void RingBuffer_SPSC_Init(RingBuffer_SPSC_t* rb, uint8_t* buffer, uint32_t capacity)
{
    rb->buffer = buffer;
    rb->head = 0;
    rb->tail = 0;
    rb->capacity = capacity;
}

bool RingBuffer_Put_Isr(RingBuffer_SPSC_t* rb, uint8_t data)
{
    uint32_t next_head = (rb->head + 1) & (rb->capacity - 1);
    
    if (next_head == rb->tail) {
        return false;  /* Full */
    }
    
    rb->buffer[rb->head] = data;
    rb->head = next_head;
    return true;
}

bool RingBuffer_Get_Main(RingBuffer_SPSC_t* rb, uint8_t* data)
{
    if (rb->head == rb->tail) {
        return false;  /* Empty */
    }
    
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) & (rb->capacity - 1);
    return true;
}

uint32_t RingBuffer_Available_Isr(RingBuffer_SPSC_t* rb)
{
    return (rb->capacity + rb->head - rb->tail) & (rb->capacity - 1);
}

bool RingBuffer_IsEmpty(RingBuffer_SPSC_t* rb)
{
    return (rb->head == rb->tail);
}

bool RingBuffer_IsFull(RingBuffer_SPSC_t* rb)
{
    return ((rb->head + 1) & (rb->capacity - 1)) == rb->tail;
}
