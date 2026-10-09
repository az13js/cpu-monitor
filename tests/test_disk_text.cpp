// test_disk_text.cpp — disk_text 模块单元测试
#include "test_assert.h"

#include <cstring>

#include "disk_monitor.h"
#include "disk_text.h"

namespace {
const DiskThresholds kThresholds = {};   // 默认：85% / 队列 2 / 延迟 5 ms

// 读取显示文本的一个字段（未采样时为空串）
const char* TextOrEmpty(const char* text)
{
    return text ? text : "";
}
}  // namespace

void RunDiskTextTests()
{
    char buffer[64] = {};

    // — 百分比 —
    FormatPercentText(0.0, false, buffer, sizeof(buffer));
    CHECK_STR(buffer, "0%");
    FormatPercentText(12.4, false, buffer, sizeof(buffer));
    CHECK_STR(buffer, "12%");
    FormatPercentText(12.6, false, buffer, sizeof(buffer));
    CHECK_STR(buffer, "13%");                            // 四舍五入
    FormatPercentText(100.0, false, buffer, sizeof(buffer));
    CHECK_STR(buffer, "100%");
    FormatPercentText(-3.0, false, buffer, sizeof(buffer));
    CHECK_STR(buffer, "0%");                             // 负值兜底
    FormatPercentText(7.0, true, buffer, sizeof(buffer));
    CHECK_STR(buffer, "7% (IDLE)");

    // — 队列长度 —
    FormatQueueText(0.0, buffer, sizeof(buffer));
    CHECK_STR(buffer, "0.00");
    FormatQueueText(1.239, buffer, sizeof(buffer));
    CHECK_STR(buffer, "1.24");
    FormatQueueText(-1.0, buffer, sizeof(buffer));
    CHECK_STR(buffer, "0.00");                           // PDH 无数据时为负

    // — 延迟 —
    FormatLatencyText(-1.0, buffer, sizeof(buffer));
    CHECK_STR(buffer, "--");
    FormatLatencyText(0.00043, buffer, sizeof(buffer));
    CHECK_STR(buffer, "0.4 us");
    FormatLatencyText(0.0801, buffer, sizeof(buffer));
    CHECK_STR(buffer, "80.1 us");
    FormatLatencyText(2.15, buffer, sizeof(buffer));
    CHECK_STR(buffer, "2.1 ms");
    FormatLatencyText(0.0, buffer, sizeof(buffer));
    CHECK_STR(buffer, "0.0 us");

    // — 未打开计数器 —
    DiskReadout missing = MakeDiskReadout(false, MakeEmptyDiskStats());
    CHECK_STR(missing.status, "counter unavailable");
    CHECK_STR(TextOrEmpty(missing.utilization), "");
    CHECK_STR(MakeEmptyDiskStats().latencyText, "--");   // 无读数时延迟直接显示 "--"

    // — 已打开但还在等第二次采样 —
    DiskReadout pending = MakeDiskReadout(true, MakeEmptyDiskStats());
    CHECK_STR(pending.status, "sampling...");

    // — 正常读数 —
    DiskStats stats = MakeDiskStats(70.0, 0.5, 0.002, kThresholds);   // %Busy = 30%
    DiskReadout readout = MakeDiskReadout(true, stats);
    CHECK_STR(readout.status, "DISK IO");
    CHECK_STR(readout.utilization, "30%");
    CHECK_STR(readout.queue, "Queue 0.50");
    CHECK_STR(readout.latency, "Latency 2.0 ms");
    CHECK(!stats.highUtilization);

    // — 饱和读数：队列 > 2、延迟 ≥ 5 ms、利用率 ≥ 85% 都加 "!" —
    DiskStats busy = MakeDiskStats(5.0, 3.2, 0.030, kThresholds);      // %Busy = 95%
    DiskReadout busyText = MakeDiskReadout(true, busy);
    CHECK_STR(busyText.utilization, "95%");
    CHECK_STR(busyText.queue, "Queue 3.20 !");
    CHECK_STR(busyText.latency, "Latency 30.0 ms !");
    CHECK(busy.highUtilization);
    CHECK(busy.queueSaturated);
    CHECK(busy.latencySaturated);

    // — 阈值边界：队列恰好 2 不算饱和，利用率恰好 85% 算高负载 —
    DiskStats edge = MakeDiskStats(15.0, 2.0, 0.0049, kThresholds);    // %Busy = 85%
    CHECK(edge.highUtilization);
    CHECK(!edge.queueSaturated);
    CHECK(!edge.latencySaturated);

    // — 计数器抖动导致越界值时必须夹紧 —
    DiskStats jitter = MakeDiskStats(130.0, -0.4, -0.01, kThresholds);
    CHECK_NEAR(jitter.busyPercent, 0.0, 1e-9);
    CHECK_NEAR(jitter.queueLength, 0.0, 1e-9);
    CHECK(jitter.latencyMs < 0.0);
    DiskReadout jitterText = MakeDiskReadout(true, jitter);
    CHECK_STR(jitterText.utilization, "0%");
    CHECK_STR(jitterText.latency, "Latency --");
}
