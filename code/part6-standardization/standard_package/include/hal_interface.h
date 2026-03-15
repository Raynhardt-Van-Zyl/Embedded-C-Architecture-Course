/**
 * @file hal_interface.h
 * @brief Common definitions for all Hardware Abstraction Layer modules.
 * 
 * Part of the Part VI: Standardization Framework.
 */

#ifndef HAL_INTERFACE_H
#define HAL_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Standardized Return Codes
 * Every function in the architecture must return one of these or a superset.
 */
typedef enum {
    HAL_STATUS_OK          = 0,
    HAL_STATUS_ERROR       = 1,
    HAL_STATUS_BUSY        = 2,
    HAL_STATUS_TIMEOUT     = 3,
    HAL_STATUS_INVALID_ARG = 4,
    HAL_STATUS_NOT_SUPPORTED = 5
} HalStatus_t;

#endif /* HAL_INTERFACE_H */
