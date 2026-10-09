// test_config_parser.cpp — config_parser 模块单元测试
#include "test_assert.h"

#include <cstring>

#include "config_parser.h"

namespace {
const unsigned kVkEscape = 0x1B;
const unsigned kVkF1     = 0x70;
const unsigned kVkF12    = 0x7B;
const unsigned kVkDelete = 0x2E;
const unsigned kVkUp     = 0x26;

// 解析后重新拼出标签，便于整体校验
void CheckLabel(unsigned modifiers, unsigned vk, const char* expected)
{
    char label[64] = {};
    FormatHotkeyLabel(modifiers, vk, label, sizeof(label));
    CHECK_STR(label, expected);
}
}  // namespace

void RunConfigParserTests()
{
    // — 修饰键 —
    CHECK_EQ_INT(ParseModifiers("Ctrl+Shift"), kModControl | kModShift);
    CHECK_EQ_INT(ParseModifiers("ctrl+SHIFT"), kModControl | kModShift);   // 大小写不敏感
    CHECK_EQ_INT(ParseModifiers("Alt"), kModAlt);
    CHECK_EQ_INT(ParseModifiers("Win"), kModWin);
    CHECK_EQ_INT(ParseModifiers("Ctrl+Alt+Shift+Win"),
                 kModControl | kModAlt | kModShift | kModWin);
    CHECK_EQ_INT(ParseModifiers(" Ctrl + Shift "), kModControl | kModShift);  // 容忍空格
    CHECK_EQ_INT(ParseModifiers("None"), kModNone);
    CHECK_EQ_INT(ParseModifiers(""), kModNone);
    CHECK_EQ_INT(ParseModifiers(nullptr), kModNone);
    CHECK_EQ_INT(ParseModifiers("Bogus+Ctrl"), kModControl);   // 未知项忽略
    CHECK_EQ_INT(ParseModifiers("Ctrl++Shift"), kModControl | kModShift);     // 空片段忽略

    // — 按键（单字符）—
    CHECK_EQ_INT(ParseKey("Q", 0), 'Q');
    CHECK_EQ_INT(ParseKey("q", 0), 'Q');
    CHECK_EQ_INT(ParseKey("7", 0), '7');

    // — 按键（功能键与特殊键）—
    CHECK_EQ_INT(ParseKey("F1", 0), kVkF1);
    CHECK_EQ_INT(ParseKey("f12", 0), kVkF12);
    CHECK_EQ_INT(ParseKey("Esc", 0), kVkEscape);
    CHECK_EQ_INT(ParseKey("esc", 0), kVkEscape);
    CHECK_EQ_INT(ParseKey("Del", 0), kVkDelete);
    CHECK_EQ_INT(ParseKey("Delete", 0), kVkDelete);
    CHECK_EQ_INT(ParseKey("Up", 0), kVkUp);

    // — 按键（异常输入回退到默认值）—
    CHECK_EQ_INT(ParseKey("", 'Q'), 'Q');
    CHECK_EQ_INT(ParseKey(nullptr, 'Q'), 'Q');
    CHECK_EQ_INT(ParseKey("F99", 'Q'), 'Q');
    CHECK_EQ_INT(ParseKey("Nonsense", 'Q'), 'Q');
    CHECK_EQ_INT(ParseKey("None", 'Q'), 0);    // 显式禁用

    // — 标签拼装 —
    CheckLabel(kModControl | kModShift, 'Q', "Ctrl+Shift+Q");
    CheckLabel(kModWin, kVkF1, "Win+F1");
    CheckLabel(kModNone, 'X', "X");
    CheckLabel(kModControl, kVkEscape, "Ctrl+Esc");
    CheckLabel(kModNone, 0, "Task Manager");   // 完全禁用热键

    // 缓冲区过小时按 size-1 截断，且仍以 '\0' 结尾
    char small[8] = {};
    FormatHotkeyLabel(kModControl | kModShift, 'Q', small, sizeof(small));
    CHECK_EQ_INT((int)strlen(small), 7);
    CHECK_STR(small, "Ctrl+Sh");

    // 虚拟键短名称
    char scratch[16] = {};
    CHECK_STR(VirtualKeyName('Q', scratch, sizeof(scratch)), "Q");
    CHECK_STR(VirtualKeyName(kVkF12, scratch, sizeof(scratch)), "F12");
    CHECK_STR(VirtualKeyName(kVkEscape, scratch, sizeof(scratch)), "Esc");
    CHECK_STR(VirtualKeyName(0x5A, scratch, sizeof(scratch)), "Z");
    CHECK_STR(VirtualKeyName(0x00, scratch, sizeof(scratch)), "?");
}
