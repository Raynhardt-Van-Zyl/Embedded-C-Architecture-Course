/**
 * @file module_template.h
 * @brief Standard module template for embedded C
 * 
 * This template demonstrates the company-standard module structure.
 * Copy this file and adapt for your specific module.
 */

#ifndef MODULE_TEMPLATE_H_
#define MODULE_TEMPLATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/* VERSION INFORMATION                                                          */
/*============================================================================*/

#define MODULE_TEMPLATE_VERSION_MAJOR  1
#define MODULE_TEMPLATE_VERSION_MINOR  0

/*============================================================================*/
/* PUBLIC TYPES                                                                */
/*============================================================================*/

/**
 * @brief Opaque handle to module instance
 */
typedef struct ModuleTemplate_Context ModuleTemplate_t;

/**
 * @brief Module status codes
 */
typedef enum {
    MODULE_TEMPLATE_OK = 0,
    MODULE_TEMPLATE_ERR_INVALID_PARAM = -1,
    MODULE_TEMPLATE_ERR_NOT_INITIALIZED = -2,
    MODULE_TEMPLATE_ERR_BUSY = -3,
    MODULE_TEMPLATE_ERR_TIMEOUT = -4
} ModuleTemplate_Status_t;

/**
 * @brief Module configuration structure
 */
typedef struct {
    uint32_t setting_a;
    uint8_t setting_b;
    bool enabled;
} ModuleTemplate_Config_t;

/*============================================================================*/
/* PUBLIC FUNCTION PROTOTYPES                                                   */
/*============================================================================*/

/**
 * @brief Create a module instance
 * @param config Configuration parameters
 * @return Handle or NULL on failure
 */
ModuleTemplate_t* ModuleTemplate_Create(const ModuleTemplate_Config_t* config);

/**
 * @brief Destroy a module instance
 * @param self Module handle
 */
void ModuleTemplate_Destroy(ModuleTemplate_t* self);

/**
 * @brief Process data
 * @param self Module handle
 * @param input Input value
 * @param output Output buffer
 * @return Status code
 */
ModuleTemplate_Status_t ModuleTemplate_Process(ModuleTemplate_t* self, 
                                               int32_t input, 
                                               int32_t* output);

#ifdef __cplusplus
}
#endif

#endif /* MODULE_TEMPLATE_H_ */
