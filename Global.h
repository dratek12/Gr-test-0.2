#pragma once
#include "SDK.h"

namespace G {
    extern HMODULE Module;
    extern HWND Window;
    extern WNDPROC OrigWndProc;
    extern getrekt::UI Ui;
    extern bool Ready, Visible, Cursor, Unload;
    extern ComPtr<ID3D11Device> Device;
    extern ComPtr<ID3D11DeviceContext> Ctx;
    extern ComPtr<ID3D11RenderTargetView> Rtv;
    extern std::vector<CPlayer> Players;
    extern Vec3 LocalOrigin, Punch;
    extern int LocalTeam;
    extern float Sens;
    extern bool LocalAlive;
    extern void* OrigPresent;
    extern void* OrigPresent1;
    extern void* OrigResize;
    extern void* OrigResize1;

    void Log(const char* fmt, ...);
    void CloseLog();
}