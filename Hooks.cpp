#include "Hooks.h"
#include "Global.h"
#include "ESP.h"
#include "LegitBot.h"
#include "Misc.h"
#include "Config.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static void RebuildRtv(IDXGISwapChain* sc) {
    G::Rtv.Reset();
    if (!G::Device) return;
    ComPtr<ID3D11Texture2D> back;
    if (SUCCEEDED(sc->GetBuffer(0, IID_PPV_ARGS(&back))))
        G::Device->CreateRenderTargetView(back.Get(), nullptr, &G::Rtv);
}

static void Unload() {
    G::Unload = false;
    G::Ready = false;
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (G::OrigWndProc && G::Window)
        SetWindowLongPtrW(G::Window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(G::OrigWndProc));
    while (ShowCursor(TRUE) < 0) {}
    G::Log("unload");
    G::CloseLog();
    FreeLibraryAndExitThread(G::Module, 0);
}

static HRESULT WINAPI hkResizeBuffers(IDXGISwapChain* sc, UINT n, UINT w, UINT h, DXGI_FORMAT f, UINT flags) {
    if (G::Rtv) { G::Ctx->OMSetRenderTargets(0, nullptr, nullptr); G::Rtv.Reset(); }
    HRESULT hr = reinterpret_cast<decltype(&hkResizeBuffers)>(G::OrigResize)(sc, n, w, h, f, flags);
    if (SUCCEEDED(hr)) RebuildRtv(sc);
    return hr;
}

static HRESULT WINAPI hkResizeBuffers1(IDXGISwapChain3* sc, UINT n, UINT w, UINT h, DXGI_FORMAT f, UINT flags,
    const UINT* node, const IUnknown* const* dev) {
    if (G::Rtv) { G::Ctx->OMSetRenderTargets(0, nullptr, nullptr); G::Rtv.Reset(); }
    HRESULT hr = reinterpret_cast<decltype(&hkResizeBuffers1)>(G::OrigResize1)(sc, n, w, h, f, flags, node, dev);
    if (SUCCEEDED(hr)) RebuildRtv(sc);
    return hr;
}

static void Setup(IDXGISwapChain* sc) {
    if (G::Ready) return;
    G::Log("setup: swapchain %p", static_cast<void*>(sc));
    if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(G::Device.GetAddressOf())))) {
        G::Log("setup: GetDevice selhal"); G::Device.Reset(); return;
    }
    G::Device->GetImmediateContext(G::Ctx.GetAddressOf());

    DXGI_SWAP_CHAIN_DESC desc{};
    sc->GetDesc(&desc);
    if (desc.OutputWindow) G::Window = desc.OutputWindow;
    if (!G::Window) G::Window = FindWindowW(L"SDL_app", nullptr);
    if (!G::Window) { G::Device.Reset(); G::Ctx.Reset(); return; }

    RebuildRtv(sc);
    if (!G::Rtv) { G::Device.Reset(); G::Ctx.Reset(); return; }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui_ImplWin32_Init(G::Window);
    ImGui_ImplDX11_Init(G::Device.Get(), G::Ctx.Get());
    G::Ready = true;
    G::Log("setup: ready");
}

static void Render() {
    if (G::Visible) {
        if (!G::Cursor) { while (ShowCursor(TRUE) < 0) {} G::Cursor = true; }
        ClipCursor(nullptr);
    } else if (G::Cursor) {
        while (ShowCursor(FALSE) >= 0) {}
        G::Cursor = false;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    RefreshPlayers();
    DrawESP();
    RunAim();
    RunTrigger();
    RunMisc();

    if (G::Visible) G::Ui.draw();

    ImGui::Render();

    ComPtr<ID3D11RenderTargetView> prev;
    G::Ctx->OMGetRenderTargets(1, prev.GetAddressOf(), nullptr);
    G::Ctx->OMSetRenderTargets(1, G::Rtv.GetAddressOf(), nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    if (prev) G::Ctx->OMSetRenderTargets(1, prev.GetAddressOf(), nullptr);
    else G::Ctx->OMSetRenderTargets(0, nullptr, nullptr);
}

static HRESULT WINAPI hkPresent(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (!G::Ready) Setup(sc);
    if (G::Ready && G::Rtv) Render();
    HRESULT hr = reinterpret_cast<decltype(&hkPresent)>(G::OrigPresent)(sc, sync, flags);
    if (G::Unload) Unload();
    return hr;
}

static HRESULT WINAPI hkPresent1(IDXGISwapChain1* sc, UINT sync, UINT flags, const DXGI_PRESENT_PARAMETERS* p) {
    if (!G::Ready) Setup(sc);
    if (G::Ready && G::Rtv) Render();
    HRESULT hr = reinterpret_cast<decltype(&hkPresent1)>(G::OrigPresent1)(sc, sync, flags, p);
    if (G::Unload) Unload();
    return hr;
}

static LRESULT CALLBACK HookProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    if (!G::OrigWndProc) return DefWindowProcW(h, msg, w, l);

    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
        if (w == VK_INSERT && !(l & (1LL << 30))) {
            G::Visible = !G::Visible;
            G::Ui.setVisible(G::Visible);
            G::Log("insert -> %d", G::Visible ? 1 : 0);
            if (!G::Visible) ClipCursor(nullptr);
            return 0;
        }
        if (w == VK_END && !(l & (1LL << 30))) { G::Unload = true; return 0; }
        if (w == VK_F5 && !(l & (1LL << 30))) { SaveConfig(); return 0; }
        if (w == VK_F9 && !(l & (1LL << 30))) { LoadConfig(); return 0; }
    }

    if (G::Ready) {
        if (getrekt::Win32Input(G::Ui, msg, w, l)) return 0;
        if (ImGui_ImplWin32_WndProcHandler(h, msg, w, l)) return 0;
    }

    if (G::Visible) {
        switch (msg) {
        case WM_MOUSEMOVE: case WM_LBUTTONDOWN: case WM_LBUTTONUP:
        case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_MBUTTONDOWN: case WM_MBUTTONUP:
        case WM_MOUSEWHEEL: case WM_XBUTTONDOWN: case WM_XBUTTONUP:
        case WM_INPUT: case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR:
            return 0;
        default: break;
        }
    }
    return CallWindowProcW(G::OrigWndProc, h, msg, w, l);
}

void InitializeHooks() {
    G::Log("hooks: start");
    for (int i = 0; i < 300 && !G::Window; ++i) {
        G::Window = FindWindowW(L"SDL_app", nullptr);
        if (!G::Window) Sleep(200);
    }
    G::Log("hooks: hwnd %p", static_cast<void*>(G::Window));

    if (G::Window) {
        WNDPROC current = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(G::Window, GWLP_WNDPROC));
        if (current == HookProc) { G::Log("hooks: uz nas"); return; }
        G::OrigWndProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(G::Window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookProc)));
        G::Log("hooks: wndproc %p", reinterpret_cast<void*>(G::OrigWndProc));
    }

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = G::Module;
    wc.lpszClassName = L"Cs2EspDummy";
    RegisterClassExW(&wc);
    HWND dummy = CreateWindowExW(0, wc.lpszClassName, L"d", WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100, nullptr, nullptr, G::Module, nullptr);

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
    G::Log("hooks: dummy hr=0x%08lX", static_cast<unsigned long>(hr));
    if (FAILED(hr)) return;

    void** vtbl = *reinterpret_cast<void***>(sc.Get());
    DWORD old = 0;
    VirtualProtect(vtbl, 48 * sizeof(void*), PAGE_EXECUTE_READWRITE, &old);
    G::OrigPresent = vtbl[8];
    G::OrigResize = vtbl[13];
    G::OrigPresent1 = vtbl[22];
    G::OrigResize1 = vtbl[39];
    vtbl[8] = &hkPresent;
    vtbl[13] = &hkResizeBuffers;
    if (G::OrigPresent1) vtbl[22] = &hkPresent1;
    if (G::OrigResize1) vtbl[39] = &hkResizeBuffers1;
    VirtualProtect(vtbl, 48 * sizeof(void*), old, &old);
    G::Log("hooks: hotovo");

    sc.Reset(); dev.Reset(); ctx.Reset();
    DestroyWindow(dummy);
    UnregisterClassW(wc.lpszClassName, G::Module);
}

void CleanupHooks() {
    if (G::OrigWndProc && G::Window)
        SetWindowLongPtrW(G::Window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(G::OrigWndProc));
}