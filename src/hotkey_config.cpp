#include "hotkey_config.h"

#include <windows.h>

#include <cstring>

namespace {
const char* kIniSection = "hotkey";
const char* kDefaultMod = "Ctrl+Shift";
const char* kDefaultKey = "Q";
}  // namespace

HotkeyConfig DefaultHotkeyConfig()
{
    HotkeyConfig cfg = {};
    cfg.modifiers = ParseModifiers(kDefaultMod);
    cfg.vk        = ParseKey(kDefaultKey, 'Q');
    cfg.enabled   = (cfg.modifiers != kModNone || cfg.vk != 0);
    FormatHotkeyLabel(cfg.modifiers, cfg.vk, cfg.label, (int)sizeof(cfg.label));
    return cfg;
}

HotkeyConfig LoadHotkeyConfig(const char* iniPath)
{
    char modBuf[64] = {};
    char keyBuf[32] = {};
    GetPrivateProfileStringA(kIniSection, "mod", kDefaultMod, modBuf, sizeof(modBuf), iniPath);
    GetPrivateProfileStringA(kIniSection, "key", kDefaultKey, keyBuf, sizeof(keyBuf), iniPath);

    HotkeyConfig cfg = {};
    cfg.modifiers = ParseModifiers(modBuf);
    cfg.vk        = ParseKey(keyBuf, 'Q');
    cfg.enabled   = (cfg.modifiers != kModNone || cfg.vk != 0);
    FormatHotkeyLabel(cfg.modifiers, cfg.vk, cfg.label, (int)sizeof(cfg.label));
    return cfg;
}

void MakeIniPathBesideExe(const char* exePath, char* out, int size)
{
    if (!out || size <= 0) return;
    strncpy_s(out, (size_t)size, exePath ? exePath : "", _TRUNCATE);

    char* dot = strrchr(out, '.');
    char* sep = strrchr(out, '\\');
    if (dot && (!sep || dot > sep)) *dot = '\0';   // 去掉扩展名
    strncat_s(out, (size_t)size, ".ini", _TRUNCATE);
}
