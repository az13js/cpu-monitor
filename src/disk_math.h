// disk_math.h — 磁盘利用率相关的纯数学换算
//
// 依据 LEARN.md：磁盘忙碌度 %Busy = 100% − \PhysicalDisk(_Total)\% Idle Time。
// 本模块不含任何 Win32 / PDH 依赖，便于单元测试。
#pragma once

// % Idle Time → 忙碌度百分比（%Busy），夹紧到 [0, 100]
double IdleToBusyPercent(double idlePercent);

// 磁盘忙碌度百分比 → 0..1 的条形图填充比例
double BusyFillFraction(double busyPercent);

// 平均单次 IO 延迟（秒） → 毫秒，负值视为无效返回负值
double TransferLatencyMs(double secondsPerTransfer);

// 延迟是否已达到"显著恶化"阈值（HDD 25 ms / SSD 5 ms 为典型经验值）
bool IsLatencySaturated(double latencyMs, double thresholdMs);
