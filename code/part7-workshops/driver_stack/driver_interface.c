/**
 * @file driver_interface.c
 * @brief Implementation of Generic Driver Interface
 * 
 * @author Embedded C Architecture Course
 */

#include "driver_interface.h"
#include <string.h>

/*============================================================================*/
/*                              PRIVATE DATA                                  */
/*============================================================================*/

static Driver_t* s_driver_registry[DRIVER_MAX_REGISTRY] = {NULL};
static uint32_t s_driver_count = 0;
static bool s_initialized = false;

/*============================================================================*/
/*                              PUBLIC API                                    */
/*============================================================================*/

DriverStatus_e Driver_InitSubsystem(void) {
    if (s_initialized) return DRIVER_OK;
    
    memset(s_driver_registry, 0, sizeof(s_driver_registry));
    s_driver_count = 0;
    s_initialized = true;
    
    return DRIVER_OK;
}

void Driver_DeinitSubsystem(void) {
    s_initialized = false;
    s_driver_count = 0;
}

DriverStatus_e Driver_Register(Driver_t *driver) {
    if (!s_initialized || driver == NULL) return DRIVER_ERR_INVALID_PARAM;
    if (s_driver_count >= DRIVER_MAX_REGISTRY) return DRIVER_ERR_NO_MEMORY;
    
    // Check if already registered
    for (uint32_t i = 0; i < s_driver_count; i++) {
        if (s_driver_registry[i] == driver) return DRIVER_ERR_ALREADY_INIT;
    }
    
    s_driver_registry[s_driver_count++] = driver;
    return DRIVER_OK;
}

DriverStatus_e Driver_Unregister(Driver_t *driver) {
    if (!s_initialized || driver == NULL) return DRIVER_ERR_INVALID_PARAM;
    
    for (uint32_t i = 0; i < s_driver_count; i++) {
        if (s_driver_registry[i] == driver) {
            // Shift remaining drivers
            for (uint32_t j = i; j < s_driver_count - 1; j++) {
                s_driver_registry[j] = s_driver_registry[j + 1];
            }
            s_driver_registry[--s_driver_count] = NULL;
            return DRIVER_OK;
        }
    }
    return DRIVER_ERR_NOT_FOUND;
}

Driver_t* Driver_FindByName(const char *name) {
    if (!s_initialized || name == NULL) return NULL;
    
    for (uint32_t i = 0; i < s_driver_count; i++) {
        if (strncmp(s_driver_registry[i]->info.name, name, DRIVER_MAX_NAME_LEN) == 0) {
            return s_driver_registry[i];
        }
    }
    return NULL;
}

Driver_t* Driver_FindByType(DriverType_e type, uint32_t instance) {
    if (!s_initialized) return NULL;
    
    for (uint32_t i = 0; i < s_driver_count; i++) {
        if (s_driver_registry[i]->info.type == type && 
            s_driver_registry[i]->info.instance == instance) {
            return s_driver_registry[i];
        }
    }
    return NULL;
}

DriverStatus_e Driver_Init(Driver_t *driver, const DriverConfig_t *config) {
    if (driver == NULL || driver->ops == NULL || driver->ops->init == NULL) return DRIVER_ERR_INVALID_PARAM;
    return driver->ops->init(driver, config);
}

DriverStatus_e Driver_Deinit(Driver_t *driver) {
    if (driver == NULL || driver->ops == NULL || driver->ops->deinit == NULL) return DRIVER_ERR_INVALID_PARAM;
    return driver->ops->deinit(driver);
}

DriverStatus_e Driver_Open(Driver_t *driver) {
    if (driver == NULL || driver->ops == NULL || driver->ops->open == NULL) return DRIVER_ERR_INVALID_PARAM;
    return driver->ops->open(driver);
}

DriverStatus_e Driver_Close(Driver_t *driver) {
    if (driver == NULL || driver->ops == NULL || driver->ops->close == NULL) return DRIVER_ERR_INVALID_PARAM;
    return driver->ops->close(driver);
}

int32_t Driver_Read(Driver_t *driver, void *buffer, uint32_t size) {
    if (driver == NULL || driver->ops == NULL || driver->ops->read == NULL) return DRIVER_ERR_INVALID_PARAM;
    return driver->ops->read(driver, buffer, size);
}

int32_t Driver_Write(Driver_t *driver, const void *data, uint32_t size) {
    if (driver == NULL || driver->ops == NULL || driver->ops->write == NULL) return DRIVER_ERR_INVALID_PARAM;
    return driver->ops->write(driver, data, size);
}

int32_t Driver_ReadAsync(Driver_t *driver, void *buffer, uint32_t size) {
    if (driver == NULL || driver->ops == NULL || driver->ops->readAsync == NULL) return DRIVER_ERR_NOT_SUPPORTED;
    return driver->ops->readAsync(driver, buffer, size);
}

int32_t Driver_WriteAsync(Driver_t *driver, const void *data, uint32_t size) {
    if (driver == NULL || driver->ops == NULL || driver->ops->writeAsync == NULL) return DRIVER_ERR_NOT_SUPPORTED;
    return driver->ops->writeAsync(driver, data, size);
}

DriverStatus_e Driver_Ioctl(Driver_t *driver, uint32_t cmd, void *arg) {
    if (driver == NULL || driver->ops == NULL || driver->ops->ioctl == NULL) return DRIVER_ERR_NOT_SUPPORTED;
    return driver->ops->ioctl(driver, cmd, arg);
}

DriverStatus_e Driver_Suspend(Driver_t *driver) {
    if (driver == NULL || driver->ops == NULL || driver->ops->suspend == NULL) return DRIVER_ERR_NOT_SUPPORTED;
    return driver->ops->suspend(driver);
}

DriverStatus_e Driver_Resume(Driver_t *driver) {
    if (driver == NULL || driver->ops == NULL || driver->ops->resume == NULL) return DRIVER_ERR_NOT_SUPPORTED;
    return driver->ops->resume(driver);
}

DriverStatus_e Driver_SelfTest(Driver_t *driver) {
    if (driver == NULL || driver->ops == NULL || driver->ops->selfTest == NULL) return DRIVER_ERR_NOT_SUPPORTED;
    return driver->ops->selfTest(driver);
}

DriverStatus_e Driver_GetInfo(Driver_t *driver, DriverInfo_t *info) {
    if (driver == NULL || info == NULL) return DRIVER_ERR_INVALID_PARAM;
    if (driver->ops && driver->ops->getInfo) return driver->ops->getInfo(driver, info);
    *info = driver->info;
    return DRIVER_OK;
}

DriverStatus_e Driver_GetStats(Driver_t *driver, DriverStats_t *stats) {
    if (driver == NULL || stats == NULL) return DRIVER_ERR_INVALID_PARAM;
    if (driver->ops && driver->ops->getStats) return driver->ops->getStats(driver, stats);
    *stats = driver->stats;
    return DRIVER_OK;
}

DriverStatus_e Driver_ClearStats(Driver_t *driver) {
    if (driver == NULL) return DRIVER_ERR_INVALID_PARAM;
    if (driver->ops && driver->ops->clearStats) return driver->ops->clearStats(driver);
    memset(&driver->stats, 0, sizeof(DriverStats_t));
    return DRIVER_OK;
}

DriverStatus_e Driver_RegisterCallback(Driver_t *driver, DriverEventCallback_t callback, void *userData) {
    if (driver == NULL) return DRIVER_ERR_INVALID_PARAM;
    if (driver->ops && driver->ops->registerCallback) return driver->ops->registerCallback(driver, callback, userData);
    driver->callback = callback;
    driver->userData = userData;
    return DRIVER_OK;
}
