#include "counter_math.h"

#include <cmath>

namespace {
// NaN / ±Inf 都视为无效采样
bool IsFinite(double value)
{
    return value == value && value != HUGE_VAL && value != -HUGE_VAL;
}
}  // namespace

double PercentOf(double value, double fallback)
{
    return IsFinite(value) ? value : fallback;
}

double ClampPercent(double percent)
{
    if (!(percent > 0.0)) return 0.0;   // 同时挡住 NaN
    if (percent > 100.0)  return 100.0;
    return percent;
}

double ClampFraction(double fraction)
{
    if (!(fraction > 0.0)) return 0.0;  // 同时挡住 NaN
    if (fraction > 1.0)    return 1.0;
    return fraction;
}

double SecondsToMilliseconds(double seconds)
{
    return seconds * 1000.0;
}
