#ifndef TEST_h
#define TEST_H

#include <stdio.h>
#include <stdbool.h>

static int tests_run = 0;
static int tests_failed = 0;
static int tests_passed = 0;

#define TEST(name) static void name(void)
#define ASSERT_EQ(actual, expected) do { \
    tests_run++; \
    if ((actual) != (expected)) { \
        tests_failed++; \
        printf("FAIL %s:%d: expected %ld, got %ld\n", \
               __FILE__, __LINE__, (long)(expected), (long)(actual)); \
    }else { \
        tests_passed++;
    } \
} while (0)

#define ASSERT_TRUE(value) do { \
    tests_run++; \
    if (value != true && value <= 0) { \
        tests_failed++; \ 
        printf("FAIL %s:%d: expected %ld, got %ld\n", \
               __FILE__, __LINE__, (long)(expected), (long)(actual)); \
    } else { \
        tests_passed++;
    } \
}while (0)

#define ASSERT_FALSE(value) do { \
    tests_run++; \
    if (value == true && value > 0) { \
        tests_failed++; \ 
        printf("FAIL %s:%d: expected %ld, got %ld\n", \
               __FILE__, __LINE__, (long)(expected), (long)(actual)); \
    } else { \
        tests_passed++;
    } \
}while (0)



#endif
