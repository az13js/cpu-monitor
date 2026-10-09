// test_overlay_layout.cpp — overlay_layout 模块单元测试
#include "test_assert.h"

#include "overlay_layout.h"

namespace {
// 断言磁盘分区各行自上而下排列，且都落在客户区内
void CheckDiskRowsOrdered(const OverlayLayout& lo, int clientHeight)
{
    CHECK(lo.diskTitleY < lo.diskValueY);
    CHECK(lo.diskValueY < lo.diskQueueY);
    CHECK(lo.diskQueueY < lo.diskLatencyY);
    CHECK(lo.diskLatencyY < clientHeight);
    CHECK(lo.barBottomY < lo.diskPanelTopY);   // 条形图在磁盘分区上方
}

// 条形图高度必须为正，且不侵入磁盘分区
void CheckBarsUsable(const OverlayLayout& lo)
{
    CHECK(lo.barBottomY > lo.barTopY);
    CHECK(lo.barWidth > 0);
    CHECK(lo.barsTotalWidth > 0);
}
}  // namespace

void RunOverlayLayoutTests()
{
    const int height = ComputeWindowHeight(8);

    // 柱宽随核数收敛，保证窗口不会被 64 核挤爆
    CHECK_EQ_INT(ComputeOverlayLayout(4, height, true).barWidth, 18);
    CHECK_EQ_INT(ComputeOverlayLayout(24, height, true).barWidth, 12);
    CHECK_EQ_INT(ComputeOverlayLayout(40, height, true).barWidth, 8);
    CHECK_EQ_INT(ComputeOverlayLayout(64, height, true).barWidth, 6);

    // 条形图总宽度 = 核数 * 柱宽 + 间隔
    const OverlayLayout four = ComputeOverlayLayout(4, height, true);
    CHECK_EQ_INT(four.barsTotalWidth, 4 * 18 + 3 * 3);
    CHECK_EQ_INT(four.barTopY, 42);
    CheckBarsUsable(four);

    // 显示磁盘分区时，CPU 条形图必须让出底部空间，四行文字都在窗口内
    CheckDiskRowsOrdered(four, height);

    // 窗口太矮时不为磁盘分区留白，条形图仍保持正高度
    const OverlayLayout tiny = ComputeOverlayLayout(8, 100, true);
    CHECK_EQ_INT(tiny.diskPanelTopY, 100);
    CHECK(tiny.barBottomY >= tiny.barTopY + 8);

    // 核数非法值按 1 处理，不产生除零或负宽度
    const OverlayLayout zero = ComputeOverlayLayout(0, height, true);
    CHECK_EQ_INT(zero.barWidth, 18);
    CHECK_EQ_INT(zero.barsTotalWidth, 18);

    // 窗口宽度包含左右留白，随核数单调不减
    CHECK_EQ_INT(ComputeWindowWidth(4, 10), 20 + 4 * 18 + 3 * 3);
    CHECK(ComputeWindowWidth(64, 10) > ComputeWindowWidth(4, 10));
    CHECK_EQ_INT(ComputeWindowWidth(0, 10), 20 + 18);

    // DPI 缩放：175% 下行距与窗口高度同比放大，避免文字重叠
    const double scale = LayoutScaleForDpi(168);
    CHECK_NEAR(scale, 1.75, 1e-6);
    CHECK_NEAR(LayoutScaleForDpi(96), 1.0, 1e-6);
    CHECK_NEAR(LayoutScaleForDpi(0), 1.0, 1e-6);      // 异常输入回落到 1.0

    const int scaledHeight = ComputeWindowHeight(8, scale);
    CHECK(scaledHeight > height);
    const OverlayLayout scaled = ComputeOverlayLayout(8, scaledHeight, true, scale);
    CheckDiskRowsOrdered(scaled, scaledHeight);
    CheckBarsUsable(scaled);

    // 缩放后各行间距必须显著大于 100% 时的间距
    const int normalGap = four.diskLatencyY - four.diskTitleY;
    const int scaledGap = scaled.diskLatencyY - scaled.diskTitleY;
    CHECK(scaledGap > normalGap);

    // 非法缩放值按 1.0 处理
    const OverlayLayout badScale = ComputeOverlayLayout(8, height, true, -3.0);
    CheckDiskRowsOrdered(badScale, height);

    // 不显示磁盘分区时，条形图占满底部留白
    const OverlayLayout noDisk = ComputeOverlayLayout(8, height, false);
    CHECK_EQ_INT(noDisk.diskPanelTopY, height);
    CHECK_EQ_INT(noDisk.barBottomY, height - 16);
}
