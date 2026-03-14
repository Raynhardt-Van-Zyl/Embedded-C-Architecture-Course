/**
 * @file safe_queue.h
 * @brief Thread-safe queue wrapper for RTOS
 */

#ifndef SAFE_QUEUE_H_
#define SAFE_QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/* PUBLIC TYPES                                                                */
/*============================================================================*/

typedef struct SafeQueue_Context SafeQueue_t;

/*============================================================================*/
/* PUBLIC FUNCTION PROTOTYPES                                                   */
/*============================================================================*/

SafeQueue_t* SafeQueue_Create(uint32_t item_size, uint32_t length);
void SafeQueue_Destroy(SafeQueue_t* queue);

bool SafeQueue_Send(SafeQueue_t* queue, const void* item, uint32_t timeout_ms);
bool SafeQueue_Receive(SafeQueue_t* queue, void* item, uint32_t timeout_ms);

uint32_t SafeQueue_GetWaiting(SafeQueue_t* queue);
bool SafeQueue_IsEmpty(SafeQueue_t* queue);
bool SafeQueue_IsFull(SafeQueue_t* queue);

#ifdef __cplusplus
}
#endif

#endif /* SAFE_QUEUE_H_ */
