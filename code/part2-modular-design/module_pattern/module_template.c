/**
 * @file module_template.c
 * @brief Standard module template implementation
 */

#include "module_template.h"

/*============================================================================*/
/* PRIVATE TYPES                                                               */
/*============================================================================*/

struct ModuleTemplate_Context {
    uint32_t internal_state;
    uint8_t counter;
    bool is_initialized;
};

/*============================================================================*/
/* PRIVATE VARIABLES                                                           */
/*============================================================================*/

static struct ModuleTemplate_Context s_instances[4];

/*============================================================================*/
/* PUBLIC FUNCTION IMPLEMENTATIONS                                               */
/*============================================================================*/

ModuleTemplate_t* ModuleTemplate_Create(const ModuleTemplate_Config_t* config)
{
    if (config == NULL) return NULL;
    
    for (int i = 0; i < 4; i++) {
        if (!s_instances[i].is_initialized) {
            s_instances[i].internal_state = config->setting_a;
            s_instances[i].counter = 0;
            s_instances[i].is_initialized = true;
            return &s_instances[i];
        }
    }
    return NULL;
}

void ModuleTemplate_Destroy(ModuleTemplate_t* self)
{
    if (self != NULL) {
        self->is_initialized = false;
    }
}

ModuleTemplate_Status_t ModuleTemplate_Process(ModuleTemplate_t* self, 
                                               int32_t input, 
                                               int32_t* output)
{
    if (self == NULL || output == NULL) {
        return MODULE_TEMPLATE_ERR_INVALID_PARAM;
    }
    if (!self->is_initialized) {
        return MODULE_TEMPLATE_ERR_NOT_INITIALIZED;
    }
    
    *output = input + (int32_t)self->internal_state;
    self->counter++;
    
    return MODULE_TEMPLATE_OK;
}
