/**
 * @file rtos_task_template.h
 * @brief RTOS task template following company standard
 */

#ifndef RTOS_TASK_TEMPLATE_H_
#define RTOS_TASK_TEMPLATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/* CONFIGURATION                                                               */
/*============================================================================*/

#define TASK_TEMPLATE_STACK_SIZE    256
#define TASK_TEMPLATE_PRIORITY      2
#define TASK_TEMPLATE_QUEUE_LENGTH  10
#define TASK_TEMPLATE_TIMEOUT_MS    500

/*============================================================================*/
/* PUBLIC TYPES                                                                */
/*============================================================================*/

typedef struct TaskTemplate_Context TaskTemplate_t;

typedef enum {
    TASK_TEMPLATE_EVENT_DATA = 1,
    TASK_TEMPLATE_EVENT_TIMER,
    TASK_TEMPLATE_EVENT_SHUTDOWN
} TaskTemplate_EventType_t;

typedef struct {
    TaskTemplate_EventType_t type;
    uint32_t data;
} TaskTemplate_Event_t;

/*============================================================================*/
/* PUBLIC FUNCTION PROTOTYPES                                                   */
/*============================================================================*/

void TaskTemplate_Init(void);
void TaskTemplate_SendEvent(const TaskTemplate_Event_t* event);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_TASK_TEMPLATE_H_ */
