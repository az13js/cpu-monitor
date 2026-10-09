// overlay_window.h — 桌面浮窗：窗口生命周期 + GDI 绘制
#pragma once

#include <windows.h>

#include "hotkey_config.h"

class CpuMonitor;
class DiskMonitor;

// 创建置顶、点击穿透、半透明的浮窗；失败返回 nullptr
HWND CreateOverlayWindow(HINSTANCE instance, int x, int y, int width, int height);

// 消息循环，收到 WM_QUIT 后返回退出码
int RunOverlayMessageLoop();

// 绑定采样器与热键（必须在窗口创建前调用）
void InitOverlay(CpuMonitor* cpu, DiskMonitor* disk, const HotkeyConfig& hotkey);

// 注销热键
void ShutdownOverlay(HWND hwnd);

// 强制刷新一次窗口内容
void RefreshOverlay(HWND hwnd);

// 绘制整个浮窗内容（供 WM_PAINT 调用，也可直接用于截图 / 单元测试）
void PaintOverlay(HDC target, const RECT& client, const CpuMonitor& cpu,
                  const DiskMonitor& disk, bool diskAvailable, const char* exitHint);
