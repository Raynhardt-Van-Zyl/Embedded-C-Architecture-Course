/**
 * @file Halrtc.h
 * @brief Hardware Abstraction Layer for Real-Time Clock (RTC).
 *
 * @details
 * ARCHITECTURE DECISION: Epoch Standard
 * To ensure absolute cross-platform compatibility, this HAL standardizes around
 * the Unix Timestamp (seconds since Jan 1, 1970). Internal conversion to hardware
 * BCD registers (Year/Month/Day) happens transparently.
 */

#ifndef HalRTC_H
#define HalRTC_H

#include <stdint.h>
#include <stdbool.h>

/** @brief Opaque handle for RTC instances. */
typedef struct HalRtc_s* HalRtcHandle;

/** @brief RTC Clock Source. */
typedef enum {
    HalRTC_CLK_LSI, /**< Low-Speed Internal (Inaccurate, no external crystal needed) */
    HalRTC_CLK_LSE  /**< Low-Speed External (Accurate 32.768kHz crystal) */
} HalRtcClockSource_t;

/** @brief RTC Alarm Callback function. */
typedef void (*HalRtcAlarmCallback_t)(void* context);

/** @brief RTC configuration structure. */
typedef struct {
    HalRtcClockSource_t clk_src;
} HalRtcConfig_t;

/**
 * @brief Initialize the RTC.
 * @param config Pointer to configuration struct.
 * @return HalRtcHandle Valid handle or NULL.
 */
HalRtcHandle HalRtc_Init(const HalRtcConfig_t* config);

/**
 * @brief Set the current time.
 * @param handle The RTC handle.
 * @param unix_timestamp Seconds since Unix epoch.
 * @return true on success.
 */
bool HalRtc_SetTime(HalRtcHandle handle, uint32_t unix_timestamp);

/**
 * @brief Get the current time.
 * @param handle The RTC handle.
 * @param out_subseconds Pointer to store sub-second fraction (optional, can be NULL).
 * @return Unix timestamp.
 */
uint32_t HalRtc_GetTime(HalRtcHandle handle, uint32_t* out_subseconds);

/**
 * @brief Set a future alarm.
 * @param handle The RTC handle.
 * @param unix_timestamp Target time for the alarm.
 * @return true on success.
 */
bool HalRtc_SetAlarm(HalRtcHandle handle, uint32_t unix_timestamp);

/**
 * @brief Register a callback for when the alarm triggers.
 * @param handle The RTC handle.
 * @param cb The callback function.
 * @param context User context.
 */
void HalRtc_RegisterAlarmCallback(HalRtcHandle handle, HalRtcAlarmCallback_t cb, void* context);

/**
 * @brief Read from a battery-backed backup register.
 * @param handle The RTC handle.
 * @param reg_index Register index.
 * @return Register value.
 */
uint32_t HalRtc_ReadBackupRegister(HalRtcHandle handle, uint8_t reg_index);

/**
 * @brief Write to a battery-backed backup register.
 * @param handle The RTC handle.
 * @param reg_index Register index.
 * @param value Value to write.
 */
void HalRtc_WriteBackupRegister(HalRtcHandle handle, uint8_t reg_index, uint32_t value);

/**
 * @brief Deinitialize the RTC.
 * @param handle The RTC handle.
 */
void HalRtc_DeInit(HalRtcHandle handle);

#endif /* HalRTC_H */
