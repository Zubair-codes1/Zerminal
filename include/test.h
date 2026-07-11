#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static int tests_run = 0;
static int tests_failed = 0;
static int tests_passed = 0;

#define TEST(name) static void name(void)

#define ASSERT_EQ(actual, expected) do { \
    tests_run++; \
    long _a = (long)(actual); \
    long _e = (long)(expected); \
    if (_a != _e) { \
        tests_failed++; \
        printf("FAIL %s:%d: expected %ld, got %ld\n", \
               __FILE__, __LINE__, _e, _a); \
    } else { \
        tests_passed++; \
    } \
} while (0)

#define ASSERT_STR_EQ(actual, expected) do { \
    tests_run++; \
    if (strcmp((actual), (expected)) != 0) { \
        tests_failed++; \
        printf("FAIL %s:%d: expected \"%s\", got \"%s\"\n", \
               __FILE__, __LINE__, (expected), (actual)); \
    } else { \
        tests_passed++; \
    } \
} while (0)

#define ASSERT_STR_CONTAINS(haystack, needle) do { \
    tests_run++; \
    if (strstr((haystack), (needle)) == NULL) { \
        tests_failed++; \
        printf("FAIL %s:%d: expected \"%s\" to contain \"%s\"\n", \
               __FILE__, __LINE__, (haystack), (needle)); \
    } else { \
        tests_passed++; \
    } \
} while (0)

#define ASSERT_TRUE(value) do { \
    tests_run++; \
    if (!(value)) { \
        tests_failed++; \
        printf("FAIL %s:%d: expected true, got false\n", __FILE__, __LINE__); \
    } else { \
        tests_passed++; \
    } \
} while (0)

#define ASSERT_FALSE(value) do { \
    tests_run++; \
    if ((value)) { \
        tests_failed++; \
        printf("FAIL %s:%d: expected false, got true\n", __FILE__, __LINE__); \
    } else { \
        tests_passed++; \
    } \
} while (0)

#define TEST_SUMMARY() do { \
    printf("\n%d/%d tests passed\n", tests_passed, tests_run); \
} while (0)

#endif
