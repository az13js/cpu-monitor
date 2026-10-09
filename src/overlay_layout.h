// overlay_layout.h — 浮窗几何布局的纯计算（无 GDI 依赖，便于单元测试）
#pragma once

struct OverlayLayout {
    // 每核条形图
    int barWidth;
    int barGap;
    int barTopY;
    int barBottomY;     // 条形图底边（= 百分比文字基线参考）
    int barsTotalWidth;

    // 磁盘分区
    int diskPanelTopY;
    int diskBarHeight;
    int diskTitleY;
    int diskValueY;
    int diskQueueY;
    int diskLatencyY;
    int diskHintY;      // 右下角退出热键提示
};

// 根据核数、客户区高度、是否显示磁盘分区计算布局。
// scale 为 DPI 缩放系数（96 DPI = 1.0），用于放大行距，避免高 DPI 下文字重叠。
OverlayLayout ComputeOverlayLayout(int numCores, int clientHeight, bool showDisk, double scale = 1.0);

// 根据核数计算窗口宽度（含左右留白）
int ComputeWindowWidth(int numCores, int margin);

// 根据核数计算窗口高度（CPU 条形图 + 磁盘分区）
int ComputeWindowHeight(int numCores, double scale = 1.0);

// DPI → 布局缩放系数
double LayoutScaleForDpi(int dpi);
