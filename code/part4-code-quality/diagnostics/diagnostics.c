/**
 * @file diagnostics.c
 * @brief Implementation of Unified Diagnostics System
 *
 * @author Embedded C Architecture Course
 */

#include "diagnostics.h"
#include <string.h>
#include <stdio.h>

/*============================================================================*/
/*                          PRIVATE DATA                                       */
/*============================================================================*/

static DiagOutputFunc_t s_output_func = NULL;

static DiagTraceBuffer_t s_trace_buffer = {0};

static DiagPerfCounter_t s_perf_counters[DIAG_MAX_COUNTERS] = {0};
static uint32_t s_perf_counter_count = 0;

static DiagHealthReport_t s_health_report = {0};
static bool s_initialized = false;

/*============================================================================*/
/*                          PRIVATE FUNCTIONS                                  */
/*============================================================================*/

static void Diag_Print(const char *format, ...) {
    if (s_output_func == NULL) return;

    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    s_output_func(buffer, (uint32_t)strlen(buffer));
}

/*============================================================================*/
/*                          PUBLIC API IMPLEMENTATION                          */
/*============================================================================*/

void Diag_Init(DiagOutputFunc_t output_func) {
    if (s_initialized) return;

    memset(&s_trace_buffer, 0, sizeof(s_trace_buffer));
    memset(&s_perf_counters, 0, sizeof(s_perf_counters));
    memset(&s_health_report, 0, sizeof(s_health_report));
    
    s_output_func = output_func;
    s_perf_counter_count = 0;
    
    // Initial health state
    s_health_report.status = DIAG_HEALTH_OK;
    s_health_report.reset_count = 1; // Assuming 1 reset on boot

    s_initialized = true;
}

void Diag_SetOutput(DiagOutputFunc_t output_func) {
    s_output_func = output_func;
}

void Diag_Trace(DiagSeverity_t severity, DiagModule_t module, DiagEventId_t event_id, uint32_t data_0, uint32_t data_1) {
    if (!s_initialized) return;

    // Atomic claim of next slot could be added here for ISR safety (e.g. LDREX/STREX)
    // For now, assuming single-core non-preemptive logging or brief critical section
    
    uint32_t idx = s_trace_buffer.head;
    DiagTraceEntry_t *entry = &s_trace_buffer.entries[idx];

    entry->timestamp = Diag_GetTimeMs();
    entry->severity = (uint8_t)severity;
    entry->module = (uint8_t)module;
    entry->event_id = (uint16_t)event_id;
    entry->data_0 = data_0;
    entry->data_1 = data_1;

    // Advance head
    s_trace_buffer.head = (idx + 1) % DIAG_TRACE_BUFFER_SIZE;
    s_trace_buffer.count++;
}

void Diag_TraceSimple(DiagSeverity_t severity, DiagModule_t module, DiagEventId_t event_id) {
    Diag_Trace(severity, module, event_id, 0, 0);
}

void Diag_TraceError(DiagModule_t module, DiagEventId_t event_id, uint32_t error_code) {
    Diag_Trace(DIAG_SEVERITY_ERROR, module, event_id, error_code, 0);
}

uint32_t Diag_TraceGetCount(void) {
    return s_trace_buffer.count > DIAG_TRACE_BUFFER_SIZE ? DIAG_TRACE_BUFFER_SIZE : s_trace_buffer.count;
}

bool Diag_TraceGetEntry(uint32_t index, DiagTraceEntry_t *entry) {
    if (index >= DIAG_TRACE_BUFFER_SIZE) return false;
    
    // Calculate actual index based on head position for LIFO or ring buffer access
    // Here we treat 'index' as offset from oldest available
    uint32_t count = Diag_TraceGetCount();
    if (index >= count) return false;

    uint32_t oldest_idx = (s_trace_buffer.count > DIAG_TRACE_BUFFER_SIZE) ? 
                          s_trace_buffer.head : 0;
    
    uint32_t actual_idx = (oldest_idx + index) % DIAG_TRACE_BUFFER_SIZE;
    *entry = s_trace_buffer.entries[actual_idx];
    return true;
}

void Diag_TraceClear(void) {
    s_trace_buffer.head = 0;
    s_trace_buffer.count = 0;
}

int32_t Diag_PerfRegister(const char *name) {
    if (s_perf_counter_count >= DIAG_MAX_COUNTERS) return -1;
    
    uint32_t id = s_perf_counter_count++;
    s_perf_counters[id].name = name;
    s_perf_counters[id].active = true;
    s_perf_counters[id].min_time_us = 0xFFFFFFFF;
    
    return (int32_t)id;
}

uint32_t Diag_PerfStart(int32_t counter_id) {
    if (counter_id < 0 || counter_id >= (int32_t)DIAG_MAX_COUNTERS) return 0;
    return Diag_GetTimeUs();
}

void Diag_PerfEnd(int32_t counter_id, uint32_t start_time) {
    if (counter_id < 0 || counter_id >= (int32_t)DIAG_MAX_COUNTERS) return;
    
    uint32_t end_time = Diag_GetTimeUs();
    uint32_t duration = end_time - start_time;
    
    DiagPerfCounter_t *ctr = &s_perf_counters[counter_id];
    ctr->count++;
    ctr->total_time_us += duration;
    ctr->last_time_us = duration;
    
    if (duration < ctr->min_time_us) ctr->min_time_us = duration;
    if (duration > ctr->max_time_us) ctr->max_time_us = duration;
}

void Diag_DumpAll(void) {
    Diag_DumpHeader("DIAGNOSTIC REPORT");
    Diag_DumpHealth();
    Diag_DumpSeparator();
    Diag_DumpPerformance();
    Diag_DumpSeparator();
    Diag_DumpTrace(0); // Dump all
    Diag_DumpSeparator();
}

void Diag_DumpHeader(const char *title) {
    Diag_Print("\n=== %s ===\n", title);
}

void Diag_DumpSeparator(void) {
    Diag_Print("----------------------------------------\n");
}

void Diag_DumpHealth(void) {
    Diag_Print("Health: %d\n", s_health_report.status);
    Diag_Print("Uptime: %u ms\n", s_health_report.uptime_ms);
    Diag_Print("Resets: %u\n", s_health_report.reset_count);
}

void Diag_DumpPerformance(void) {
    Diag_Print("PERFORMANCE COUNTERS:\n");
    for (uint32_t i = 0; i < s_perf_counter_count; i++) {
        DiagPerfCounter_t *c = &s_perf_counters[i];
        if (c->count > 0) {
            uint32_t avg = c->total_time_us / c->count;
            Diag_Print("%-15s: %u calls, min/max/avg: %u/%u/%u us\n", 
                       c->name, c->count, c->min_time_us, c->max_time_us, avg);
        }
    }
}

void Diag_DumpTrace(uint32_t count) {
    Diag_Print("TRACE LOG:\n");
    uint32_t available = Diag_TraceGetCount();
    if (count == 0 || count > available) count = available;
    
    for (uint32_t i = 0; i < count; i++) {
        DiagTraceEntry_t e;
        if (Diag_TraceGetEntry(i, &e)) {
            Diag_Print("[%u] %d:%d evt:%d data:%x %x\n", 
                       e.timestamp, e.severity, e.module, e.event_id, e.data_0, e.data_1);
        }
    }
}

/* Stubs for system-specific time functions */
uint32_t Diag_GetTimeUs(void) { return 0; } // TODO: Link to Hardware Timer
uint32_t Diag_GetTimeMs(void) { return 0; } // TODO: Link to Systick

// Stub implementations for the rest of the API to satisfy linker
bool Diag_PerfGetCounter(int32_t counter_id, DiagPerfCounter_t *counter) { (void)counter_id; (void)counter; return false; }
bool Diag_PerfReset(int32_t counter_id) { (void)counter_id; return false; }
void Diag_PerfResetAll(void) {}
void Diag_UpdateHealth(void) {}
void Diag_GetHealthReport(DiagHealthReport_t *report) { *report = s_health_report; }
DiagHealthStatus_t Diag_GetHealthStatus(void) { return s_health_report.status; }
void Diag_InitStackMonitor(uint32_t stack_start, uint32_t stack_size) { (void)stack_start; (void)stack_size; }
void Diag_UpdateCpuLoad(uint32_t total_time_ms, uint32_t idle_time_ms) { (void)total_time_ms; (void)idle_time_ms; }
const char *Diag_GetSeverityName(DiagSeverity_t severity) { (void)severity; return "UNK"; }
const char *Diag_GetModuleName(DiagModule_t module) { (void)module; return "UNK"; }
const char *Diag_GetEventName(DiagEventId_t event_id) { (void)event_id; return "UNK"; }
