// test_main.cpp — 单元测试入口：依次运行各模块测试并汇总结果
#include <cstdio>

#include "test_assert.h"

void RunCounterMathTests();
void RunDiskMathTests();
void RunUsageColorTests();
void RunOverlayLayoutTests();
void RunConfigParserTests();
void RunDiskTextTests();
void RunCpuMonitorTests();
void RunDiskMonitorTests();

namespace {
struct Suite {
    const char* name;
    void (*run)();
};

const Suite kSuites[] = {
    { "counter_math",   RunCounterMathTests },
    { "disk_math",      RunDiskMathTests },
    { "usage_color",    RunUsageColorTests },
    { "overlay_layout", RunOverlayLayoutTests },
    { "config_parser",  RunConfigParserTests },
    { "disk_text",      RunDiskTextTests },
    { "cpu_monitor",    RunCpuMonitorTests },
    { "disk_monitor",   RunDiskMonitorTests }
};
}  // namespace

int main()
{
    std::printf("== cpu-monitor unit tests ==\n");

    for (const Suite& suite : kSuites) {
        const int before = TestFailureCount();
        suite.run();
        std::printf("[%s] %s\n", suite.name,
                    TestFailureCount() == before ? "ok" : "FAILED");
        std::fflush(stdout);
    }

    return TestSummary("cpu-monitor");
}
