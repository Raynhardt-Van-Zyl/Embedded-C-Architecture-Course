/**
 * @file unity.h
 * @brief Minimal version of Unity Test Framework for Embedded Course
 */
#ifndef UNITY_H
#define UNITY_H

#include <stdint.h>
#include <stdbool.h>

void UnityBegin(const char* filename);
int UnityEnd(void);

void UnityAssert(bool condition, const char* msg, int line);

#define TEST_ASSERT(cond) UnityAssert(cond, #cond, __LINE__)
#define TEST_ASSERT_TRUE(cond) TEST_ASSERT(cond)
#define TEST_ASSERT_FALSE(cond) TEST_ASSERT(!(cond))
#define TEST_ASSERT_EQUAL(expected, actual) TEST_ASSERT((expected) == (actual))

#define RUN_TEST(func) \
    void func(void); \
    setUp(); \
    func(); \
    tearDown();

void setUp(void);
void tearDown(void);

#endif
