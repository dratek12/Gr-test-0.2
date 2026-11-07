#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
#include <imgui.h>
#include <getrekt/GetRektUI.hpp>
#include <getrekt/Win32Input.hpp>
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

using Microsoft::WRL::ComPtr;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

struct V3 { float x, y, z; };
static V3 operator-(const V3& a, const V3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }

enum {
    dwEntityList = 0x271EBA8, dwLocalPlayerPawn = 0x2569948, dwViewMatrix = 0x256F250,
    dwViewAngles = 0x2580098, dwSensitivity = 0x2566C48,
    m_iHealth = 0x34C, m_lifeState = 0x354, m_iTeamNum = 0x3E7,
    m_pGameSceneNode = 0x330, m_bDormant = 0x103,
    m_pWeaponServices = 0x12F0, m_hActiveWeapon = 0x60, m_iClip1 = 0x1928,
    m_pAimPunch = 0x1598, m_punchAngle = 0x50, m_flSensitivity = 0x58
};

static HMODULE gModule = nullptr;
static uintptr_t gClient = 0;
static WNDPROC gOrigWndProc = nullptr;
static HWND gHwnd = nullptr;

static ComPtr<ID3D11Device> gDevice;
static ComPtr<ID3D11DeviceContext> gCtx;
static ComPtr<ID3D11RenderTargetView> gRtv;

static getrekt::UI gUi;
static bool gReady = false, gVisible = false, gCursor = false, gUnload = false;

struct Player {
    V3 origin{}, head{};
    int health = 0, team = 0, ammo = 0;
    bool valid = false;
};
static std::vector<Player> gPlayers;
static V3 gLocalOrigin{}, gPunch{};
static int gLocalTeam = 0;
static float gSens = 1.f;
static bool gLocalAlive = false;

static void* gOrigPresent = nullptr;
static void* gOrigPresent1 = nullptr;
static void* gOrigResize = nullptr;
static void* gOrigResize1 = nullptr;

// ---------------------------------------------------------------- log
static HANDLE gLogFile = INVALID_HANDLE_VALUE;

static void Log(const char* fmt, ...) {
    if (gLogFile == INVALID_HANDLE_VALUE) {
        wchar_t p[MAX_PATH]{};
        GetModuleFileNameW(gModule, p, MAX_PATH);
        wchar_t* s = wcsrchr(p, L'\\');
        if (s) *(s + 1) = 0;
        wcscat_s(p, L"cs2esp.log");
        gLogFile = CreateFileW(p, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (gLogFile == INVALID_HANDLE_VALUE) return;
    }
    char buf[512];
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n <= 0) return;
    if (n > static_cast<int>(sizeof buf) - 1) n = static_cast<int>(sizeof buf) - 1;
    buf[n++] = '\n';
    DWORD wrote = 0;
    WriteFile(gLogFile, buf, static_cast<DWORD>(n), &wrote, nullptr);
}

// ---------------------------------------------------------------- bezpecne cteni
template <typename T> static T R(uintptr_t a) {
    if (!a) return T{};
    __try { return *reinterpret_cast<T*>(a); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return T{}; }
}

static bool W2S(const float m[16], const V3& w, ImVec2& out, ImVec2 size) {
    float cx = m[0] * w.x + m[1] * w.y + m[2] * w.z + m[3];
    float cy = m[4] * w.x + m[5] * w.y + m[6] * w.z + m[7];
    float cw = m[12] * w.x + m[13] * w.y + m[14] * w.z + m[15];
    if (cw < 0.01f) return false;
    out.x = (size.x * 0.5f) + (cx / cw) * (size.x * 0.5f);
    out.y = (size.y * 0.5f) - (cy / cw) * (size.y * 0.5f);
    return out.x >= 0 && out.y >= 0 && out.x <= size.x && out.y <= size.y;
}

static void SendMouse(float dx, float dy) {
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dx = static_cast<LONG>(dx);
    in.mi.dy = static_cast<LONG>(dy);
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &in, sizeof(in));
}

// ---------------------------------------------------------------- data
static void Refresh() {
    gPlayers.clear();
    gLocalAlive = false;
    if (!gClient) return;

    uintptr_t local = R<uintptr_t>(gClient + dwLocalPlayerPawn);
    if (!local) return;

    int lhp = R<int>(local + m_iHealth);
    uintptr_t scene = R<uintptr_t>(local + m_pGameSceneNode);
    if (lhp > 0 && lhp <= 100 && scene) {
        gLocalAlive = true;
        gLocalOrigin = R<V3>(scene + 0xC8);
        gLocalTeam = R<int>(local + m_iTeamNum);
        gSens = R<float>(gClient + dwSensitivity + m_flSensitivity);
        uintptr_t punch = R<uintptr_t>(local + m_pAimPunch);
        gPunch = punch ? R<V3>(punch + m_punchAngle) : V3{};
    }

    uintptr_t list = R<uintptr_t>(gClient + dwEntityList);
    if (!list) return;

    for (int i = 1; i < 65; ++i) {
        uintptr_t chunk = R<uintptr_t>(list + 0x10 + (i / 512) * 8);
        if (!chunk) continue;
        uintptr_t ident = chunk + (i % 512) * 0x70;
        uintptr_t ent = R<uintptr_t>(ident);
        if (!ent) continue;
        if (static_cast<int>(R<uint32_t>(ident + 0x10) & 0x7FFF) != i) continue;

        int hp = R<int>(ent + m_iHealth);
        if (hp <= 0 || hp > 100) continue;
        if (R<uint8_t>(ent + m_lifeState)) continue;
        if (R<uint8_t>(ent + m_bDormant)) continue;

        uintptr_t es = R<uintptr_t>(ent + m_pGameSceneNode);
        if (!es) continue;
        V3 org = R<V3>(es + 0xC8);
        if (org.x == 0.f && org.y == 0.f && org.z == 0.f) continue;

        Player p;
        p.origin = org;
        p.head = V3{ org.x, org.y, org.z + 72.f };
        p.health = hp;
        p.team = R<int>(ent + m_iTeamNum);
        p.valid = true;

        uintptr_t ws = R<uintptr_t>(ent + m_pWeaponServices);
        if (ws) {
            uint32_t wh = R<uint32_t>(ws + m_hActiveWeapon) & 0x7FFF;
            if (wh) {
                uintptr_t wc = R<uintptr_t>(list + 0x10 + (wh / 512) * 8);
                if (wc) {
                    uintptr_t w = R<uintptr_t>(wc + (wh % 512) * 0x70);
                    if (w) p.ammo = R<int>(w + m_iClip1);
                }
            }
        }
        gPlayers.push_back(p);
    }
}

// ---------------------------------------------------------------- aim
static void RunAim() {
    auto& c = gUi.state.cs2;
    if (!gLocalAlive || gPlayers.empty() || !c.aimKey || !c.aim) return;
    if (!(GetAsyncKeyState(c.aimKey) & 0x8000)) return;

    V3 eye{ gLocalOrigin.x, gLocalOrigin.y, gLocalOrigin.z + 64.f };
    V3 cur = R<V3>(gClient + dwViewAngles);
    float best = c.fov;
    Player* target = nullptr;

    for (auto& p : gPlayers) {
        if (!p.valid) continue;
        V3 aim = c.bone == 0 ? p.head : V3{ p.origin.x, p.origin.y, p.origin.z + 45.f };
        V3 d = aim - eye;
        float yaw = std::atan2(-d.x, d.y) * 57.29578f;
        float pitch = std::atan2(d.z, std::sqrt(d.x * d.x + d.y * d.y)) * 57.29578f;
        float dy = std::remainder(yaw - cur.y, 360.f);
        float dp = std::remainder(pitch - cur.x, 360.f);
        float dist = std::sqrt(dy * dy + dp * dp);
        if (dist < best) { best = dist; target = &p; }
    }
    if (!target) return;

    V3 aim = c.bone == 0 ? target->head : V3{ target->origin.x, target->origin.y, target->origin.z + 45.f };
    V3 d = aim - eye;
    float yaw = std::atan2(-d.x, d.y) * 57.29578f;
    float pitch = std::atan2(d.z, std::sqrt(d.x * d.x + d.y * d.y)) * 57.29578f;
    float dy = std::remainder(yaw - cur.y, 360.f);
    float dp = std::remainder(pitch - cur.x, 360.f);
    if (c.rcs) { dy -= gPunch.y * 2.f; dp -= gPunch.x * 2.f; }

    float scale = (gSens > 0.0001f ? gSens : 1.f) * 0.022f;
    float sm = std::max(c.smooth, 1.f);
    SendMouse(dy / scale / sm, -dp / scale / sm);
}

// ---------------------------------------------------------------- esp
static void DrawEsp() {
    if (gPlayers.empty()) return;
    ImDrawList* d = ImGui::GetForegroundDrawList();
    ImVec2 size = ImGui::GetIO().DisplaySize;

    float m[16]{};
    memcpy(m, reinterpret_cast<void*>(gClient + dwViewMatrix), sizeof(m));

    auto& st = gUi.state;
    ImU32 col = ImGui::ColorConvertFloat4ToU32(st.preview.color);

    for (auto& p : gPlayers) {
        if (!p.valid) continue;
        if (st.cs2.team && p.team == gLocalTeam) continue;

        ImVec2 top, bot;
        if (!W2S(m, p.head, top, size)) continue;
        if (!W2S(m, p.origin, bot, size)) continue;
        float h = bot.y - top.y;
        if (h < 4 || h > 900) continue;
        float w = h * 0.45f;

        if (st.preview.boxes) {
            d->AddRect(ImVec2(top.x - w / 2 - 1, top.y - 1), ImVec2(top.x + w / 2 + 1, bot.y + 1), IM_COL32(0, 0, 0, 255));
            d->AddRect(ImVec2(top.x - w / 2, top.y), ImVec2(top.x + w / 2, bot.y), col);
        }
        if (st.preview.health) {
            float frac = std::clamp(p.health / 100.f, 0.f, 1.f);
            d->AddRectFilled(ImVec2(top.x - w / 2 - 6, top.y), ImVec2(top.x - w / 2 - 2, bot.y), IM_COL32(0, 0, 0, 180));
            d->AddRectFilled(ImVec2(top.x - w / 2 - 5, bot.y - (bot.y - top.y) * frac),
                ImVec2(top.x - w / 2 - 3, bot.y), IM_COL32(60, 220, 60, 255));
        }
        if (st.preview.names) d->AddText(ImVec2(top.x - w / 2, top.y - 14), col, "player");
        if (st.cs2.ammo) d->AddText(ImVec2(top.x - w / 2, bot.y + 2), col, std::to_string(p.ammo).c_str());
        if (st.preview.snaplines) d->AddLine(ImVec2(size.x * 0.5f, size.y), ImVec2(top.x, bot.y), col);
        if (st.preview.head) d->AddCircle(ImVec2(top.x, top.y + h * 0.08f), h * 0.10f, col, 16);
    }
}

// ---------------------------------------------------------------- wndproc
static LRESULT CALLBACK HookProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    if (gOrigWndProc && (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP)) {
        if (msg == WM_KEYDOWN && w == VK_INSERT && !(l & (1LL << 30))) {
            gVisible = !gVisible;
            gUi.setVisible(gVisible);
            Log("insert -> visible=%d", gVisible ? 1 : 0);
            if (!gVisible) ClipCursor(nullptr);
            return 0;
        }
        if (msg == WM_KEYDOWN && w == VK_END && !(l & (1LL << 30))) { gUnload = true; return 0; }
    }

    if (gReady) {
        if (getrekt::Win32Input(gUi, msg, w, l)) return 0;
        if (ImGui_ImplWin32_WndProcHandler(h, msg, w, l)) return 0;
    }

    if (gVisible) {
        switch (msg) {
        case WM_MOUSEMOVE: case WM_LBUTTONDOWN: case WM_LBUTTONUP:
        case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_MBUTTONDOWN: case WM_MBUTTONUP:
        case WM_MOUSEWHEEL: case WM_XBUTTONDOWN: case WM_XBUTTONUP:
        case WM_INPUT: case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR:
            return 0;
        default: break;
        }
    }
    return CallWindowProcW(gOrigWndProc, h, msg, w, l);
}

// ---------------------------------------------------------------- render
static void Render() {
    if (!gVisible) {
        if (gCursor) { while (ShowCursor(FALSE) >= 0) {} gCursor = false; }
        return;
    }
    if (!gCursor) { while (ShowCursor(TRUE) < 0) {} gCursor = true; }
    ClipCursor(nullptr);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    Refresh();
    DrawEsp();
    RunAim();

    gUi.draw();

    ImGui::Render();

    ComPtr<ID3D11RenderTargetView> prev;
    gCtx->OMGetRenderTargets(1, prev.GetAddressOf(), nullptr);
    gCtx->OMSetRenderTargets(1, gRtv.GetAddressOf(), nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    if (prev) gCtx->OMSetRenderTargets(1, prev.GetAddressOf(), nullptr);
    else gCtx->OMSetRenderTargets(0, nullptr, nullptr);
}

// ---------------------------------------------------------------- hooks
static void Unload() {
    gUnload = false;
    gReady = false;
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (gOrigWndProc && gHwnd) SetWindowLongPtrW(gHwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(gOrigWndProc));
    while (ShowCursor(TRUE) < 0) {}
    Log("unload");
    if (gLogFile != INVALID_HANDLE_VALUE) { CloseHandle(gLogFile); gLogFile = INVALID_HANDLE_VALUE; }
    FreeLibraryAndExitThread(gModule, 0);
}

static void RebuildRtv(IDXGISwapChain* sc) {
    gRtv.Reset();
    if (!gDevice) return;
    ComPtr<ID3D11Texture2D> back;
    if (SUCCEEDED(sc->GetBuffer(0, IID_PPV_ARGS(&back))))
        gDevice->CreateRenderTargetView(back.Get(), nullptr, &gRtv);
}

static HRESULT WINAPI hkResizeBuffers(IDXGISwapChain* sc, UINT n, UINT w, UINT h, DXGI_FORMAT f, UINT flags) {
    if (gRtv) { gCtx->OMSetRenderTargets(0, nullptr, nullptr); gRtv.Reset(); }
    HRESULT hr = reinterpret_cast<decltype(&hkResizeBuffers)>(gOrigResize)(sc, n, w, h, f, flags);
    if (SUCCEEDED(hr)) RebuildRtv(sc);
    return hr;
}

static HRESULT WINAPI hkResizeBuffers1(IDXGISwapChain3* sc, UINT n, UINT w, UINT h, DXGI_FORMAT f, UINT flags,
    const UINT* node, const IUnknown* const* dev) {
    if (gRtv) { gCtx->OMSetRenderTargets(0, nullptr, nullptr); gRtv.Reset(); }
    HRESULT hr = reinterpret_cast<decltype(&hkResizeBuffers1)>(gOrigResize1)(sc, n, w, h, f, flags, node, dev);
    if (SUCCEEDED(hr)) RebuildRtv(sc);
    return hr;
}

static void Setup(IDXGISwapChain* sc) {
    Log("setup: swapchain %p", static_cast<void*>(sc));

    if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(gDevice.GetAddressOf())))) {
        Log("setup: GetDevice selhal");
        gDevice.Reset();
        return;
    }
    gDevice->GetImmediateContext(gCtx.GetAddressOf());

    DXGI_SWAP_CHAIN_DESC desc{};
    sc->GetDesc(&desc);
    if (desc.OutputWindow) gHwnd = desc.OutputWindow;
    if (!gHwnd) gHwnd = FindWindowW(L"SDL_app", nullptr);
    Log("setup: hwnd %p", static_cast<void*>(gHwnd));
    if (!gHwnd) { gDevice.Reset(); gCtx.Reset(); return; }

    RebuildRtv(sc);
    Log("setup: rtv %s", gRtv ? "ok" : "FAIL");
    if (!gRtv) { gDevice.Reset(); gCtx.Reset(); return; }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui_ImplWin32_Init(gHwnd);
    ImGui_ImplDX11_Init(gDevice.Get(), gCtx.Get());
    gReady = true;
    Log("setup: ready");
}

static HRESULT WINAPI hkPresent(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (!gReady) Setup(sc);
    if (gReady && gRtv) Render();
    HRESULT hr = reinterpret_cast<decltype(&hkPresent)>(gOrigPresent)(sc, sync, flags);
    if (gUnload) Unload();
    return hr;
}

static HRESULT WINAPI hkPresent1(IDXGISwapChain1* sc, UINT sync, UINT flags, const DXGI_PRESENT_PARAMETERS* p) {
    if (!gReady) Setup(sc);
    if (gReady && gRtv) Render();
    HRESULT hr = reinterpret_cast<decltype(&hkPresent1)>(gOrigPresent1)(sc, sync, flags, p);
    if (gUnload) Unload();
    return hr;
}

// ---------------------------------------------------------------- init
static BOOL CALLBACK EnumOwn(HWND h, LPARAM p) {
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid != GetCurrentProcessId() || !IsWindowVisible(h)) return TRUE;
    wchar_t cls[128]{};
    GetClassNameW(h, cls, 128);
    Log("okno: %p trida=%ls", static_cast<void*>(h), cls);
    *reinterpret_cast<HWND*>(p) = h;
    return FALSE;
}

static DWORD WINAPI InitThread(LPVOID) {
    Log("init: start");
    Log("init: pid %lu", GetCurrentProcessId());
    Log("init: krok 1");

    for (int i = 0; i < 300; ++i) {
        gClient = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
        if (gClient) break;
        if (i % 10 == 0) Log("init: pokus %d, chyba %lu", i, GetLastError());
        Sleep(200);
    }
    Log("init: client.dll %s (%p)", gClient ? "ok" : "FAIL", reinterpret_cast<void*>(gClient));
    if (!gClient) return 0;

    for (int i = 0; i < 300 && !gHwnd; ++i) {
        EnumWindows(EnumOwn, reinterpret_cast<LPARAM>(&gHwnd));
        if (!gHwnd) Sleep(200);
    }
    Log("init: hwnd %p", static_cast<void*>(gHwnd));

    if (gHwnd && !gOrigWndProc) {
        gOrigWndProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(gHwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookProc)));
        Log("init: wndproc %s", gOrigWndProc ? "ok" : "FAIL");
    }

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = gModule;
    wc.lpszClassName = L"Cs2EspDummy";
    RegisterClassExW(&wc);

    HWND dummy = CreateWindowExW(0, wc.lpszClassName, L"dummy", WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100, nullptr, nullptr, gModule, nullptr);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = dummy;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ComPtr<ID3D11Device> dev;
    ComPtr<ID3D11DeviceContext> ctx;
    ComPtr<IDXGISwapChain> sc;
    D3D_FEATURE_LEVEL fl{};

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &sc, &dev, &fl, &ctx);
    Log("init: dummy device hr=0x%08lX", static_cast<unsigned long>(hr));
    if (FAILED(hr)) return 0;

    void** vtbl = *reinterpret_cast<void***>(sc.Get());
    Log("init: vtable %p", static_cast<void*>(vtbl));

    DWORD old = 0;
    VirtualProtect(vtbl, 48 * sizeof(void*), PAGE_EXECUTE_READWRITE, &old);
    gOrigPresent = vtbl[8];
    gOrigResize = vtbl[13];
    gOrigPresent1 = vtbl[22];
    gOrigResize1 = vtbl[39];
    vtbl[8] = &hkPresent;
    vtbl[13] = &hkResizeBuffers;
    if (gOrigPresent1) vtbl[22] = &hkPresent1;
    if (gOrigResize1) vtbl[39] = &hkResizeBuffers1;
    VirtualProtect(vtbl, 48 * sizeof(void*), old, &old);
    Log("init: hooky instalovany");

    sc.Reset(); dev.Reset(); ctx.Reset();
    DestroyWindow(dummy);
    UnregisterClassW(wc.lpszClassName, gModule);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        gModule = mod;
        DisableThreadLibraryCalls(mod);
        Log("attach");
        HANDLE t = CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        if (t) CloseHandle(t);
        else Log("CreateThread selhal: %lu", GetLastError());
    }
    return TRUE;
}