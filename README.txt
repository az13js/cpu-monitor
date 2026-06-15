cpu-monitor — 桌面置顶 CPU 每核占用率监控
=============================================

  \cpu-monitor\


一、运行

  双击 cpu-monitor.exe 即可。

  浮窗显示在屏幕右上角，半透明、穿透点击，不干扰其他窗口操作。

  退出: 默认按 Ctrl+Shift+Q。也可配置自定义热键或完全禁用（见下文）。
     禁用热键后，须通过任务管理器结束 cpu-monitor.exe 进程。


二、配置热键

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


五、完整示例

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


六、编译

  前置: CMake ≥ 3.10 + Visual Studio 2022 Build Tools

    cmake -B build
    cmake --build build --config Release

  产物: build\Release\cpu-monitor.exe
