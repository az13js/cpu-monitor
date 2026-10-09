#include "cpu_monitor.h"

#include <windows.h>

namespace {
typedef LONG NTSTATUS;
const ULONG kSystemProcessorPerformanceInformation = 8;

// SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION（仅取需要的字段）
struct PerformanceInfo {
    LARGE_INTEGER idle;
    LARGE_INTEGER kernel;
    LARGE_INTEGER user;
    LARGE_INTEGER reserved1[2];
    ULONG         reserved2;
};

typedef NTSTATUS(WINAPI* NtQuerySystemInformationFn)(
    ULONG systemInformationClass, void* systemInformation,
    ULONG systemInformationLength, ULONG* returnLength);

// 无效增量（计数器回绕 / 复位）时视为 0 占用
bool IsUsableDelta(long long total, long long idle)
{
    return total > 0 && idle >= 0 && idle <= total;
}
}  // namespace

std::vector<float> ComputeUsage(const std::vector<CoreTimes>& prev,
                                const std::vector<CoreTimes>& now)
{
    std::vector<float> usage(now.size(), 0.0f);
    if (prev.size() != now.size()) return usage;   // 首次采样只建立基准

    for (size_t i = 0; i < now.size(); i++) {
        long long total = (now[i].kernel - prev[i].kernel) + (now[i].user - prev[i].user);
        long long idle  = now[i].idle - prev[i].idle;
        if (!IsUsableDelta(total, idle)) continue;
        usage[i] = (float)(1.0 - (double)idle / (double)total);
    }
    return usage;
}

float AverageUsage(const std::vector<float>& usage)
{
    if (usage.empty()) return 0.0f;
    double sum = 0.0;
    for (float value : usage) sum += value;
    return (float)(sum / (double)usage.size());
}

bool CpuMonitor::Attach(void* ntQuerySystemInformation)
{
    m_query = ntQuerySystemInformation;
    if (!m_query) return false;

    SYSTEM_INFO info = {};
    GetSystemInfo(&info);
    const int cores = (int)info.dwNumberOfProcessors;
    if (cores < 1) return false;

    m_prev.assign((size_t)cores, CoreTimes{});
    m_usage.assign((size_t)cores, 0.0f);
    Sample();   // 建立基准
    return true;
}

void CpuMonitor::Detach()
{
    m_query = nullptr;
    m_prev.clear();
    m_usage.clear();
}

bool CpuMonitor::Sample()
{
    if (!m_query || m_prev.empty()) return false;

    const int cores = (int)m_prev.size();
    std::vector<PerformanceInfo> raw((size_t)cores);
    ULONG length = (ULONG)(sizeof(PerformanceInfo) * (size_t)cores);

    NtQuerySystemInformationFn query = (NtQuerySystemInformationFn)m_query;
    if (query(kSystemProcessorPerformanceInformation, raw.data(), length, &length) != 0)
        return false;

    std::vector<CoreTimes> now((size_t)cores);
    for (int i = 0; i < cores; i++) {
        now[(size_t)i].idle   = raw[(size_t)i].idle.QuadPart;
        now[(size_t)i].kernel = raw[(size_t)i].kernel.QuadPart;
        now[(size_t)i].user   = raw[(size_t)i].user.QuadPart;
    }

    m_usage = ComputeUsage(m_prev, now);
    m_prev  = now;
    return true;
}
