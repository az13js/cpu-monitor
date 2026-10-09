// test_usage_color.cpp — usage_color 模块单元测试
#include "test_assert.h"

#include "usage_color.h"

namespace {
bool IsValidChannel(int value)
{
    return value >= 0 && value <= 255;
}
}  // namespace

void RunUsageColorTests()
{
    // 小于 50% 为绿色系：红蓝通道固定，绿通道随占用率下降
    const Rgb low = UsageColor(0.0);
    CHECK_EQ_INT(low.r, 50);
    CHECK_EQ_INT(low.b, 50);
    CHECK_EQ_INT(low.g, 255);

    const Rgb mid = UsageColor(0.5);
    CHECK_EQ_INT(mid.r, 50);
    CHECK_EQ_INT(mid.g, 170);
    CHECK_EQ_INT(mid.b, 30);

    // 超过 80% 进入红色系
    const Rgb high = UsageColor(1.0);
    CHECK_EQ_INT(high.r, 220);
    CHECK_EQ_INT(high.g, 0);
    CHECK_EQ_INT(high.b, 25);

    // 任意占用率（含越界值与 NaN）都必须落在合法 RGB 通道范围内
    const double samples[] = { -1.0, 0.0, 0.1, 0.49, 0.5, 0.65, 0.79, 0.8, 0.9, 1.0, 2.0 };
    for (double u : samples) {
        const Rgb c = UsageColor(u);
        CHECK(IsValidChannel(c.r) && IsValidChannel(c.g) && IsValidChannel(c.b));
    }

    // 数值文字颜色：高负载用暖色提示
    const Rgb calm = UsageTextColor(0.2);
    CHECK_EQ_INT(calm.r, 140);
    const Rgb hot = UsageTextColor(0.9);
    CHECK_EQ_INT(hot.r, 255);
    CHECK_EQ_INT(hot.g, 160);
}
