// test_cpu_monitor.cpp — cpu_monitor 模块单元测试
#include "test_assert.h"

#include <windows.h>

#include <vector>

#include "cpu_monitor.h"

namespace {
const long long kTick = 10000;   // 1 ms = 10000 * 100ns

CoreTimes MakeCore(long long idleMs, long long kernelMs, long long userMs)
{
    return CoreTimes{ idleMs * kTick, kernelMs * kTick, userMs * kTick };
}

bool AllWithinRange(const std::vector<float>& usage)
{
    for (float u : usage) {
        if (u < 0.0f || u > 1.0f) return false;
    }
    return true;
}
}  // namespace

void RunCpuMonitorTests()
{
    // — 纯计算：两次采样求差 —
    const std::vector<CoreTimes> prev = { MakeCore(500, 500, 100), MakeCore(400, 400, 100) };
    // 核 0: idle +25ms, total +100ms → 75%
    // 核 1: idle +50ms, total +100ms → 50%
    const std::vector<CoreTimes> now  = { MakeCore(525, 550, 150), MakeCore(450, 450, 150) };
    std::vector<float> usage = ComputeUsage(prev, now);
    CHECK_EQ_INT(usage.size(), 2);
    CHECK_NEAR(usage[0], 0.75, 1e-4);
    CHECK_NEAR(usage[1], 0.50, 1e-4);

    // — 全空闲 / 全占用 —
    const std::vector<CoreTimes> allIdlePrev = { MakeCore(0, 0, 0) };
    const std::vector<CoreTimes> allIdleNow  = { MakeCore(100, 100, 0) };
    CHECK_NEAR(ComputeUsage(allIdlePrev, allIdleNow)[0], 0.0, 1e-4);

    const std::vector<CoreTimes> allBusyNow  = { MakeCore(0, 100, 0) };
    CHECK_NEAR(ComputeUsage(allIdlePrev, allBusyNow)[0], 1.0, 1e-4);

    // — 首次采样（只有 now，没有基准）应为 0，而不是除零 —
    std::vector<float> first = ComputeUsage({}, allIdleNow);
    CHECK_EQ_INT(first.size(), 1);
    CHECK_NEAR(first[0], 0.0, 1e-4);

    // — 计数器回绕 / 复位：不得产生负占用或 >1 的值 —
    const std::vector<CoreTimes> wrapPrev = { MakeCore(900, 900, 100) };
    const std::vector<CoreTimes> wrapNow  = { MakeCore(10, 10, 5) };
    std::vector<float> wrapped = ComputeUsage(wrapPrev, wrapNow);
    CHECK_NEAR(wrapped[0], 0.0, 1e-4);

    // — 非法增量（idle > total）按 0 处理，绝不出现负数 —
    const std::vector<CoreTimes> badPrev = { MakeCore(0, 0, 0) };
    const std::vector<CoreTimes> badNow  = { MakeCore(200, 100, 0) };
    CHECK_NEAR(ComputeUsage(badPrev, badNow)[0], 0.0, 1e-4);

    // — 核数变化时保守返回全 0，不做越界访问 —
    CHECK_EQ_INT(ComputeUsage(prev, { MakeCore(10, 10, 10) }).size(), 1);

    // — 平均值 —
    CHECK_NEAR(AverageUsage({ 1.0f, 0.0f }), 0.5, 1e-6);
    CHECK_NEAR(AverageUsage({}), 0.0, 1e-6);

    // — 真实 ntdll 采样（Windows 专用集成测试）—
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    CHECK(ntdll != nullptr);
    if (!ntdll) return;

    CpuMonitor monitor;
    const bool attached = monitor.Attach((void*)GetProcAddress(ntdll, "NtQuerySystemInformation"));
    CHECK(attached);
    if (!attached) return;

    SYSTEM_INFO sysInfo = {};
    GetSystemInfo(&sysInfo);
    CHECK_EQ_INT(monitor.coreCount(), (int)sysInfo.dwNumberOfProcessors);
    CHECK(monitor.isReady());

    bool sampled = false;
    for (int i = 0; i < 3 && !sampled; i++) {
        Sleep(20);
        sampled = monitor.Sample();
    }
    CHECK(sampled);
    CHECK_EQ_INT((int)monitor.usage().size(), monitor.coreCount());
    CHECK(AllWithinRange(monitor.usage()));

    monitor.Detach();
    CHECK_EQ_INT(monitor.coreCount(), 0);
}
