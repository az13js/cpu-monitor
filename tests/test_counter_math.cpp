// test_counter_math.cpp — counter_math 模块单元测试
#include "test_assert.h"

#include <limits>

#include "counter_math.h"

void RunCounterMathTests()
{
    CHECK_NEAR(PercentOf(42.5, 0.0), 42.5, 1e-9);
    CHECK_NEAR(PercentOf(-7.0, 0.0), -7.0, 1e-9);              // 负值原样返回，交给调用方判定
    CHECK_NEAR(PercentOf(std::numeric_limits<double>::quiet_NaN(), 5.0), 5.0, 1e-9);
    CHECK_NEAR(PercentOf(std::numeric_limits<double>::infinity(), 5.0), 5.0, 1e-9);

    CHECK_NEAR(ClampPercent(0.0), 0.0, 1e-9);
    CHECK_NEAR(ClampPercent(50.0), 50.0, 1e-9);
    CHECK_NEAR(ClampPercent(100.0), 100.0, 1e-9);
    CHECK_NEAR(ClampPercent(180.0), 100.0, 1e-9);               // Disk Time 类计数器可能超过 100%
    CHECK_NEAR(ClampPercent(-1.0), 0.0, 1e-9);
    CHECK_NEAR(ClampPercent(std::numeric_limits<double>::quiet_NaN()), 0.0, 1e-9);

    CHECK_NEAR(ClampFraction(0.25), 0.25, 1e-9);
    CHECK_NEAR(ClampFraction(1.5), 1.0, 1e-9);
    CHECK_NEAR(ClampFraction(-0.5), 0.0, 1e-9);
    CHECK_NEAR(ClampFraction(std::numeric_limits<double>::quiet_NaN()), 0.0, 1e-9);

    CHECK_NEAR(SecondsToMilliseconds(0.005), 5.0, 1e-9);
    CHECK_NEAR(SecondsToMilliseconds(0.0), 0.0, 1e-9);
}
