// cpu-monitor - desktop overlay showing per-core CPU usage
// Exit: Ctrl+Shift+Q
#include <windows.h>
#include <vector>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// — NtQuerySystemInformation (ntdll.dll) —
typedef LONG NTSTATUS;
#define SystemProcessorPerformanceInformation 8

typedef struct {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER Reserved1[2];
    ULONG         Reserved2;
} SPPI;

typedef NTSTATUS (WINAPI *PNtQuerySystemInformation)(
    ULONG  SystemInformationClass,
    PVOID  SystemInformation,
    ULONG  SystemInformationLength,
    PULONG ReturnLength
);

// — 全局状态 —
static int                g_numCores = 0;
static std::vector<SPPI>  g_prevInfo;
static std::vector<float> g_cpuUsage;
static PNtQuerySystemInformation g_NtQuery = nullptr;

// — 热键配置 —
static UINT g_hotkeyMod = MOD_CONTROL | MOD_SHIFT;
static UINT g_hotkeyVk  = 'Q';
static char g_hotkeyLabel[64] = "Ctrl+Shift+Q";
static bool g_hasHotkey = true;   // false = 完全禁用热键

// — 读取每核 CPU 时间，计算占用率 —
static void UpdateUsage()
{
    ULONG len = sizeof(SPPI) * g_numCores;
    std::vector<SPPI> info(g_numCores);
    if (g_NtQuery(SystemProcessorPerformanceInformation, info.data(), len, &len) != 0)
        return;

    if (!g_prevInfo.empty()) {
        for (int i = 0; i < g_numCores; i++) {
            ULONGLONG dIdle  = info[i].IdleTime.QuadPart    - g_prevInfo[i].IdleTime.QuadPart;
            ULONGLONG sysNow = info[i].KernelTime.QuadPart  + info[i].UserTime.QuadPart;
            ULONGLONG sysPrev= g_prevInfo[i].KernelTime.QuadPart + g_prevInfo[i].UserTime.QuadPart;
            ULONGLONG dTotal = sysNow - sysPrev;
            g_cpuUsage[i] = dTotal > 0 ? 1.0f - (float)dIdle / (float)dTotal : 0.0f;
        }
    }
    g_prevInfo = info;
}

// — 解析修饰键字符串 "Ctrl+Shift" → MOD_* flags —
static void ParseModifiers(const char* str, UINT* mod)
{
    *mod = 0;
    if (!str || str[0] == '\0' || _stricmp(str, "None") == 0) return;

    char buf[64];
    strcpy_s(buf, str);
    char* ctx = nullptr;
    char* tok = strtok_s(buf, "+", &ctx);
    while (tok) {
        while (*tok == ' ') tok++;
        char* end = tok + strlen(tok) - 1;
        while (end > tok && *end == ' ') *end-- = '\0';

        if      (_stricmp(tok, "Ctrl")  == 0) *mod |= MOD_CONTROL;
        else if (_stricmp(tok, "Shift") == 0) *mod |= MOD_SHIFT;
        else if (_stricmp(tok, "Alt")   == 0) *mod |= MOD_ALT;
        else if (_stricmp(tok, "Win")   == 0) *mod |= MOD_WIN;

        tok = strtok_s(nullptr, "+", &ctx);
    }
}

// — 解析按键字符串 "Q" / "F1" / "Esc" → 虚拟键码 —
static UINT ParseKey(const char* str, UINT defaultVk)
{
    if (!str || str[0] == '\0') return defaultVk;
    if (_stricmp(str, "None") == 0) return 0;  // 显式禁用

    int len = (int)strlen(str);

    if (len == 1) {
        char c = str[0];
        if (c >= 'a' && c <= 'z') return (UINT)(c - 'a' + 'A');
        if (c >= 'A' && c <= 'Z') return (UINT)c;
        if (c >= '0' && c <= '9') return (UINT)c;
        return defaultVk;
    }

    if (str[0] == 'F' || str[0] == 'f') {
        int fn = atoi(str + 1);
        if (fn >= 1 && fn <= 12) return VK_F1 + fn - 1;
        return defaultVk;
    }

    if (_stricmp(str, "Esc")       == 0) return VK_ESCAPE;
    if (_stricmp(str, "Tab")       == 0) return VK_TAB;
    if (_stricmp(str, "Space")     == 0) return VK_SPACE;
    if (_stricmp(str, "Enter")     == 0 || _stricmp(str, "Return") == 0) return VK_RETURN;
    if (_stricmp(str, "Back")      == 0 || _stricmp(str, "Backspace")==0) return VK_BACK;
    if (_stricmp(str, "Delete")    == 0 || _stricmp(str, "Del")    == 0) return VK_DELETE;
    if (_stricmp(str, "Insert")    == 0 || _stricmp(str, "Ins")    == 0) return VK_INSERT;
    if (_stricmp(str, "Home")      == 0) return VK_HOME;
    if (_stricmp(str, "End")       == 0) return VK_END;
    if (_stricmp(str, "PgUp")      == 0) return VK_PRIOR;
    if (_stricmp(str, "PgDn")      == 0) return VK_NEXT;
    if (_stricmp(str, "Up")        == 0) return VK_UP;
    if (_stricmp(str, "Down")      == 0) return VK_DOWN;
    if (_stricmp(str, "Left")      == 0) return VK_LEFT;
    if (_stricmp(str, "Right")     == 0) return VK_RIGHT;

    return defaultVk;
}

// — 根据修饰键 + 虚拟键生成显示标签 "Ctrl+Shift+Q" —
static void BuildHotkeyLabel()
{
    if (!g_hasHotkey) {
        strcpy_s(g_hotkeyLabel, "Task Manager");
        return;
    }

    char part[64] = {};
    int  pos = 0;
    if (g_hotkeyMod & MOD_CONTROL) pos += sprintf(part + pos, "Ctrl+");
    if (g_hotkeyMod & MOD_SHIFT)   pos += sprintf(part + pos, "Shift+");
    if (g_hotkeyMod & MOD_ALT)     pos += sprintf(part + pos, "Alt+");
    if (g_hotkeyMod & MOD_WIN)     pos += sprintf(part + pos, "Win+");
    if (pos > 0) part[--pos] = '\0';  // strip trailing '+'

    char keyName[16];
    if (g_hotkeyVk >= 'A' && g_hotkeyVk <= 'Z') {
        keyName[0] = (char)g_hotkeyVk; keyName[1] = '\0';
    } else if (g_hotkeyVk >= '0' && g_hotkeyVk <= '9') {
        keyName[0] = (char)g_hotkeyVk; keyName[1] = '\0';
    } else if (g_hotkeyVk >= VK_F1 && g_hotkeyVk <= VK_F12) {
        sprintf(keyName, "F%d", g_hotkeyVk - VK_F1 + 1);
    } else {
        const char* n = "?";
        switch (g_hotkeyVk) {
            case VK_ESCAPE: n = "Esc";       break;
            case VK_TAB:    n = "Tab";       break;
            case VK_SPACE:  n = "Space";     break;
            case VK_RETURN: n = "Enter";     break;
            case VK_BACK:   n = "Backspace"; break;
            case VK_DELETE: n = "Delete";    break;
            case VK_INSERT: n = "Insert";    break;
            case VK_HOME:   n = "Home";      break;
            case VK_END:    n = "End";       break;
            case VK_PRIOR:  n = "PgUp";      break;
            case VK_NEXT:   n = "PgDn";      break;
            case VK_UP:     n = "Up";        break;
            case VK_DOWN:   n = "Down";      break;
            case VK_LEFT:   n = "Left";      break;
            case VK_RIGHT:  n = "Right";     break;
        }
        strcpy_s(keyName, n);
    }

    sprintf(g_hotkeyLabel, "%s%s%s", part, (part[0] != '\0' && keyName[0] != '\0') ? "+" : "", keyName);
}

// — 从 cpu-monitor.ini 加载热键配置 —
static void LoadConfig()
{
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    char* dot = strrchr(exePath, '.');
    if (dot) strcpy_s(dot, MAX_PATH - (dot - exePath), ".ini");

    char modBuf[64] = {};
    char keyBuf[32] = {};
    GetPrivateProfileStringA("hotkey", "mod", "Ctrl+Shift", modBuf, sizeof(modBuf), exePath);
    GetPrivateProfileStringA("hotkey", "key", "Q",           keyBuf, sizeof(keyBuf), exePath);

    ParseModifiers(modBuf, &g_hotkeyMod);
    g_hotkeyVk = ParseKey(keyBuf, 'Q');
    g_hasHotkey = !(g_hotkeyMod == 0 && g_hotkeyVk == 0);
    BuildHotkeyLabel();
}

// — 占用率 → 渐变色 —
static COLORREF UsageColor(float u)
{
    if (u < 0.5f) {
        int g = (int)(160 + 95 * (1.0f - u * 2.0f));
        return RGB(50, g > 255 ? 255 : g, 50);
    }
    if (u < 0.8f) {
        int r = (int)(50 + 170 * (u - 0.5f) / 0.3f);
        return RGB(r > 255 ? 255 : r, 170, 30);
    }
    int g = (int)(50 * (1.0f - u) / 0.2f);
    return RGB(220, g < 0 ? 0 : g, 25);
}

// — 窗口过程 —
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        SetTimer(hwnd, 1, 1000, nullptr);
        if (g_hasHotkey) RegisterHotKey(hwnd, 1, g_hotkeyMod, g_hotkeyVk);
        return 0;
    }
    case WM_HOTKEY:
        if (wp == 1) DestroyWindow(hwnd);
        return 0;
    case WM_TIMER:
        if (wp == 1) { UpdateUsage(); InvalidateRect(hwnd, nullptr, FALSE); }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        int w = rc.right, h = rc.bottom;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, w, h);
        HGDIOBJ oldBM = SelectObject(memDC, memBM);

        // 半透明深色面板
        HBRUSH panelBr = CreateSolidBrush(RGB(16, 16, 26));
        FillRect(memDC, &rc, panelBr);
        DeleteObject(panelBr);

        // 细边框
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(50, 50, 70));
        HGDIOBJ oldPen = SelectObject(memDC, borderPen);
        HGDIOBJ nullBr = GetStockObject(NULL_BRUSH);
        SelectObject(memDC, nullBr);
        Rectangle(memDC, 0, 0, w, h);
        SelectObject(memDC, oldPen);
        DeleteObject(borderPen);

        // 标题
        HFONT titleF = CreateFontA(15, 0, 0, 0, FW_BOLD, 0, 0, 0,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
        HGDIOBJ oldF = SelectObject(memDC, titleF);
        SetTextColor(memDC, RGB(160, 160, 190));
        SetBkMode(memDC, TRANSPARENT);
        TextOutA(memDC, 8, 5, "CPU", 3);
        SelectObject(memDC, oldF);
        DeleteObject(titleF);

        // 总占用率
        if (!g_cpuUsage.empty()) {
            float total = 0;
            for (float v : g_cpuUsage) total += v;
            total /= g_numCores;

            HFONT totalF = CreateFontA(18, 0, 0, 0, FW_BOLD, 0, 0, 0,
                ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
            SelectObject(memDC, totalF);
            SetTextColor(memDC, UsageColor(total));
            char buf[16];
            sprintf(buf, "%d%%", (int)(total * 100));
            SIZE sz;
            GetTextExtentPoint32A(memDC, buf, (int)strlen(buf), &sz);
            TextOutA(memDC, w - sz.cx - 8, 2, buf, (int)strlen(buf));
            SelectObject(memDC, oldF);
            DeleteObject(totalF);
        }

        // 提示文字
        HFONT hintF = CreateFontA(11, 0, 0, 0, FW_NORMAL, 0, 0, 0,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
        oldF = SelectObject(memDC, hintF);
        SetTextColor(memDC, RGB(80, 80, 100));
        TextOutA(memDC, 8, 24, g_hotkeyLabel, (int)strlen(g_hotkeyLabel));
        SelectObject(memDC, oldF);
        DeleteObject(hintF);

        // — 每核条形图 —
        if (!g_cpuUsage.empty()) {
            // 根据核数自适应柱宽
            int barW = 18;
            if (g_numCores > 16) barW = 12;
            if (g_numCores > 32) barW = 8;
            if (g_numCores > 48) barW = 6;
            const int GAP    = 3;
            const int MARGIN = 10;
            const int TOP_Y  = 42;
            const int BTM_Y  = h - 16;
            const int BAR_H  = BTM_Y - TOP_Y;
            const int totalW = g_numCores * barW + (g_numCores - 1) * GAP;
            const int startX = (w - totalW) / 2;

            for (int i = 0; i < g_numCores; i++) {
                float u = g_cpuUsage[i];
                int x = startX + i * (barW + GAP);
                int filled = (int)(BAR_H * u);
                if (filled < 1 && u > 0.001f) filled = 1;

                // 底色
                HBRUSH bgBr = CreateSolidBrush(RGB(30, 30, 42));
                RECT bgRc = { x, TOP_Y, x + barW, BTM_Y };
                FillRect(memDC, &bgRc, bgBr);
                DeleteObject(bgBr);

                // 占用色
                HBRUSH barBr = CreateSolidBrush(UsageColor(u));
                RECT barRc = { x, BTM_Y - filled, x + barW, BTM_Y };
                FillRect(memDC, &barRc, barBr);
                DeleteObject(barBr);

                // 百分比
                HFONT pctF = CreateFontA(10, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                    ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
                SelectObject(memDC, pctF);
                SetTextColor(memDC, u > 0.85f ? RGB(255, 160, 80) : RGB(140, 140, 165));
                char pct[8];
                sprintf(pct, "%d", (int)(u * 100));
                SIZE sz;
                GetTextExtentPoint32A(memDC, pct, (int)strlen(pct), &sz);
                int ty = BTM_Y - filled - (filled > 10 ? 13 : 4);
                if (ty < TOP_Y - 10) ty = TOP_Y - 10;
                TextOutA(memDC, x + (barW - sz.cx) / 2, ty, pct, (int)strlen(pct));
                SelectObject(memDC, oldF);
                DeleteObject(pctF);

                // 核编号
                HFONT numF = CreateFontA(11, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                    ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
                SelectObject(memDC, numF);
                SetTextColor(memDC, RGB(100, 100, 125));
                char num[4];
                sprintf(num, "%d", i);
                GetTextExtentPoint32A(memDC, num, (int)strlen(num), &sz);
                TextOutA(memDC, x + (barW - sz.cx) / 2, BTM_Y + 1, num, (int)strlen(num));
                SelectObject(memDC, oldF);
                DeleteObject(numF);
            }
        }

        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBM);
        DeleteObject(memBM);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        if (g_hasHotkey) UnregisterHotKey(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// — WinMain —
int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
    // 单实例检测
    HANDLE hMutex = CreateMutexA(nullptr, FALSE, "CPUMonitorOverlay_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    SetProcessDPIAware();

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    g_numCores = (int)si.dwNumberOfProcessors;
    g_cpuUsage.resize(g_numCores);

    g_NtQuery = (PNtQuerySystemInformation)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation");
    if (!g_NtQuery) { CloseHandle(hMutex); return 1; }

    UpdateUsage();

    LoadConfig();

    // 注册窗口类
    WNDCLASSEXA wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "CPUMonitorOverlay";
    RegisterClassExA(&wc);

    // 计算窗口尺寸
    int barW = 18;
    if (g_numCores > 16) barW = 12;
    if (g_numCores > 32) barW = 8;
    if (g_numCores > 48) barW = 6;
    const int GAP   = 3;
    const int MARGIN= 10;
    int winW = MARGIN * 2 + g_numCores * barW + (g_numCores - 1) * GAP;
    int winH = 160;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int x = screenW - winW - 30;
    int y = 80;

    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        "CPUMonitorOverlay", "CPU Monitor",
        WS_POPUP,
        x, y, winW, winH,
        nullptr, nullptr, hInst, nullptr);
    if (!hwnd) { CloseHandle(hMutex); return 1; }

    SetLayeredWindowAttributes(hwnd, 0, 210, LWA_ALPHA);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    CloseHandle(hMutex);
    return (int)msg.wParam;
}
