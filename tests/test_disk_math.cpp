// test_disk_math.cpp — disk_math 模块单元测试
#include "test_assert.h"

#include <limits>

#include "disk_math.h"

void RunDiskMathTests()
{
    // %Busy = 100 − % Idle Time（LEARN.md 的核心公式）
    CHECK_NEAR(IdleToBusyPercent(100.0), 0.0, 1e-9);
    CHECK_NEAR(IdleToBusyPercent(0.0), 100.0, 1e-9);
    CHECK_NEAR(IdleToBusyPercent(85.0), 15.0, 1e-9);

    // 计数器抖动导致的越界值必须被夹紧，浮窗不能显示负数或 >100%
    CHECK_NEAR(IdleToBusyPercent(105.0), 0.0, 1e-9);
    CHECK_NEAR(IdleToBusyPercent(-5.0), 100.0, 1e-9);
    CHECK_NEAR(IdleToBusyPercent(std::numeric_limits<double>::quiet_NaN()), 100.0, 1e-9);

    CHECK_NEAR(BusyFillFraction(0.0), 0.0, 1e-9);
    CHECK_NEAR(BusyFillFraction(50.0), 0.5, 1e-9);
    CHECK_NEAR(BusyFillFraction(100.0), 1.0, 1e-9);
    CHECK_NEAR(BusyFillFraction(140.0), 1.0, 1e-9);

    // 秒 → 毫秒；负值 / NaN 视为无效（返回负值）
    CHECK_NEAR(TransferLatencyMs(0.0021), 2.1, 1e-9);
    CHECK(TransferLatencyMs(-0.001) < 0.0);
    CHECK(TransferLatencyMs(std::numeric_limits<double>::quiet_NaN()) < 0.0);

    // 饱和度判据：HDD 25 ms / SSD 5 ms 为典型阈值
    CHECK(IsLatencySaturated(5.0, 5.0));
    CHECK(IsLatencySaturated(30.0, 25.0));
    CHECK(!IsLatencySaturated(4.9, 5.0));
    CHECK(!IsLatencySaturated(-1.0, 5.0));   // 无数据不算饱和
}
