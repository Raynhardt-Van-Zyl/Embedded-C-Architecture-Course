/**
 * @file safe_queue.c
 * @brief Thread-safe queue wrapper implementation using FreeRTOS
 * 
 * @author Embedded C Architecture Course
 */

#include "safe_queue.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <stddef.h>

/*============================================================================*/
/* PRIVATE TYPES                                                               */
/*============================================================================*/

struct SafeQueue_Context {
    QueueHandle_t handle;
    /* Additional metadata could go here (e.g., name, stats) */
};

/*============================================================================*/
/* PUBLIC FUNCTIONS                                                            */
/*============================================================================*/

SafeQueue_t* SafeQueue_Create(uint32_t item_size, uint32_t length) {
    /* 
     * Note: Creating the wrapper struct requires dynamic allocation.
     * In a strict static-only system, caller would provide the memory.
     */
    SafeQueue_t* queue = (SafeQueue_t*)pvPortMalloc(sizeof(SafeQueue_t));
    if (queue == NULL) {
        return NULL;
    }

    queue->handle = xQueueCreate(length, item_size);
    if (queue->handle == NULL) {
        vPortFree(queue);
        return NULL;
    }

    return queue;
}

void SafeQueue_Destroy(SafeQueue_t* queue) {
    if (queue != NULL) {
        if (queue->handle != NULL) {
            vQueueDelete(queue->handle);
        }
        vPortFree(queue);
    }
}

bool SafeQueue_Send(SafeQueue_t* queue, const void* item, uint32_t timeout_ms) {
    if (queue == NULL || queue->handle == NULL || item == NULL) {
        return false;
    }

    TickType_t ticks = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return (xQueueSend(queue->handle, item, ticks) == pdTRUE);
}

bool SafeQueue_Receive(SafeQueue_t* queue, void* item, uint32_t timeout_ms) {
    if (queue == NULL || queue->handle == NULL || item == NULL) {
        return false;
    }

    TickType_t ticks = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return (xQueueReceive(queue->handle, item, ticks) == pdTRUE);
}

uint32_t SafeQueue_GetWaiting(SafeQueue_t* queue) {
    if (queue == NULL || queue->handle == NULL) {
        return 0;
    }
    return (uint32_t)uxQueueMessagesWaiting(queue->handle);
}

bool SafeQueue_IsEmpty(SafeQueue_t* queue) {
    return (SafeQueue_GetWaiting(queue) == 0);
}

bool SafeQueue_IsFull(SafeQueue_t* queue) {
    if (queue == NULL || queue->handle == NULL) {
        return false;
    }
    return (uxQueueSpacesAvailable(queue->handle) == 0);
}
