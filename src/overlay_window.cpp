#include "overlay_window.h"

#include <windows.h>

#include <cstdio>
#include <cstring>

#include "counter_math.h"
#include "cpu_monitor.h"
#include "disk_math.h"
#include "disk_monitor.h"
#include "disk_text.h"
#include "overlay_layout.h"
#include "usage_color.h"

namespace {
const char* kClassName      = "CPUMonitorOverlay";
const char* kWindowTitle    = "CPU Monitor";
const int   kTimerId        = 1;
const int   kHotkeyId       = 1;
const int   kPanelAlpha     = 210;
const int   kLeftMargin     = 8;
const int   kRightMargin    = 8;
const int   kTitleBaselineY = 5;

CpuMonitor*  g_cpu  = nullptr;
DiskMonitor* g_disk = nullptr;
HotkeyConfig g_hotkey = {};
char         g_exitHint[64] = {};
double       g_layoutScale = 1.0;   // DPI 缩放系数，避免高 DPI 下文字重叠

struct FontSpec {
    int         height;
    int         weight;
    const char* face;
};

const FontSpec kTitleFont  = { 15, FW_BOLD,   "Consolas" };
const FontSpec kTotalFont  = { 18, FW_BOLD,   "Consolas" };
const FontSpec kValueFont  = { 12, FW_NORMAL, "Consolas" };
const FontSpec kHintFont   = { 11, FW_NORMAL, "Consolas" };
const FontSpec kBarFont    = { 10, FW_NORMAL, "Consolas" };
const FontSpec kCoreNumFont= { 11, FW_NORMAL, "Consolas" };

COLORREF ToColor(const Rgb& rgb)
{
    return RGB(rgb.r, rgb.g, rgb.b);
}

HFONT CreateUiFont(const FontSpec& spec)
{
    return CreateFontA(spec.height, 0, 0, 0, spec.weight, 0, 0, 0,
                       ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, spec.face);
}

// 用指定字体 / 颜色输出一行文字（透明背景）
void DrawTextLine(HDC dc, const FontSpec& font, COLORREF color, int x, int y, const char* text)
{
    HFONT textFont = CreateUiFont(font);
    HGDIOBJ oldFont = SelectObject(dc, textFont);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    TextOutA(dc, x, y, text, (int)strlen(text));
    SelectObject(dc, oldFont);
    DeleteObject(textFont);
}

int MeasureText(HDC dc, const FontSpec& font, const char* text)
{
    HFONT textFont = CreateUiFont(font);
    HGDIOBJ oldFont = SelectObject(dc, textFont);
    SIZE size = {};
    GetTextExtentPoint32A(dc, text, (int)strlen(text), &size);
    SelectObject(dc, oldFont);
    DeleteObject(textFont);
    return size.cx;
}

// 布局缩放系数 = 目标 DC 的 DPI / 96
double LayoutScaleOf(HDC dc)
{
    return LayoutScaleForDpi(GetDeviceCaps(dc, LOGPIXELSY));
}

// 水平居中输出文字
void DrawTextCentered(HDC dc, const FontSpec& font, COLORREF color, int centerX, int y,
                      const char* text)
{
    DrawTextLine(dc, font, color, centerX - MeasureText(dc, font, text) / 2, y, text);
}

void FillSolidRect(HDC dc, const RECT& rect, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

// 半透明深色面板 + 细边框
void PaintBackground(HDC dc, const RECT& client)
{
    FillSolidRect(dc, client, RGB(16, 16, 26));

    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(50, 50, 70));
    HGDIOBJ oldPen = SelectObject(dc, borderPen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, 0, 0, client.right, client.bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(borderPen);
}

// 竖直条形图的底槽 + 填充
void PaintVerticalBar(HDC dc, int x, int width, int top, int bottom, double fraction, COLORREF fill)
{
    FillSolidRect(dc, RECT{ x, top, x + width, bottom }, RGB(30, 30, 42));

    int filled = (int)((bottom - top) * ClampFraction(fraction));
    if (filled < 1 && fraction > 0.001) filled = 1;
    if (filled > 0) FillSolidRect(dc, RECT{ x, bottom - filled, x + width, bottom }, fill);
}

// 水平条形图的底槽 + 填充
void PaintHorizontalBar(HDC dc, int x, int y, int width, int height, double fraction, COLORREF fill)
{
    FillSolidRect(dc, RECT{ x, y, x + width, y + height }, RGB(30, 30, 42));

    int filled = (int)(width * ClampFraction(fraction));
    if (filled > 0) FillSolidRect(dc, RECT{ x, y, x + filled, y + height }, fill);
}
}  // namespace

const char* OverlayWindowClassName()
{
    return kClassName;
}

// — 标题行：左上角标题 + 右上角总占用率 —
void PaintHeader(HDC dc, const RECT& client, const CpuMonitor& cpu)
{
    DrawTextLine(dc, kTitleFont, RGB(160, 160, 190), kLeftMargin, kTitleBaselineY, "CPU");

    if (cpu.usage().empty()) return;

    const float total = AverageUsage(cpu.usage());
    char text[16];
    sprintf_s(text, "%d%%", (int)(total * 100));

    DrawTextLine(dc, kTotalFont, ToColor(UsageColor(total)),
                 client.right - MeasureText(dc, kTotalFont, text) - kLeftMargin,
                 kTitleBaselineY - 3, text);
}

// — 每核条形图 —
void PaintCpuBars(HDC dc, const CpuMonitor& cpu, const OverlayLayout& layout,
                  int clientWidth)
{
    const std::vector<float>& usage = cpu.usage();
    if (usage.empty()) return;

    const int cores  = (int)usage.size();
    const int startX = (clientWidth - layout.barsTotalWidth) / 2;
    const int barH   = layout.barBottomY - layout.barTopY;
    const int pitch  = layout.barWidth + layout.barGap;

    for (int i = 0; i < cores; i++) {
        const float u = usage[(size_t)i];
        const int x = startX + i * pitch;
        char text[8];

        PaintVerticalBar(dc, x, layout.barWidth, layout.barTopY, layout.barBottomY,
                         u, ToColor(UsageColor(u)));

        sprintf_s(text, "%d", (int)(ClampFraction(u) * 100));
        int labelY = layout.barBottomY - (int)(barH * ClampFraction(u)) - 13;
        if (labelY < layout.barTopY - 10) labelY = layout.barTopY - 10;
        DrawTextCentered(dc, kBarFont, ToColor(UsageTextColor(u)),
                         x + layout.barWidth / 2, labelY, text);

        sprintf_s(text, "%d", i);
        DrawTextCentered(dc, kCoreNumFont, RGB(100, 100, 125),
                         x + layout.barWidth / 2, layout.barBottomY + 1, text);
    }
}

// — 磁盘 IO 利用率条 + 指标文字 —
void PaintDiskPanel(HDC dc, const DiskMonitor& disk, bool available, const OverlayLayout& layout)
{
    const int barX = kLeftMargin;
    const int barW = layout.barsTotalWidth > 40 ? layout.barsTotalWidth : 40;
    const int barY = layout.diskPanelTopY;

    const DiskReadout readout = MakeDiskReadout(available, disk.stats());
    DrawTextLine(dc, kHintFont, RGB(120, 140, 180), barX, layout.diskTitleY, readout.status);

    const DiskStats& stats = disk.stats();
    if (!available || !stats.valid) {
        DrawTextLine(dc, kHintFont, RGB(120, 90, 90), barX, layout.diskValueY, readout.status);
        return;
    }

    const Rgb barColor = UsageColor(stats.busyPercent / 100.0);
    PaintHorizontalBar(dc, barX, barY, barW, layout.diskBarHeight,
                       BusyFillFraction(stats.busyPercent), ToColor(barColor));

    DrawTextLine(dc, kValueFont, ToColor(barColor), barX, layout.diskValueY, readout.utilization);
    DrawTextLine(dc, kHintFont, stats.highUtilization ? RGB(255, 170, 70) : RGB(80, 80, 100),
                 barX + MeasureText(dc, kValueFont, readout.utilization) + 6,
                 layout.diskValueY + 2, stats.highUtilization ? "BUSY" : "");

    DrawTextLine(dc, kHintFont, stats.queueSaturated ? RGB(255, 170, 70) : RGB(140, 140, 165),
                 barX, layout.diskQueueY, readout.queue);
    DrawTextLine(dc, kHintFont, stats.latencySaturated ? RGB(255, 170, 70) : RGB(140, 140, 165),
                 barX, layout.diskLatencyY, readout.latency);
}

// — 右下角的退出热键提示 —
void PaintExitHint(HDC dc, const OverlayLayout& layout, const RECT& client, const char* exitHint)
{
    if (!exitHint || exitHint[0] == '\0') return;
    const int width = MeasureText(dc, kHintFont, exitHint);
    DrawTextLine(dc, kHintFont, RGB(80, 80, 100),
                 client.right - width - kRightMargin, layout.diskHintY, exitHint);
}

void PaintOverlay(HDC target, const RECT& client, const CpuMonitor& cpu,
                  const DiskMonitor& disk, bool diskAvailable, const char* exitHint)
{
    const int width  = client.right - client.left;
    const int height = client.bottom - client.top;

    HDC memDC = CreateCompatibleDC(target);
    HBITMAP memBM = CreateCompatibleBitmap(target, width, height);
    HGDIOBJ oldBM = SelectObject(memDC, memBM);

    PaintBackground(memDC, RECT{ 0, 0, width, height });
    PaintHeader(memDC, RECT{ 0, 0, width, height }, cpu);

    const double scale = LayoutScaleOf(memDC);
    const OverlayLayout layout = ComputeOverlayLayout(cpu.coreCount(), height, true, scale);
    g_layoutScale = scale;
    PaintCpuBars(memDC, cpu, layout, width);
    PaintDiskPanel(memDC, disk, diskAvailable, layout);
    PaintExitHint(memDC, layout, RECT{ 0, 0, width, height }, exitHint);

    BitBlt(target, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBM);
    DeleteObject(memBM);
    DeleteDC(memDC);
}

void InitOverlay(CpuMonitor* cpu, DiskMonitor* disk, const HotkeyConfig& hotkey)
{
    g_cpu  = cpu;
    g_disk = disk;
    g_hotkey = hotkey;
    strncpy_s(g_exitHint, hotkey.label, _TRUNCATE);
}

void ShutdownOverlay(HWND hwnd)
{
    KillTimer(hwnd, kTimerId);
    if (g_hotkey.enabled) UnregisterHotKey(hwnd, kHotkeyId);
    g_cpu  = nullptr;
    g_disk = nullptr;
}

void RefreshOverlay(HWND hwnd)
{
    InvalidateRect(hwnd, nullptr, FALSE);
    UpdateWindow(hwnd);
}

// — 定时器：采样 CPU 与磁盘，然后重绘 —
void SampleAndRepaint(HWND hwnd)
{
    if (g_cpu) g_cpu->Sample();
    if (g_disk) g_disk->Sample();
    RefreshOverlay(hwnd);
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        SetTimer(hwnd, kTimerId, 1000, nullptr);
        if (g_hotkey.enabled) RegisterHotKey(hwnd, kHotkeyId, g_hotkey.modifiers, g_hotkey.vk);
        return 0;
    case WM_HOTKEY:
        if (wp == kHotkeyId) DestroyWindow(hwnd);
        return 0;
    case WM_TIMER:
        if (wp == kTimerId) SampleAndRepaint(hwnd);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT client;
        GetClientRect(hwnd, &client);
        if (g_cpu && g_disk) {
            PaintOverlay(dc, client, *g_cpu, *g_disk, g_disk->isOpen(), g_exitHint);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        ShutdownOverlay(hwnd);
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

HWND CreateOverlayWindow(HINSTANCE instance, int x, int y, int width, int height)
{
    WNDCLASSEXA wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = OverlayWndProc;
    wc.hInstance     = instance;
    wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = kClassName;
    if (!RegisterClassExA(&wc)) return nullptr;

    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        kClassName, kWindowTitle, WS_POPUP,
        x, y, width, height, nullptr, nullptr, instance, nullptr);
    if (!hwnd) return nullptr;

    SetLayeredWindowAttributes(hwnd, 0, kPanelAlpha, LWA_ALPHA);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return hwnd;
}

int RunOverlayMessageLoop()
{
    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}
