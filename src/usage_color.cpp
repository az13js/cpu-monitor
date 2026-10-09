#include "usage_color.h"

#include "counter_math.h"

namespace {
int ClampChannel(double channel)
{
    if (!(channel > 0.0)) return 0;
    if (channel > 255.0)  return 255;
    return (int)channel;
}
}  // namespace

Rgb UsageColor(double fraction)
{
    const double u = ClampFraction(fraction);

    if (u < 0.5) {                                   // 绿 →（偏亮）绿
        return Rgb{ 50, ClampChannel(160 + 95 * (1.0 - u * 2.0)), 50 };
    }
    if (u < 0.8) {                                   // 绿 → 橙
        return Rgb{ ClampChannel(50 + 170 * (u - 0.5) / 0.3), 170, 30 };
    }
    return Rgb{ 220, ClampChannel(50 * (1.0 - u) / 0.2), 25 };  // 橙 → 红
}

Rgb UsageTextColor(double fraction)
{
    if (ClampFraction(fraction) > 0.85) return Rgb{ 255, 160, 80 };
    return Rgb{ 140, 140, 165 };
}
