#include "Global.h"

namespace G {
    HMODULE Module = nullptr;
    HWND Window = nullptr;
    WNDPROC OrigWndProc = nullptr;
    getrekt::UI Ui;
    bool Ready = false, Visible = false, Cursor = false, Unload = false;
    ComPtr<ID3D11Device> Device;
    ComPtr<ID3D11DeviceContext> Ctx;
    ComPtr<ID3D11RenderTargetView> Rtv;
    std::vector<CPlayer> Players;
    Vec3 LocalOrigin{}, Punch{};
    int LocalTeam = 0;
    float Sens = 1.f;
    bool LocalAlive = false;
    void* OrigPresent = nullptr;
    void* OrigPresent1 = nullptr;
    void* OrigResize = nullptr;
    void* OrigResize1 = nullptr;

    static HANDLE logFile = INVALID_HANDLE_VALUE;

    void Log(const char* fmt, ...) {
        if (logFile == INVALID_HANDLE_VALUE) {
            wchar_t p[MAX_PATH]{};
            GetModuleFileNameW(Module, p, MAX_PATH);
            wchar_t* s = wcsrchr(p, L'\\');
            if (s) *(s + 1) = 0;
            wcscat_s(p, MAX_PATH, L"cs2esp.log");
            logFile = CreateFileW(p, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (logFile == INVALID_HANDLE_VALUE) return;
        }
        char buf[512];
        va_list ap; va_start(ap, fmt);
        int n = vsnprintf(buf, sizeof buf, fmt, ap);
        va_end(ap);
        if (n <= 0) return;
        if (n > static_cast<int>(sizeof buf) - 1) n = static_cast<int>(sizeof buf) - 1;
        buf[n++] = '\n';
        DWORD wrote = 0;
        WriteFile(logFile, buf, static_cast<DWORD>(n), &wrote, nullptr);
    }

    void CloseLog() {
        if (logFile != INVALID_HANDLE_VALUE) { CloseHandle(logFile); logFile = INVALID_HANDLE_VALUE; }
    }
}