/**
 * @file app_layer.c
 * @brief Application Layer Implementation
 */
#include "app_layer.h"
#include "service_layer.h"
#include <stdio.h>

void App_Run(void) {
    printf("[APP] Starting application logic cycle...\n");
    
    if (Service_ProcessSensorData() == SERVICE_OK) {
        printf("[APP] Cycle completed successfully.\n");
    } else {
        printf("[APP] ERROR: Service layer reported failure.\n");
    }
}
