/**
 * @file test_diagnostics.c
 * @brief Unit tests for the diagnostics module
 */
#include "unity.h"
#include "diagnostics.h"
#include <string.h>

void test_DiagInit_ShouldClearBuffer(void) {
    Diag_Init(NULL);
    TEST_ASSERT_EQUAL(0, Diag_TraceGetCount());
}

void test_DiagTrace_ShouldStoreEvent(void) {
    Diag_Init(NULL);
    Diag_TraceSimple(DIAG_SEVERITY_INFO, DIAG_MODULE_APP, DIAG_EVENT_INIT);
    TEST_ASSERT_EQUAL(1, Diag_TraceGetCount());
}

void test_DiagPerf_ShouldTrackTiming(void) {
    Diag_Init(NULL);
    int32_t id = Diag_PerfRegister("Test");
    TEST_ASSERT(id >= 0);
    
    uint32_t start = Diag_PerfStart(id);
    Diag_PerfEnd(id, start);
    
    DiagPerfCounter_t stats;
    TEST_ASSERT(Diag_PerfGetCounter(id, &stats));
    TEST_ASSERT_EQUAL(1, stats.count);
}

int main(void) {
    UnityBegin("diagnostics.c");
    RUN_TEST(test_DiagInit_ShouldClearBuffer);
    RUN_TEST(test_DiagTrace_ShouldStoreEvent);
    RUN_TEST(test_DiagPerf_ShouldTrackTiming);
    return UnityEnd();
}
