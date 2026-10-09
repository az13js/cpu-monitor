// preview_main.cpp — 离屏渲染浮窗到 BMP，用于人工检查布局（不依赖真实 CPU / 磁盘活动）
#include <windows.h>

#include <cstdio>
#include <vector>

#include "cpu_monitor.h"
#include "disk_monitor.h"
#include "disk_text.h"
#include "overlay_layout.h"
#include "overlay_window.h"

namespace {
const int kWidth = 600;

// 覆盖低 / 中 / 高负载三种颜色的伪造占用率
std::vector<float> FakeUsage()
{
    const float samples[] = { 0.04f, 0.18f, 0.42f, 0.61f, 0.79f, 0.93f, 0.55f, 0.27f };
    return std::vector<float>(samples, samples + sizeof(samples) / sizeof(samples[0]));
}

bool SaveBitmapToFile(HBITMAP bitmap, HDC dc, const char* path, int width, int height)
{
    BITMAPINFOHEADER header = {};
    header.biSize        = sizeof(header);
    header.biWidth       = width;
    header.biHeight      = -height;   // 自顶向下
    header.biPlanes      = 1;
    header.biBitCount    = 24;
    header.biCompression = BI_RGB;

    const int stride = ((width * 3 + 3) / 4) * 4;
    std::vector<unsigned char> pixels((size_t)stride * (size_t)height);

    if (GetDIBits(dc, bitmap, 0, (UINT)height, pixels.data(),
                  (BITMAPINFO*)&header, DIB_RGB_COLORS) == 0)
        return false;

    BITMAPFILEHEADER fileHeader = {};
    fileHeader.bfType    = 0x4D42;   // "BM"
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize    = fileHeader.bfOffBits + (DWORD)pixels.size();

    FILE* file = nullptr;
    if (fopen_s(&file, path, "wb") != 0 || !file) return false;
    fwrite(&fileHeader, sizeof(fileHeader), 1, file);
    fwrite(&header, sizeof(header), 1, file);
    fwrite(pixels.data(), 1, pixels.size(), file);
    fclose(file);
    return true;
}

// 打印行距与实测文字高度，便于判断排版是否过空
void ReportMetrics(HDC dc, double scale, int clientHeight)
{
    const OverlayLayout layout = ComputeOverlayLayout(8, clientHeight, true, scale);

    HFONT font = CreateFontA(11, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                             "Consolas");
    HGDIOBJ oldFont = SelectObject(dc, font);
    SIZE textSize = {};
    GetTextExtentPoint32A(dc, "Queue 3.40 !", 12, &textSize);
    SelectObject(dc, oldFont);
    DeleteObject(font);

    std::printf("pitch=%d textHeight=%ld rowGap=%d title=%d value=%d queue=%d latency=%d "
                "panelTop=%d height=%d\n",
                layout.diskValueY - layout.diskTitleY, (long)textSize.cy,
                layout.diskValueY - layout.diskTitleY - (int)textSize.cy,
                layout.diskTitleY, layout.diskValueY, layout.diskQueueY,
                layout.diskLatencyY, layout.diskPanelTopY, clientHeight);
    std::fflush(stdout);
}

int RenderPreview(const char* outPath, const DiskStats& stats)
{
    // 用屏幕 DC 建内存 DC，保证字体 / DPI 与真实浮窗一致（96 DPI 内存 DC 会低估字体高度）
    HDC screenDC = GetDC(nullptr);
    const double scale = LayoutScaleForDpi(screenDC ? GetDeviceCaps(screenDC, LOGPIXELSY) : 96);
    const int height = ComputeWindowHeight(0, scale);

    BITMAPINFO info = {};
    info.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth       = kWidth;
    info.bmiHeader.biHeight      = -height;
    info.bmiHeader.biPlanes      = 1;
    info.bmiHeader.biBitCount    = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(screenDC, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bitmap) { ReleaseDC(nullptr, screenDC); return 1; }

    HDC memDC = CreateCompatibleDC(screenDC);
    HGDIOBJ oldBitmap = SelectObject(memDC, bitmap);

    CpuMonitor cpu;
    cpu.SetUsageForTesting(FakeUsage());
    DiskMonitor disk;
    disk.SetStatsForTesting(stats);

    PaintOverlay(memDC, RECT{ 0, 0, kWidth, height }, cpu, disk, true, "Ctrl+Shift+Q");
    ReportMetrics(memDC, scale, height);

    const bool saved = SaveBitmapToFile(bitmap, memDC, outPath, kWidth, height);

    SelectObject(memDC, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
    return saved ? 0 : 1;
}
}  // namespace

int main(int argc, char** argv)
{
    SetProcessDPIAware();   // 与浮窗进程保持相同的 DPI 语义

    const char* outPath = argc > 1 ? argv[1] : "preview.bmp";
    DiskStats stats = MakeDiskStats(35.0, 3.4, 0.018, DiskThresholds{});   // %Busy = 65%
    return RenderPreview(outPath, stats);
}
