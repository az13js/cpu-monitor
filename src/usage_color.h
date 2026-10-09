// usage_color.h — 占用率 → 颜色（纯计算部分，便于单元测试）
#pragma once

struct Rgb {
    int r;
    int g;
    int b;
};

// 占用率 0..1 → 绿 → 黄 → 红 渐变
Rgb UsageColor(double fraction);

// 占用率 0..1 → 条形图上方百分比数字的颜色
Rgb UsageTextColor(double fraction);
