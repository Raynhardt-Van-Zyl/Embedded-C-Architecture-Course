/**
 * @file rtos_task_template.c
 * @brief RTOS task template implementation
 * 
 * This template demonstrates the canonical task structure:
 * 1. Initialization phase
 * 2. Event-driven loop with finite timeout
 * 3. Housekeeping/heartbeat
 */

#include "rtos_task_template.h"

/* FreeRTOS includes - these would come from the RTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*============================================================================*/
/* PRIVATE TYPES                                                               */
/*============================================================================*/

struct TaskTemplate_Context {
    QueueHandle_t event_queue;
    uint32_t events_processed;
    bool running;
};

/*============================================================================*/
/* PRIVATE VARIABLES                                                           */
/*============================================================================*/

static TaskTemplate_t s_ctx;
static TaskHandle_t s_task_handle;
static StackType_t s_stack[TASK_TEMPLATE_STACK_SIZE];
static StaticTask_t s_task_tcb;
static uint8_t s_queue_storage[TASK_TEMPLATE_QUEUE_LENGTH * sizeof(TaskTemplate_Event_t)];
static StaticQueue_t s_queue_tcb;

/*============================================================================*/
/* PRIVATE FUNCTIONS                                                           */
/*============================================================================*/

static void TaskTemplate_Main(void* pvParameters)
{
    (void)pvParameters;
    
    /* Phase 1: INITIALIZATION */
    s_ctx.events_processed = 0;
    s_ctx.running = true;
    
    /* Phase 2: EVENT-DRIVEN LOOP */
    while (s_ctx.running) {
        TaskTemplate_Event_t event;
        
        /* Use FINITE timeout for watchdog check-in */
        BaseType_t result = xQueueReceive(
            s_ctx.event_queue,
            &event,
            pdMS_TO_TICKS(TASK_TEMPLATE_TIMEOUT_MS)
        );
        
        if (result == pdPASS) {
            /* Phase 3: EVENT PROCESSING */
            s_ctx.events_processed++;
            
            switch (event.type) {
                case TASK_TEMPLATE_EVENT_DATA:
                    /* Process data event */
                    break;
                    
                case TASK_TEMPLATE_EVENT_TIMER:
                    /* Timer tick - pet watchdog here */
                    break;
                    
                case TASK_TEMPLATE_EVENT_SHUTDOWN:
                    s_ctx.running = false;
                    break;
            }
        } else {
            /* Phase 4: TIMEOUT - watchdog check-in opportunity */
            /* This is normal - use for heartbeat */
        }
        
        /* Phase 5: HOUSEKEEPING */
        /* watchdog_check_in(TASK_ID_TEMPLATE); */
    }
    
    vTaskDelete(NULL);
}

/*============================================================================*/
/* PUBLIC FUNCTIONS                                                            */
/*============================================================================*/

void TaskTemplate_Init(void)
{
    s_ctx.event_queue = xQueueCreateStatic(
        TASK_TEMPLATE_QUEUE_LENGTH,
        sizeof(TaskTemplate_Event_t),
        s_queue_storage,
        &s_queue_tcb
    );
    
    s_task_handle = xTaskCreateStatic(
        TaskTemplate_Main,
        "TaskTemplate",
        TASK_TEMPLATE_STACK_SIZE,
        NULL,
        TASK_TEMPLATE_PRIORITY,
        s_stack,
        &s_task_tcb
    );
}

void TaskTemplate_SendEvent(const TaskTemplate_Event_t* event)
{
    if (s_ctx.event_queue != NULL && event != NULL) {
        xQueueSend(s_ctx.event_queue, event, pdMS_TO_TICKS(100));
    }
}
