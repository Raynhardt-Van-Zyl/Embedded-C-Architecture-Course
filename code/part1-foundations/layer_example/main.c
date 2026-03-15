/**
 * @file main.c
 * @brief Entry point for Part I foundational example
 */
#include "app_layer.h"
#include "driver_layer.h"
#include <stdio.h>

int main(void) {
    printf("=== Part I: Layered Architecture Example ===\n");
    
    // Initialize low-level first
    Driver_Init();
    
    // Run application logic
    while(1) {
        App_Run();
        
        // In a real embedded system, we'd wait for a tick or event
        // Here we just loop a few times for the example
        static int cycles = 0;
        if (++cycles >= 3) break;
    }
    
    printf("Example finished.\n");
    return 0;
}
