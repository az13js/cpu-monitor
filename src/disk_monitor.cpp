#include "disk_monitor.h"

#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>

#include <cwchar>

#include "counter_math.h"
#include "disk_math.h"
#include "disk_text.h"

namespace {
// 计数器名（英文，与系统语言无关）。最终拼出的路径形如：
//   \PhysicalDisk(_Total)\% Idle Time
const wchar_t* kIdleCounterName    = L"% Idle Time";
const wchar_t* kQueueCounterName   = L"Avg. Disk Queue Length";
const wchar_t* kLatencyCounterName = L"Avg. Disk sec/Transfer";

const int kCounterPathChars = 128;

bool IsGoodStatus(DWORD status)
{
    return status == PDH_CSTATUS_VALID_DATA || status == PDH_CSTATUS_NEW_DATA;
}

// 读取一个计数器的 double 值，成功时写入 value
bool ReadDouble(PDH_HCOUNTER counter, double* value)
{
    PDH_FMT_COUNTERVALUE raw = {};
    if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr, &raw) != ERROR_SUCCESS)
        return false;
    if (!IsGoodStatus(raw.CStatus)) return false;
    *value = raw.doubleValue;
    return true;
}

// 用英文计数器名绑定；最低支持 Vista，Windows 11 完全兼容。
// counterPath 形如 "\\PhysicalDisk(_Total)\\% Idle Time"
bool AddEnglishCounter(PDH_HQUERY query, const wchar_t* counterPath, PDH_HCOUNTER* counter)
{
    const PDH_STATUS status = PdhAddEnglishCounterW(query, counterPath, 0, counter);
#ifdef CPU_MONITOR_DIAGNOSTICS
    if (status != ERROR_SUCCESS) {
        std::fwprintf(stderr, L"[disk_monitor] add counter failed: status=0x%08lX path=%s\n",
                      (unsigned long)status, counterPath);
    }
#endif
    return status == ERROR_SUCCESS;
}

// 拼出计数器完整路径，如 \PhysicalDisk(_Total)\% Idle Time。
// 逐段拼接而不是走 printf 格式串，避免 "% Idle Time" 里的百分号被当成格式符。
bool BuildCounterPath(const wchar_t* counterName, const wchar_t* objectName,
                      const wchar_t* instance, wchar_t* out, int size)
{
    out[0] = L'\0';
    if (wcscat_s(out, (size_t)size, L"\\") != 0) return false;
    if (wcscat_s(out, (size_t)size, objectName) != 0) return false;
    if (wcscat_s(out, (size_t)size, L"(") != 0) return false;
    if (wcscat_s(out, (size_t)size, instance) != 0) return false;
    if (wcscat_s(out, (size_t)size, L")\\") != 0) return false;
    return wcscat_s(out, (size_t)size, counterName) == 0;
}

// 依次绑定一个计数器，失败时返回 false
bool AddOneCounter(PDH_HQUERY query, const wchar_t* counterName, const wchar_t* objectName,
                   const wchar_t* instance, PDH_HCOUNTER* counter)
{
    wchar_t path[kCounterPathChars];
    if (!BuildCounterPath(counterName, objectName, instance, path, kCounterPathChars))
        return false;
    return AddEnglishCounter(query, path, counter);
}
}  // namespace

DiskStats MakeEmptyDiskStats()
{
    DiskStats stats;
    stats.valid       = false;
    stats.idlePercent = 0.0;
    stats.busyPercent = 0.0;
    stats.queueLength = 0.0;
    stats.latencyMs   = -1.0;
    FormatLatencyText(stats.latencyMs, stats.latencyText, (int)sizeof(stats.latencyText));
    return stats;
}

DiskStats MakeDiskStats(double idlePercent, double queueLength, double secondsPerTransfer,
                        const DiskThresholds& thresholds)
{
    DiskStats stats;
    stats.valid       = true;
    stats.idlePercent = ClampPercent(idlePercent);
    stats.busyPercent = IdleToBusyPercent(idlePercent);
    stats.queueLength = queueLength > 0.0 ? queueLength : 0.0;   // 队列无数据时为负
    stats.latencyMs   = TransferLatencyMs(secondsPerTransfer);
    FormatLatencyText(stats.latencyMs, stats.latencyText, (int)sizeof(stats.latencyText));

    stats.highUtilization  = stats.busyPercent >= thresholds.highUtilizationPercent;
    stats.queueSaturated   = stats.queueLength > thresholds.saturatedQueueLength;
    stats.latencySaturated = IsLatencySaturated(stats.latencyMs, thresholds.saturatedLatencyMs);
    return stats;
}

DiskMonitor::~DiskMonitor()
{
    if (m_query) {
        PdhCloseQuery((PDH_HQUERY)m_query);
        m_query = nullptr;
    }
}

bool DiskMonitor::Open(const wchar_t* objectName, const wchar_t* instance)
{
    if (m_query) return true;

    PDH_HQUERY query = nullptr;
    if (PdhOpenQueryW(nullptr, 0, &query) != ERROR_SUCCESS) return false;

    m_query = query;
    if (!AddCounters(objectName, instance)) {
        PdhCloseQuery(query);
        m_query = nullptr;
        return false;
    }
    return true;
}

bool DiskMonitor::AddCounters(const wchar_t* objectName, const wchar_t* instance)
{
    PDH_HQUERY query = (PDH_HQUERY)m_query;

    if (!AddOneCounter(query, kIdleCounterName, objectName, instance, &m_idle))
        return false;
    if (!AddOneCounter(query, kQueueCounterName, objectName, instance, &m_queue))
        return false;
    return AddOneCounter(query, kLatencyCounterName, objectName, instance, &m_latency);
}

bool DiskMonitor::ReadCounters()
{
    double idle = 0.0, queue = 0.0, latencySeconds = 0.0;
    if (!ReadDouble((PDH_HCOUNTER)m_idle, &idle)) return false;
    if (!ReadDouble((PDH_HCOUNTER)m_queue, &queue)) return false;
    if (!ReadDouble((PDH_HCOUNTER)m_latency, &latencySeconds)) return false;

    m_stats = MakeDiskStats(idle, queue, latencySeconds, m_thresholds);
    return true;
}

void DiskMonitor::Sample()
{
    if (!m_query) return;

    if (PdhCollectQueryData((PDH_HQUERY)m_query) != ERROR_SUCCESS) {
        m_stats.valid = false;
        return;
    }
    // 第一次采集只建立基准，此时读取的值无意义（LEARN.md），有效值要等下一次采样
    if (!ReadCounters()) m_stats.valid = false;
}
