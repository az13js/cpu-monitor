#include "disk_text.h"

#include <cstdio>
#include <cstring>

#include "disk_monitor.h"

namespace {
// 达到饱和阈值时加感叹号，提示"需要关注"
void AppendFlag(char* out, int size, bool flagged)
{
    if (flagged) strncat_s(out, (size_t)size, " !", _TRUNCATE);
}

void FormatWithSuffix(char* out, int size, const char* suffix, double value, bool flagged)
{
    char number[32];
    FormatQueueText(value, number, sizeof(number));
    sprintf_s(out, (size_t)size, "%s %s", suffix, number);
    AppendFlag(out, size, flagged);
}
}  // namespace

int FormatPercentText(double percent, bool idle, char* out, int size)
{
    if (!(percent > 0.0)) percent = 0.0;
    int value = (int)(percent + 0.5);   // 四舍五入
    if (idle) return sprintf_s(out, (size_t)size, "%d%% (IDLE)", value);
    return sprintf_s(out, (size_t)size, "%d%%", value);
}

int FormatQueueText(double queueLength, char* out, int size)
{
    if (!(queueLength > 0.0)) queueLength = 0.0;
    return sprintf_s(out, (size_t)size, "%.2f", queueLength);
}

int FormatLatencyText(double latencyMs, char* out, int size)
{
    if (!(latencyMs >= 0.0)) return sprintf_s(out, (size_t)size, "--");
    const double us = latencyMs * 1000.0;
    if (us < 100.0) return sprintf_s(out, (size_t)size, "%.1f us", us);
    if (us < 1000.0) return sprintf_s(out, (size_t)size, "%.0f us", us);
    return sprintf_s(out, (size_t)size, "%.1f ms", latencyMs);
}

DiskReadout MakeDiskReadout(bool available, const DiskStats& stats)
{
    DiskReadout out = {};
    strcpy_s(out.status, "DISK IO");

    if (!available) {
        strcpy_s(out.status, "counter unavailable");
        return out;
    }
    if (!stats.valid) {
        strcpy_s(out.status, "sampling...");
        return out;
    }

    FormatPercentText(stats.busyPercent, false, out.utilization, sizeof(out.utilization));
    FormatWithSuffix(out.queue, sizeof(out.queue), "Queue", stats.queueLength, stats.queueSaturated);

    sprintf_s(out.latency, sizeof(out.latency), "Latency %s", stats.latencyText);
    AppendFlag(out.latency, sizeof(out.latency), stats.latencySaturated);
    return out;
}
