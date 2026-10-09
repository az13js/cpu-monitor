// hotkey_config.h — 从 cpu-monitor.ini 读取 [hotkey] 配置（Win32 INI 读取层）
#pragma once

#include "config_parser.h"

struct HotkeyConfig {
    unsigned modifiers;          // kMod* 位标志
    unsigned vk;                 // 虚拟键码，0 = 该键禁用
    char     label[64];          // 显示用标签，如 "Ctrl+Shift+Q"
    bool     enabled;            // modifiers == 0 && vk == 0 时为 false
};

// 默认配置：Ctrl+Shift+Q
HotkeyConfig DefaultHotkeyConfig();

// 从 iniPath 读取 [hotkey] 段的 mod / key；缺省值 = Ctrl+Shift / Q
HotkeyConfig LoadHotkeyConfig(const char* iniPath);

// 根据 exePath 推出同目录同名 .ini 路径，写入 out（容量 size）
void MakeIniPathBesideExe(const char* exePath, char* out, int size);
