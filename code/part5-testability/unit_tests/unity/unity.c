/**
 * @file unity.c
 * @brief Minimal Unity implementation
 */
#include "unity.h"
#include <stdio.h>

static int tests_run = 0;
static int tests_failed = 0;

void UnityBegin(const char* filename) {
    printf("--- Testing: %s ---\n", filename);
    tests_run = 0;
    tests_failed = 0;
}

int UnityEnd(void) {
    printf("\nTests Run: %d, Failures: %d\n", tests_run, tests_failed);
    return tests_failed;
}

void UnityAssert(bool condition, const char* msg, int line) {
    tests_run++;
    if (!condition) {
        tests_failed++;
        printf("FAIL: %s (line %d)\n", msg, line);
    } else {
        printf(".");
    }
}

__attribute__((weak)) void setUp(void) {}
__attribute__((weak)) void tearDown(void) {}
