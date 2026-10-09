// counter_math.h — 性能计数器原始值 → 百分比 的纯数学换算
//
// 本模块不含任何 Win32 依赖，便于单元测试。
#pragma once

// 原始计数器值 → 百分比，非有限值返回 fallback
double PercentOf(double value, double fallback);

// 夹紧到 [0, 100]
double ClampPercent(double percent);

// 夹紧到 [0, 1]
double ClampFraction(double fraction);

// 一秒 = 1000 毫秒
double SecondsToMilliseconds(double seconds);
