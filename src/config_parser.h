// config_parser.h — cpu-monitor.ini 的解析（纯字符串处理，便于单元测试）
#pragma once

// 修饰键 Win32 位标志（与 MOD_CONTROL 等保持一致，此处不引入 windows.h）
enum HotkeyModifier {
    kModNone    = 0x0000,
    kModAlt     = 0x0001,
    kModControl = 0x0002,
    kModShift   = 0x0004,
    kModWin     = 0x0008
};

// 解析修饰键字符串 "Ctrl+Shift" → 位标志组合。
// "None" / 空串 → kModNone
unsigned ParseModifiers(const char* text);

// 解析按键字符串 "Q" / "F1" / "Esc" → 虚拟键码（'A'..'Z'、'0'..'9'、VK_F1..VK_F12、特殊键）。
// 无法识别时返回 defaultVk；"None" 返回 0（显式禁用）。
unsigned ParseKey(const char* text, unsigned defaultVk);

// 根据修饰键 + 虚拟键生成显示标签，写入 out（容量 size）。返回写入的字符数。
int FormatHotkeyLabel(unsigned modifiers, unsigned vk, char* out, int size);

// 虚拟键码 → 短名称（"Esc" / "F1" / "Q"），未知键返回 "?"。
const char* VirtualKeyName(unsigned vk, char* scratch, int scratchSize);
