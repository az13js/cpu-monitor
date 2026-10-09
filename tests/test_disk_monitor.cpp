// test_disk_monitor.cpp — disk_monitor 的 PDH 集成测试 + 未打开时的安全行为
#include "test_assert.h"

#include <windows.h>

#include "disk_monitor.h"

namespace {
bool WithinPercent(double value)
{
    return value >= 0.0 && value <= 100.0;
}
}  // namespace

void RunDiskMonitorTests()
{
    // — 未 Open 时所有操作必须安全无副作用 —
    DiskMonitor closed;
    CHECK(!closed.isOpen());
    CHECK(!closed.stats().valid);
    closed.Sample();                       // 不应崩溃
    CHECK(!closed.stats().valid);

    // — 真实 PDH 查询（Windows 专用集成测试）—
    DiskMonitor disk;
    const bool opened = disk.Open();
    CHECK(opened);
    if (!opened) return;
    CHECK(disk.isOpen());

    // LEARN.md：第一次采集只建立基准，有效值要等第二次采样
    disk.Sample();
    bool valid = disk.stats().valid;
    for (int i = 0; i < 20 && !valid; i++) {
        Sleep(200);
        disk.Sample();
        valid = disk.stats().valid;
    }
    CHECK(valid);
    if (!valid) return;

    const DiskStats& stats = disk.stats();
    CHECK(WithinPercent(stats.busyPercent));
    CHECK(WithinPercent(stats.idlePercent));
    CHECK_NEAR(stats.busyPercent, 100.0 - stats.idlePercent, 1e-6);
    CHECK(stats.queueLength >= 0.0);
    CHECK(stats.latencyMs >= 0.0);
    std::printf("  disk sample: busy=%.1f%% idle=%.1f%% queue=%.2f latency=%.2fms\n",
                stats.busyPercent, stats.idlePercent, stats.queueLength, stats.latencyMs);
}
