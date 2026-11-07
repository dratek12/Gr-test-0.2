#pragma once
#include <windows.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <commctrl.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include <getrekt/GetRektUI.hpp>
#include "Global.h"
#pragma comment(lib, "comctl32.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace inp {

    static HWND          g_hwnd = nullptr;
    static WNDPROC       g_old = nullptr;
    static getrekt::UI* g_gui = nullptr;
    static bool          g_open = false;
    static bool          g_inSub = false;
    static DWORD         g_lastToggle = 0;
    static bool          g_prevInsert = false;
    static bool          g_prevEnd = false;
    static void        (*g_onUnload)() = nullptr;

    inline LRESULT CALLBACK SubProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR ref);
    inline LRESULT CALLBACK MyWndProc(HWND h, UINT m, WPARAM w, LPARAM l);

    inline void SetUnloadCallback(void (*fn)()) { g_onUnload = fn; }

    inline void Log(const char* s)
    {
        wchar_t path[MAX_PATH]{};
        HMODULE hm{};
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)&inp::Log, &hm);
        if (!hm) return;
        GetModuleFileNameW(hm, path, MAX_PATH);
        wchar_t* slash = wcsrchr(path, L'\\');
        if (slash) wcscpy_s(slash + 1, (size_t)(MAX_PATH - (slash + 1 - path)), L"cs2esp.log");
        HANDLE f = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (f == INVALID_HANDLE_VALUE) return;
        DWORD w{};
        WriteFile(f, s, (DWORD)strlen(s), &w, nullptr);
        WriteFile(f, "\r\n", 2, &w, nullptr);
        CloseHandle(f);
    }

    inline void Logf(const char* fmt, ...)
    {
        char b[512]{};
        va_list a; va_start(a, fmt);
        vsnprintf(b, sizeof(b) - 1, fmt, a);
        va_end(a);
        Log(b);
    }

    inline bool MenuOpen() { return g_open; }
    inline HWND GameHwnd() { return g_hwnd; }

    inline void Unload()
    {
        Log("unload: start");
        if (g_onUnload) g_onUnload();
        if (g_hwnd && g_old) SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)g_old);
        if (g_hwnd) RemoveWindowSubclass(g_hwnd, &inp::SubProc, 0x1337);
        HMODULE hm{};
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)&inp::Unload, &hm);
        FreeLibraryAndExitThread(hm, 0);
    }

    inline void Toggle()
    {
        DWORD now = GetTickCount();
        if (now - g_lastToggle < 200) return;
        g_lastToggle = now;
        g_open = !g_open;

        if (g_open) ClipCursor(nullptr);
        else SetCursor(nullptr);

        Logf("menu: %d", (int)g_open);
    }

    inline bool HandleMsg(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        if (m == WM_KEYDOWN && w == VK_INSERT) { Toggle(); return true; }
        if (m == WM_KEYDOWN && w == VK_END) { Unload(); return true; }

        if (!g_gui) return false;

        if (m == WM_SETFOCUS) g_gui->setFocused(true);
        else if (m == WM_KILLFOCUS) g_gui->setFocused(false);

        bool consumed = false;

        if (m == WM_KEYDOWN || m == WM_SYSKEYDOWN)
            consumed = g_gui->keyEvent((int)w, true);
        else if (m == WM_KEYUP || m == WM_SYSKEYUP)
            consumed = g_gui->keyEvent((int)w, false);
        else
        {
            int k = 0;
            bool down = false;
            switch (m)
            {
            case WM_LBUTTONDOWN: k = 1; down = true; break;
            case WM_LBUTTONUP:   k = 1; break;
            case WM_RBUTTONDOWN: k = 2; down = true; break;
            case WM_RBUTTONUP:   k = 2; break;
            case WM_MBUTTONDOWN: k = 3; down = true; break;
            case WM_MBUTTONUP:   k = 3; break;
            case WM_XBUTTONDOWN: k = (HIWORD(w) == XBUTTON1) ? 5 : 6; down = true; break;
            case WM_XBUTTONUP:   k = (HIWORD(w) == XBUTTON1) ? 5 : 6; break;
            }
            if (k) consumed = g_gui->keyEvent(k, down);
        }

        ImGui_ImplWin32_WndProcHandler(h, m, w, l);
        return consumed;
    }

    inline bool ShouldBlock(UINT m)
    {
        switch (m)
        {
        case WM_MOUSEMOVE: case WM_LBUTTONDOWN: case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK: case WM_RBUTTONDOWN: case WM_RBUTTONUP:
        case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MOUSEWHEEL:
        case WM_XBUTTONDOWN: case WM_XBUTTONUP:
        case WM_INPUT: case WM_SETCURSOR:
        case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR: case WM_SYSKEYDOWN: case WM_SYSKEYUP:
            return true;
        }
        return false;
    }

    inline LRESULT CALLBACK SubProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR)
    {
        bool consumed = HandleMsg(h, m, w, l);

        g_inSub = true;
        LRESULT r = DefSubclassProc(h, m, w, l);
        g_inSub = false;

        if (g_open || consumed)
            if (ShouldBlock(m)) return 1;

        return r;
    }

    inline LRESULT CALLBACK MyWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        bool consumed = false;
        if (!g_inSub) consumed = HandleMsg(h, m, w, l);

        if (g_open || consumed)
            if (ShouldBlock(m)) return 1;

        return g_old ? CallWindowProcW(g_old, h, m, w, l) : DefWindowProcW(h, m, w, l);
    }

    inline BOOL CALLBACK EnumCb(HWND h, LPARAM)
    {
        DWORD pid{};
        GetWindowThreadProcessId(h, &pid);
        if (pid != GetCurrentProcessId()) return TRUE;
        if (!IsWindowVisible(h)) return TRUE;

        wchar_t cls[128]{};
        GetClassNameW(h, cls, 128);
        if (!wcscmp(cls, L"SDL_app") || !wcscmp(cls, L"Valve001"))
        {
            inp::g_hwnd = h;
            return FALSE;
        }
        if (!inp::g_hwnd) inp::g_hwnd = h;
        return TRUE;
    }

    inline void Init(IDXGISwapChain* sc)
    {
        if (g_hwnd && IsWindow(g_hwnd)) return;

        HWND h = nullptr;
        if (sc)
        {
            DXGI_SWAP_CHAIN_DESC d{};
            if (SUCCEEDED(sc->GetDesc(&d))) h = d.OutputWindow;
        }
        if (!h || !IsWindow(h))
        {
            g_hwnd = nullptr;
            EnumWindows(EnumCb, 0);
            h = g_hwnd;
        }
        if (!h || !IsWindow(h)) { Log("input: hwnd NENALEZEN"); return; }

        g_hwnd = h;
        Logf("input: hwnd %p", (void*)h);

        SetLastError(0);
        g_old = (WNDPROC)SetWindowLongPtrW(h, GWLP_WNDPROC, (LONG_PTR)&inp::MyWndProc);
        Logf("input: SetWindowLongPtr %p err=%lu", (void*)g_old, GetLastError());

        SetLastError(0);
        BOOL sub = SetWindowSubclass(h, &inp::SubProc, 0x1337, 0);
        Logf("input: SetWindowSubclass %d err=%lu", (int)sub, GetLastError());
    }

    inline void Tick()
    {
        bool ins = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        if (ins && !g_prevInsert) Toggle();
        g_prevInsert = ins;

        bool end = (GetAsyncKeyState(VK_END) & 0x8000) != 0;
        if (end && !g_prevEnd) Unload();
        g_prevEnd = end;
    }

    inline void Feed()
    {
        if (!g_hwnd) return;

        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = g_open;

        if (g_open)
        {
            ClipCursor(nullptr);
            SetCursor(nullptr);
        }

        POINT p{}; GetCursorPos(&p);
        ScreenToClient(g_hwnd, &p);

        bool b[5] = {
            g_open && (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0,
            g_open && (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0,
            g_open && (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0,
            g_open && (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0,
            g_open && (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0
        };

#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM >= 18700
        io.AddMousePosEvent((float)p.x, (float)p.y);
        for (int i = 0; i < 5; ++i) io.AddMouseButtonEvent(i, b[i]);
#else
        io.MousePos = ImVec2((float)p.x, (float)p.y);
        for (int i = 0; i < 5; ++i) io.MouseDown[i] = b[i];
#endif
    }

} // namespace inp