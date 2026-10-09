// disk_monitor.h — 物理磁盘 IO 利用率采样（PDH 计数器）
//
// 依据 LEARN.md：
//   时间维度利用率 = 100% − \PhysicalDisk(_Total)\% Idle Time
//   饱和度辅助判据 = \PhysicalDisk(_Total)\Avg. Disk Queue Length
//   延迟辅助判据   = \PhysicalDisk(_Total)\Avg. Disk sec/Transfer
// 使用 PdhAddEnglishCounter 直接指定英文计数器名，避免系统语言本地化差异。
#pragma once

#include <vector>

struct DiskStats {
    bool   valid = false;          // 是否已拿到有效采样（首次采样只能建立基准）
    double idlePercent = 0.0;      // % Idle Time，夹紧到 [0, 100]
    double busyPercent = 0.0;      // 100 − % Idle Time = 时间维度利用率
    double queueLength = 0.0;      // Avg. Disk Queue Length，负数（无数据）夹紧到 0
    double latencyMs = -1.0;       // Avg. Disk sec/Transfer → 毫秒，负值表示无效
    char   latencyText[32] = {};   // 延迟的显示文本，如 "2.1 ms" / "430 us" / "--"

    // 饱和度辅助判据（参考经验阈值）
    bool highUtilization = false;  // busyPercent ≥ 85
    bool queueSaturated = false;   // queueLength > 2（SSD 单队列放宽到 2）
    bool latencySaturated = false; // latencyMs ≥ 5 ms
};

struct DiskThresholds {
    double highUtilizationPercent = 85.0;
    double saturatedQueueLength = 2.0;
    double saturatedLatencyMs = 5.0;
};

// 原始计数器读数（PDH 输出）→ 展示用统计值。纯计算，便于单元测试。
DiskStats MakeDiskStats(double idlePercent, double queueLength, double secondsPerTransfer,
                        const DiskThresholds& thresholds);

// 仅把原始读数夹紧 / 换算，不判定饱和度（用于跳过无效读数的场景）
DiskStats MakeEmptyDiskStats();

class DiskMonitor {
public:
    ~DiskMonitor();

    // 打开 PDH 查询并绑定三个 _Total 计数器；失败返回 false
    bool Open(const wchar_t* objectName = L"PhysicalDisk", const wchar_t* instance = L"_Total");

    // 采样一次。第一次调用只建立基准，返回的 stats.valid 为 false
    void Sample();

    bool             isOpen() const { return m_query != nullptr; }
    const DiskStats& stats() const { return m_stats; }

    // 注入读数（离屏渲染 / 测试用，避免依赖真实磁盘活动）
    void SetStatsForTesting(const DiskStats& stats) { m_stats = stats; }

private:
    bool AddCounters(const wchar_t* objectName, const wchar_t* instance);
    bool ReadCounters();

    void*              m_query = nullptr;
    void*              m_idle = nullptr;
    void*              m_queue = nullptr;
    void*              m_latency = nullptr;
    DiskThresholds     m_thresholds;
    DiskStats          m_stats;
};
