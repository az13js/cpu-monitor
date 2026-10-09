cpu-monitor — 桌面置顶 CPU 每核占用率监控（也支持磁盘IO监控）
================================================================

  \cpu-monitor\


一、运行

  双击 cpu-monitor.exe 即可。

  浮窗显示在屏幕右上角，半透明、穿透点击，不干扰其他窗口操作。

  退出: 默认按 Ctrl+Shift+Q。也可配置自定义热键或完全禁用（见下文）。
     禁用热键后，须通过任务管理器结束 cpu-monitor.exe 进程。


二、浮窗内容

  每 1 秒刷新一次，分两个分区：

  1) CPU 分区

     CPU                    18%     ← 左上角标题 + 右上角总占用率
     ▁▃▅█▇▅▃▁                      ← 每核一根竖条，条顶是百分比，条底是核编号
     0 1 2 3 4 5 6 7

     颜色随占用率从绿（低）→ 黄（中）→ 红（高）。

  2) DISK IO 分区（物理磁盘整体）

     ▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂   ← 磁盘忙碌度进度条
     DISK IO
     65%                           ← 时间维度利用率 = 100% − % Idle Time
     Queue 3.40 !                  ← 平均队列长度，> 2 加 "!"
     Latency 18.0 ms !             ← 平均单次 IO 延迟，≥ 5 ms 加 "!"

     达到饱和阈值的那一行会变成橙色并加 "!"，提示磁盘已成瓶颈。

  指标说明:

    %Busy = 100% − % Idle Time   Windows 官方推荐的口径。经典的
                                 \PhysicalDisk\% Disk Time 用读时间+写时间直接相加，
                                 并发请求时会超过 100%（例如 180%），故不采用。
    Avg. Disk Queue Length       排队中的请求数，持续 > 1 说明新 IO 要等待。
                                 NVMe SSD 单队列可放宽到 > 2。
    Avg. Disk sec/Transfer       单次 IO 平均延迟，延迟陡升通常先于 %Busy 饱和。

     提示: 在 NVMe SSD 上 %Busy 常在 60-70% 时延迟就已飙升，因此队列长度和
     延迟比单纯的忙碌度更敏感，三者结合才能判断磁盘是否真的跑满。


三、配置热键

  在 exe 同目录下创建 cpu-monitor.ini 文件，格式如下：

    [hotkey]
    mod = Ctrl+Shift
    key = Q

  如果该文件不存在，默认使用 Ctrl+Shift+Q。


三、mod 参数（修饰键）

  支持以下值，用 "+" 组合多个修饰键：

    Ctrl    左或右 Ctrl 键
    Shift   左或右 Shift 键
    Alt     左或右 Alt 键
    Win     Windows 徽标键

  设为 "None" 表示不加修饰键（或禁用热键）。
  mod = None 且 key = None → 完全禁用系统热键，只能用任务管理器结束进程。

  示例：

    mod = Ctrl+Alt          → 需要同时按住 Ctrl 和 Alt
    mod = Win               → 单独按 Win 键
    mod = None              → 不注册系统热键


四、key 参数（按键）

  支持三类写法：

  1. 单字符
     A - Z  （大小写均可，内部转为大写）
     0 - 9
     None   （完全禁用按键，搭配 mod = None 使用）

  2. 功能键
     F1  F2  F3  F4  F5  F6  F7  F8  F9  F10  F11  F12

  3. 特殊键名
     Esc           退出键
     Tab           制表键
     Space         空格键
     Enter         回车键（也可写 Return）
     Backspace     退格键（也可写 Back）
     Delete        删除键（也可写 Del）
     Insert        插入键（也可写 Ins）
     Home          Home 键
     End           End 键
     PgUp          上翻页
     PgDn          下翻页
     Up            方向键 上
     Down          方向键 下
     Left          方向键 左
     Right         方向键 右


六、完整示例

  示例 1 — Ctrl+Shift+Q（默认，无须创建 ini 文件）

  示例 2 — 改为 Ctrl+Alt+X

    [hotkey]
    mod = Ctrl+Alt
    key = X

  示例 3 — 改为 Win+F3

    [hotkey]
    mod = Win
    key = F3

  示例 4 — 只用 F12，不加修饰键

    [hotkey]
    mod = None
    key = F12

  示例 5 — 完全禁用热键，用任务管理器结束

    [hotkey]
    mod = None
    key = None


七、编译

  前置: CMake ≥ 3.10 + Visual Studio 2022 Build Tools

    cmake -B build
    cmake --build build --config Release

  产物:

    build\Release\cpu-monitor.exe           浮窗程序
    build\Release\cpu-monitor-tests.exe     单元测试
    build\Release\cpu-monitor-preview.exe   离屏渲染预览（输出 BMP，用于检查布局）


八、单元测试

  无第三方测试框架，纯 assert 风格。在 build 目录下运行:

    ctest --test-dir build -C Release

  或直接运行:

    build\Release\cpu-monitor-tests.exe

  覆盖内容: 计数器换算、%Busy 计算与越界夹紧、饱和度阈值、颜色渐变、
  布局与 DPI 缩放、ini 解析、显示文本格式化，以及真实的 CPU / PDH 采样集成测试。


九、目录结构

  src\     程序源码（按职责拆分成多个模块）
  tests\   单元测试
  tools\   离屏渲染预览工具

