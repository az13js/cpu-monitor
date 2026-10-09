# cpu-monitor

在桌面叠加显示 CPU 每核占用率 + 物理磁盘 IO 利用率的 Win32 C++ 桌面程序。

## 构建

```bash
cmake -B build
cmake --build build --config Release
```

输出：
- `build/Release/cpu-monitor.exe` — 浮窗程序
- `build/Release/cpu-monitor-tests.exe` — 单元测试（也可 `ctest --test-dir build -C Release`）
- `build/Release/cpu-monitor-preview.exe` — 离屏渲染预览，把浮窗画到 BMP 便于人工检查布局

## Key facts

- 无外部依赖；系统库只用到 `pdh` / `user32` / `gdi32`
- 代码拆分在 `src/`（见下），测试在 `tests/`
- Requires **Visual Studio Build Tools 2022** with x64 native tools command prompt
- Single-instance enforcement via named mutex `"CPUMonitorOverlay_SingleInstance"`
- Config read from `cpu-monitor.ini` (same dir as exe, on first launch will be next to CMake source dir)
- Exit hotkey default: `Ctrl+Shift+Q`; configured via `[hotkey]` section in ini
- `.gitignore` only ignores `build/` — screenshots and other files are tracked
- 采样间隔 1 秒（`SetTimer`）；磁盘计数器第一次采样只建立基准，第二次起才有效

## 模块划分

| 文件 | 职责 |
| --- | --- |
| `src/main.cpp` | WinMain：单实例、初始化采样器、创建窗口、消息循环 |
| `src/cpu_monitor.*` | 每核 CPU 累计时间采样（`NtQuerySystemInformation`）+ 纯函数 `ComputeUsage` |
| `src/disk_monitor.*` | 物理磁盘 PDH 计数器采样，输出 `DiskStats` |
| `src/counter_math.*` | 计数器原始值 → 百分比的夹紧 / 换算 |
| `src/disk_math.*` | `%Busy = 100 − % Idle Time`、延迟换算、饱和度判据 |
| `src/disk_text.*` | `DiskStats` → 浮窗显示文本（`MakeDiskReadout`） |
| `src/usage_color.*` | 占用率 → RGB 渐变色 |
| `src/overlay_layout.*` | 浮窗几何布局（柱宽、条形图区、磁盘分区各行位置、DPI 缩放） |
| `src/overlay_window.*` | 窗口过程 + GDI 绘制 |
| `src/config_parser.*` | ini 里的 mod / key 字符串解析与标签拼装 |
| `src/hotkey_config.*` | 读取 `cpu-monitor.ini` 的 `[hotkey]` 段 |

## 磁盘 IO 指标（依据 ../learn-windows-idle-time/LEARN.md）

PDH + `PdhAddEnglishCounterW` 直接使用英文计数器名，避免系统语言差异。三个计数器：

| 计数器 | 用途 |
| --- | --- |
| `\PhysicalDisk(_Total)\% Idle Time` | 时间维度利用率 = `100 − % Idle Time`，浮窗进度条 |
| `\PhysicalDisk(_Total)\Avg. Disk Queue Length` | 队列长度，>2 显示 `!`（SSD 单队列放宽阈值） |
| `\PhysicalDisk(_Total)\Avg. Disk sec/Transfer` | 平均单次 IO 延迟，≥5 ms 显示 `!` |

条与文字颜色走 `UsageColor`，达阈值时文字变橙色并加 `!`。

## MSVC quirks

- `cmake -B build` auto-detects VS 2022; no manual generator needed
- `cpu-monitor.ini` must be copied next to `cpu-monitor.exe` for runtime config to work
- 拼接 PDH 计数器路径**不要**用 `swprintf_s` 格式串：`"% Idle Time"` 里的 `%` 会被当成格式符
  （曾导致 `PdhAddEnglishCounterW` 报 `0xC0000BB9`）。`BuildCounterPath` 用 `wcscat_s` 逐段拼接
- `std::vector<CoreTimes>` 用 `assign(count, CoreTimes{})` 初始化，避免 `{0}` 触发 C4351

## 开发注意

程序的源代码需要适当地封装，使得每个函数的职责单一，避免函数过于复杂。这样可以提高代码的可读性和可维护性。建议每个函数行数不超过20，圈复杂度不超过10。遗留的旧代码在改动时如果不符合这个要求，那么在改动时顺便重构。

适当地模块化逻辑，拆分到一个或者多个文件。每个模块的职责单一，避免模块之间耦合度过高。旧代码如果不符合这个要求，改动涉及到了，就需要重构代码，使其符合要求。

函数需要编写单元测试。为了保持独立 **不要** 引入复杂的单元测试框架。可以使用简单的assert模块。
测试用 `tests/test_assert.h` 的 `CHECK` / `CHECK_EQ_INT` / `CHECK_NEAR` / `CHECK_STR` 宏，
在 `tests/test_main.cpp` 的 `kSuites` 表里注册后即可运行。

