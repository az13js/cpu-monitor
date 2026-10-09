#include "config_parser.h"

#include <cstdio>
#include <cstring>

namespace {
const int kMaxTokens = 8;

// 取下一个以 sep 分隔的片段，同时去掉首尾空白。返回 nullptr 表示没有更多片段。
char* NextToken(char*& cursor, const char* sep)
{
    if (!cursor) return nullptr;
    char* token = cursor;
    char* found = strpbrk(token, sep);
    if (found) {
        *found  = '\0';
        cursor  = found + 1;
    } else {
        cursor = nullptr;
    }
    while (*token == ' ' || *token == '\t') token++;
    char* end = token + strlen(token);
    while (end > token && (end[-1] == ' ' || end[-1] == '\t')) *--end = '\0';
    return token;
}

struct KeyName {
    const char* name;
    unsigned    vk;
};

const KeyName kSpecialKeys[] = {
    { "Esc",      0x1B }, { "Tab",      0x09 }, { "Space",    0x20 },
    { "Enter",    0x0D }, { "Return",   0x0D }, { "Back",     0x08 },
    { "Backspace",0x08 }, { "Delete",   0x2E }, { "Del",      0x2E },
    { "Insert",   0x2D }, { "Ins",      0x2D }, { "Home",     0x24 },
    { "End",      0x23 }, { "PgUp",     0x21 }, { "PgDn",     0x22 },
    { "Up",       0x26 }, { "Down",     0x28 }, { "Left",     0x25 },
    { "Right",    0x27 }
};

unsigned ParseSingleCharKey(const char* text)
{
    char c = text[0];
    if (c >= 'a' && c <= 'z') return (unsigned)(c - 'a' + 'A');
    if (c >= 'A' && c <= 'Z') return (unsigned)c;
    if (c >= '0' && c <= '9') return (unsigned)c;
    return 0;
}

unsigned ParseFunctionKey(const char* text)
{
    int index = 0;
    if (sscanf_s(text + 1, "%d", &index) != 1) return 0;
    if (index < 1 || index > 12) return 0;
    return 0x70 + (unsigned)(index - 1);   // VK_F1 == 0x70
}

unsigned MatchSpecialKey(const char* text)
{
    const int count = (int)(sizeof(kSpecialKeys) / sizeof(kSpecialKeys[0]));
    for (int i = 0; i < count; i++) {
        if (_stricmp(text, kSpecialKeys[i].name) == 0) return kSpecialKeys[i].vk;
    }
    return 0;
}

int AppendPart(char* out, int size, int pos, const char* text)
{
    if (pos >= size - 1) return pos;
    int written = _snprintf_s(out + pos, (size_t)(size - pos), _TRUNCATE, "%s", text);
    return written < 0 ? size - 1 : pos + written;
}

}  // namespace

unsigned ParseModifiers(const char* text)
{
    if (!text || text[0] == '\0') return kModNone;

    char buffer[64];
    strncpy_s(buffer, text, _TRUNCATE);

    unsigned flags = kModNone;
    char* cursor = buffer;
    for (int i = 0; i < kMaxTokens; i++) {
        const char* token = NextToken(cursor, "+");
        if (!token) break;
        if      (_stricmp(token, "Ctrl")  == 0) flags |= kModControl;
        else if (_stricmp(token, "Shift") == 0) flags |= kModShift;
        else if (_stricmp(token, "Alt")   == 0) flags |= kModAlt;
        else if (_stricmp(token, "Win")   == 0) flags |= kModWin;
    }
    return flags;
}

unsigned ParseKey(const char* text, unsigned defaultVk)
{
    if (!text || text[0] == '\0') return defaultVk;
    if (_stricmp(text, "None") == 0) return 0;   // 显式禁用

    if (strlen(text) == 1) {
        unsigned vk = ParseSingleCharKey(text);
        return vk ? vk : defaultVk;
    }

    if (text[0] == 'F' || text[0] == 'f') {
        unsigned vk = ParseFunctionKey(text);
        if (vk) return vk;
    }

    unsigned special = MatchSpecialKey(text);
    return special ? special : defaultVk;
}

const char* VirtualKeyName(unsigned vk, char* scratch, int scratchSize)
{
    if (vk >= 'A' && vk <= 'Z') { scratch[0] = (char)vk; scratch[1] = '\0'; return scratch; }
    if (vk >= '0' && vk <= '9') { scratch[0] = (char)vk; scratch[1] = '\0'; return scratch; }
    if (vk >= 0x70 && vk <= 0x7B) {   // VK_F1..VK_F12
        sprintf_s(scratch, (size_t)scratchSize, "F%u", vk - 0x70 + 1);
        return scratch;
    }

    const KeyName kAliasNames[] = {
        { "Esc", 0x1B }, { "Tab", 0x09 }, { "Space", 0x20 }, { "Enter",   0x0D },
        { "Backspace", 0x08 }, { "Delete", 0x2E }, { "Insert", 0x2D },
        { "Home", 0x24 }, { "End", 0x23 }, { "PgUp", 0x21 }, { "PgDn",    0x22 },
        { "Up", 0x26 }, { "Down", 0x28 }, { "Left", 0x25 }, { "Right",    0x27 }
    };
    const int count = (int)(sizeof(kAliasNames) / sizeof(kAliasNames[0]));
    for (int i = 0; i < count; i++) {
        if (kAliasNames[i].vk == vk) return kAliasNames[i].name;
    }
    return "?";
}

int FormatHotkeyLabel(unsigned modifiers, unsigned vk, char* out, int size)
{
    if (!out || size <= 0) return 0;
    out[0] = '\0';

    if (modifiers == kModNone && vk == 0) {
        return AppendPart(out, size, 0, "Task Manager");   // 无热键，只能靠任务管理器
    }

    int pos = 0;
    if (modifiers & kModControl) pos = AppendPart(out, size, pos, pos ? "+Ctrl" : "Ctrl");
    if (modifiers & kModShift)   pos = AppendPart(out, size, pos, pos ? "+Shift" : "Shift");
    if (modifiers & kModAlt)     pos = AppendPart(out, size, pos, pos ? "+Alt" : "Alt");
    if (modifiers & kModWin)     pos = AppendPart(out, size, pos, pos ? "+Win" : "Win");

    if (vk != 0) {
        char scratch[16];
        const char* name = VirtualKeyName(vk, scratch, sizeof(scratch));
        char combined[32];
        sprintf_s(combined, (size_t)sizeof(combined), "%s%s", pos ? "+" : "", name);
        pos = AppendPart(out, size, pos, combined);
    }
    return pos;
}
