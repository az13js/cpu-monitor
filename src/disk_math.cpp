#include "disk_math.h"

#include "counter_math.h"

double IdleToBusyPercent(double idlePercent)
{
    return ClampPercent(100.0 - ClampPercent(idlePercent));
}

double BusyFillFraction(double busyPercent)
{
    return ClampFraction(ClampPercent(busyPercent) / 100.0);
}

double TransferLatencyMs(double secondsPerTransfer)
{
    if (!(secondsPerTransfer >= 0.0)) return -1.0;  // 负值 / NaN = 无效
    return SecondsToMilliseconds(secondsPerTransfer);
}

bool IsLatencySaturated(double latencyMs, double thresholdMs)
{
    return latencyMs >= 0.0 && latencyMs >= thresholdMs;
}
