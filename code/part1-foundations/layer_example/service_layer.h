/**
 * @file service_layer.h
 * @brief Service Layer - Part of the Layered Architecture Example
 */
#ifndef SERVICE_LAYER_H
#define SERVICE_LAYER_H

#include <stdint.h>

typedef enum {
    SERVICE_OK,
    SERVICE_ERROR
} ServiceStatus_t;

ServiceStatus_t Service_ProcessSensorData(void);

#endif
