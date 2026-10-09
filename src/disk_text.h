// disk_text.h — 磁盘指标 → 显示文本（纯格式化，便于单元测试）
#pragma once

// 浮窗磁盘分区使用的四行文本
struct DiskReadout {
    char utilization[32];   // "12%" 或 "12% (IDLE)"
    char queue[32];         // "Queue 1.24" / "Queue 1.24 !"
    char latency[32];       // "Latency 2.1 ms" / "Latency --"
    char status[64];        // "DISK IO" / "sampling..." / "counter unavailable"
};

// 写入 "12%" 或 "12% (IDLE)"；返回写入的字符数
int FormatPercentText(double percent, bool idle, char* out, int size);

// 写入队列长度，如 "1.24"；返回写入的字符数
int FormatQueueText(double queueLength, char* out, int size);

// 写入延迟，如 "2.1 ms" / "430 us"，无效值写 "--"；返回写入的字符数
int FormatLatencyText(double latencyMs, char* out, int size);

// 采样状态 + 统计值 → 浮窗要显示的四行文本（! 后缀表示达到饱和阈值）
DiskReadout MakeDiskReadout(bool available, const struct DiskStats& stats);
