// cpu_monitor.h — 每核 CPU 占用率采样（NtQuerySystemInformation）
#pragma once

#include <vector>

// 单个核心的累计时间计数（单位 = 100 ns）
struct CoreTimes {
    long long idle;
    long long kernel;   // 含 idle 时间
    long long user;
};

// 两次采样 → 每核占用率 0..1。纯计算，便于单元测试。
std::vector<float> ComputeUsage(const std::vector<CoreTimes>& prev,
                                const std::vector<CoreTimes>& now);

// 每核占用率的算术平均值；空输入返回 0
float AverageUsage(const std::vector<float>& usage);

class CpuMonitor {
public:
    // 绑定 ntdll!NtQuerySystemInformation 地址并通过一次采样确定核数
    bool Attach(void* ntQuerySystemInformation);
    void Detach();

    int  coreCount() const { return (int)m_prev.size(); }
    bool isReady() const { return m_query != nullptr && !m_prev.empty(); }

    // 采样一次；成功时刷新 usage() 并返回 true
    bool Sample();

    const std::vector<float>& usage() const { return m_usage; }

    // 注入占用率（离屏渲染 / 测试用，避免依赖真实 CPU 活动）
    void SetUsageForTesting(const std::vector<float>& usage) { m_usage = usage; }

private:
    void*                  m_query = nullptr;   // PNtQuerySystemInformation
    std::vector<CoreTimes> m_prev;
    std::vector<float>     m_usage;
};
