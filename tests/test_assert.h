// test_assert.h — 极简断言工具（不引入任何测试框架）
#pragma once

#include <cmath>
#include <cstdio>
#include <cstring>

inline int& TestFailureCount()
{
    static int failures = 0;
    return failures;
}

inline void ReportFailure(const char* file, int line, const char* what)
{
    std::printf("FAIL %s:%d: %s\n", file, line, what);
    TestFailureCount()++;
}

inline bool NearlyEqual(double a, double b, double epsilon = 1e-9)
{
    return std::fabs(a - b) <= epsilon;
}

inline int TestSummary(const char* suite)
{
    const int failures = TestFailureCount();
    if (failures == 0) {
        std::printf("%s: all tests passed\n", suite);
        return 0;
    }
    std::printf("%s: %d test(s) FAILED\n", suite, failures);
    return 1;
}

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) ReportFailure(__FILE__, __LINE__, #cond);             \
    } while (0)

#define CHECK_EQ_INT(actual, expected)                                     \
    do {                                                                  \
        long long a_ = (long long)(actual), e_ = (long long)(expected);    \
        if (a_ != e_) {                                                    \
            char buf_[160];                                                \
            std::snprintf(buf_, sizeof(buf_), "%s == %s (got %lld, want %lld)", \
                          #actual, #expected, a_, e_);                     \
            ReportFailure(__FILE__, __LINE__, buf_);                       \
        }                                                                 \
    } while (0)

#define CHECK_NEAR(actual, expected, epsilon)                              \
    do {                                                                  \
        double a_ = (double)(actual), e_ = (double)(expected);             \
        if (!NearlyEqual(a_, e_, (epsilon))) {                             \
            char buf_[160];                                                \
            std::snprintf(buf_, sizeof(buf_), "%s ~= %s (got %.6f, want %.6f)", \
                          #actual, #expected, a_, e_);                     \
            ReportFailure(__FILE__, __LINE__, buf_);                       \
        }                                                                 \
    } while (0)

#define CHECK_STR(actual, expected)                                        \
    do {                                                                  \
        const char* a_ = (actual);                                         \
        const char* e_ = (expected);                                       \
        if (!a_ || !e_ || std::strcmp(a_, e_) != 0) {                      \
            char buf_[200];                                                \
            std::snprintf(buf_, sizeof(buf_), "%s == %s (got \"%s\")",      \
                          #actual, #expected, a_ ? a_ : "(null)");         \
            ReportFailure(__FILE__, __LINE__, buf_);                       \
        }                                                                 \
    } while (0)
