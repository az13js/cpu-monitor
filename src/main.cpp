// cpu-monitor — 桌面浮窗显示每核 CPU 占用率 + 物理磁盘 IO 利用率
// 退出：默认 Ctrl+Shift+Q（可在 cpu-monitor.ini 配置）
#include <windows.h>

#include "cpu_monitor.h"
#include "disk_monitor.h"
#include "hotkey_config.h"
#include "overlay_layout.h"
#include "overlay_window.h"

namespace {
const char* kSingleInstanceMutex = "CPUMonitorOverlay_SingleInstance";
const int   kMargin      = 10;
const int   kScreenRight = 30;
const int   kScreenTop   = 80;
}  // namespace

// 取 exe 同目录的 cpu-monitor.ini 路径
void MakeIniPath(char* out, int size)
{
    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    MakeIniPathBesideExe(exePath, out, size);
}

// 根据 ntdll!NtQuerySystemInformation 初始化 CPU 采样器（内部已建立首个基准）
bool InitCpuMonitor(CpuMonitor* monitor)
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) return false;
    return monitor->Attach((void*)GetProcAddress(ntdll, "NtQuerySystemInformation"));
}

// 屏幕 DPI → 布局缩放系数（必须在 SetProcessDPIAware 之后调用）
double ScreenLayoutScale()
{
    HDC screenDC = GetDC(nullptr);
    const int dpi = screenDC ? GetDeviceCaps(screenDC, LOGPIXELSY) : 96;
    if (screenDC) ReleaseDC(nullptr, screenDC);
    return LayoutScaleForDpi(dpi);
}

int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
    HANDLE mutex = CreateMutexA(nullptr, FALSE, kSingleInstanceMutex);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (mutex) CloseHandle(mutex);
        return 0;
    }

    SetProcessDPIAware();

    CpuMonitor  cpu;
    DiskMonitor disk;
    if (!InitCpuMonitor(&cpu)) { CloseHandle(mutex); return 1; }
    disk.Open();   // 失败时浮窗会显示 counter unavailable，程序仍可运行

    char iniPath[MAX_PATH] = {};
    MakeIniPath(iniPath, sizeof(iniPath));
    const HotkeyConfig hotkey = LoadHotkeyConfig(iniPath);
    InitOverlay(&cpu, &disk, hotkey);

    // 不在这里采样：Attach/Open 已建立基准，等定时器第一次触发（1 秒后）再取差值，
    // 否则会用一个只有几毫秒的窗口算出无意义的占用率
    const double scale = ScreenLayoutScale();
    const int width  = ComputeWindowWidth(cpu.coreCount(), kMargin);
    const int height = ComputeWindowHeight(cpu.coreCount(), scale);
    const int x = GetSystemMetrics(SM_CXSCREEN) - width - kScreenRight;

    HWND hwnd = CreateOverlayWindow(hInst, x, kScreenTop, width, height);
    if (!hwnd) { CloseHandle(mutex); return 1; }

    const int exitCode = RunOverlayMessageLoop();
    cpu.Detach();
    CloseHandle(mutex);
    return exitCode;
}
