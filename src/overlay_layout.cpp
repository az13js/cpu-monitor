#include "overlay_layout.h"

#include <cmath>

namespace {
const int kBarGap        = 3;    // 柱间距
const int kBarTopY       = 42;   // CPU 条形图顶边
const int kBarFloorGap   = 16;   // 条形图底边距客户区底部的留白
const int kDiskPanelH    = 60;   // 磁盘分区高度（条形图 + 四行文字，最后一行紧贴底边）
const int kDiskBarHeight = 10;   // 磁盘利用率条的厚度
const int kMinDiskBarRoom= 116;  // 保证磁盘分区不被 CPU 条形图挤掉
const int kBarToDiskGap  = 6;    // CPU 条形图底边到磁盘分区的间距

// 磁盘分区各行相对于分区顶边的偏移（96 DPI 下的基准值）。行距取 13：
// 贴合 Consolas 11 号字在小数点间距下的观感，既不空旷也不拥挤。
const int kDiskRowPitch      = 13;
const int kDiskTitleOffset   = kDiskRowPitch - 1;
const int kDiskValueOffset   = kDiskTitleOffset + kDiskRowPitch;
const int kDiskQueueOffset   = kDiskValueOffset + kDiskRowPitch;
const int kDiskLatencyOffset = kDiskQueueOffset + kDiskRowPitch;
const int kDiskHintOffset    = kDiskQueueOffset;   // 右下角热键提示与 Queue 行同高

// 最后一行文字下方到客户区底边的留白（96 DPI 基准值）
const int kDiskBottomRoom = 12;

int BarWidthOf(int numCores)
{
    if (numCores > 48) return 6;
    if (numCores > 32) return 8;
    if (numCores > 16) return 12;
    return 18;
}

double NormalizeScale(double scale)
{
    return (scale > 0.0 && scale < 10.0) ? scale : 1.0;
}

int Scaled(double base, double scale)
{
    return (int)std::lround(base * scale);
}
}  // namespace

OverlayLayout ComputeOverlayLayout(int numCores, int clientHeight, bool showDisk, double scale)
{
    OverlayLayout lo = {};
    if (numCores < 1) numCores = 1;
    scale = NormalizeScale(scale);

    lo.barWidth        = BarWidthOf(numCores);
    lo.barGap          = kBarGap;
    lo.barTopY         = kBarTopY;
    lo.barsTotalWidth  = numCores * lo.barWidth + (numCores - 1) * lo.barGap;
    lo.diskBarHeight   = kDiskBarHeight;
    lo.diskPanelTopY   = clientHeight;

    int barBottom = clientHeight - kBarFloorGap;
    if (showDisk) {
        int diskTop = clientHeight - Scaled(kDiskPanelH, scale);
        if (diskTop > kMinDiskBarRoom) {   // 空间足够才让出磁盘分区
            lo.diskPanelTopY = diskTop;
            barBottom        = diskTop - kBarToDiskGap;
        }
    }
    lo.barBottomY = barBottom;

    // 极小窗口下防止条形图高度为负
    if (lo.barBottomY < lo.barTopY + 8) lo.barBottomY = lo.barTopY + 8;

    // 磁盘分区各行随分区顶边下移；行距随 DPI 缩放，避免字体放大后互相重叠
    lo.diskTitleY   = lo.diskPanelTopY + Scaled(kDiskTitleOffset, scale);
    lo.diskValueY   = lo.diskPanelTopY + Scaled(kDiskValueOffset, scale);
    lo.diskQueueY   = lo.diskPanelTopY + Scaled(kDiskQueueOffset, scale);
    lo.diskLatencyY = lo.diskPanelTopY + Scaled(kDiskLatencyOffset, scale);
    lo.diskHintY    = lo.diskPanelTopY + Scaled(kDiskHintOffset, scale);
    return lo;
}

int ComputeWindowWidth(int numCores, int margin)
{
    if (numCores < 1) numCores = 1;
    return margin * 2 + numCores * BarWidthOf(numCores) + (numCores - 1) * kBarGap;
}

int ComputeWindowHeight(int numCores, double scale)
{
    (void)numCores;   // 条形图高度自适应客户区高度，窗口高度固定
    scale = NormalizeScale(scale);
    // 窗口高度 = 磁盘分区块 + 其内最后一行文字 + 底部留白 + CPU 条形图最小高度
    return Scaled(kDiskPanelH, scale) + Scaled(kDiskBottomRoom, scale) + kMinDiskBarRoom;
}

double LayoutScaleForDpi(int dpi)
{
    if (dpi <= 0) return 1.0;
    return (double)dpi / 96.0;
}
