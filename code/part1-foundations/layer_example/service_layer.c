/**
 * @file service_layer.c
 * @brief Service Layer Implementation
 */
#include "service_layer.h"
#include "driver_layer.h"
#include <stdio.h>

ServiceStatus_t Service_ProcessSensorData(void) {
    printf("[SERVICE] Requesting data from driver...\n");
    
    uint16_t raw_data = Driver_ReadRawValue();
    
    printf("[SERVICE] Applying calibration offset and filtering...\n");
    uint16_t processed_data = raw_data - 50; // Simple business logic
    
    printf("[SERVICE] Processed value: %u\n", processed_data);
    
    return SERVICE_OK;
}
