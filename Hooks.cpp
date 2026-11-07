#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <thread>
#include <atomic>
#include <cstdio>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "Input.hpp"
#include "Global.h"
#include "ESP.h"
#include "LegitBot.h"
#include "Misc.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

typedef HRESULT(WINAPI* Present_t)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT(WINAPI* Present1_t)(IDXGISwapChain1*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*);
typedef HRESULT(WINAPI* ResizeBuffers_t)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
typedef HRESULT(WINAPI* ResizeBuffers1_t)(IDXGISwapChain3*, UINT, UINT, UINT, DXGI_FORMAT, UINT, const UINT*, const IUnknown* const*);

static Present_t        oPresent = nullptr;
static Present1_t       oPresent1 = nullptr;
static ResizeBuffers_t  oResizeBuffers = nullptr;
static ResizeBuffers1_t oResizeBuffers1 = nullptr;

static ID3D11Device* g_device = nullptr;
static ID3D11DeviceContext* g_context = nullptr;
static IDXGISwapChain* g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static bool                    g_imguiReady = false;
static bool                    g_hooked = false;

static void CreateRTV()
{
    if (!g_swap || !g_device || g_rtv) return;
    ID3D11Texture2D* back = nullptr;
    if (SUCCEEDED(g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back)) && back)
    {
        g_device->CreateRenderTargetView(back, nullptr, &g_rtv);
        back->Release();
    }
}

static void ReleaseRTV()
{
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
}

static void Setup(IDXGISwapChain* sc)
{
    if (g_imguiReady) return;
    inp::Log("setup: start");

    g_swap = sc;
    if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), (void**)&g_device)) || !g_device)
    {
        inp::Log("setup: GetDevice FAIL");
        return;
    }
    g_device->GetImmediateContext(&g_context);
    CreateRTV();

    inp::Init(sc);
    inp::g_gui = &G::Ui;

    if (!inp::GameHwnd())
    {
        inp::Log("setup: hwnd chybi, zkusim priste");
        ReleaseRTV();
        if (g_context) { g_context->Release(); g_context = nullptr; }
        if (g_device) { g_device->Release();  g_device = nullptr; }
        return;
    }

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplWin32_Init(inp::GameHwnd());
    ImGui_ImplDX11_Init(g_device, g_context);

    g_imguiReady = true;
    inp::Log("setup: hotovo");
}

static void Render()
{
    if (!g_imguiReady) return;

    inp::Tick();
    inp::Feed();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawESP();
    RunAim();
    RunTrigger();
    RunMisc();

    G::Ui.setVisible(inp::MenuOpen());
    if (inp::MenuOpen())
        G::Ui.draw();

    ImGui::Render();

    ID3D11RenderTargetView* prev = nullptr;
    g_context->OMGetRenderTargets(1, &prev, nullptr);
    g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_context->OMSetRenderTargets(1, &prev, nullptr);
    if (prev) prev->Release();
}

static HRESULT WINAPI hkPresent(IDXGISwapChain* sc, UINT sync, UINT flags)
{
    __try
    {
        if (!g_imguiReady) Setup(sc);
        if (g_imguiReady) Render();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { inp::Log("present: SEH"); }

    return oPresent(sc, sync, flags);
}

static HRESULT WINAPI hkPresent1(IDXGISwapChain1* sc, UINT sync, UINT flags, const DXGI_PRESENT_PARAMETERS* pp)
{
    __try
    {
        if (!g_imguiReady) Setup((IDXGISwapChain*)sc);
        if (g_imguiReady) Render();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { inp::Log("present1: SEH"); }

    return oPresent1(sc, sync, flags, pp);
}

static HRESULT WINAPI hkResizeBuffers(IDXGISwapChain* sc, UINT n, UINT w, UINT h, DXGI_FORMAT f, UINT fl)
{
    ReleaseRTV();
    HRESULT hr = oResizeBuffers(sc, n, w, h, f, fl);
    if (g_imguiReady) CreateRTV();
    return hr;
}

static HRESULT WINAPI hkResizeBuffers1(IDXGISwapChain3* sc, UINT n, UINT w, UINT h, DXGI_FORMAT f, UINT fl,
    const UINT* nodeMask, const IUnknown* const* presentQueue)
{
    ReleaseRTV();
    HRESULT hr = oResizeBuffers1(sc, n, w, h, f, fl, nodeMask, presentQueue);
    if (g_imguiReady) CreateRTV();
    return hr;
}

static void RemoveHooks()
{
    if (!g_hooked || !oPresent || !g_swap) return;

    void** vt = *(void***)g_swap;
    if (vt)
    {
        DWORD old{};
        VirtualProtect(vt, 0x100 * sizeof(void*), PAGE_EXECUTE_READWRITE, &old);
        vt[8] = oPresent;
        vt[13] = oResizeBuffers;
        vt[22] = oPresent1;
        if (oResizeBuffers1) vt[39] = oResizeBuffers1;
        VirtualProtect(vt, 0x100 * sizeof(void*), old, &old);
    }
    g_hooked = false;
}

static void Shutdown()
{
    if (g_imguiReady)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_imguiReady = false;
    }
    inp::g_gui = nullptr;
    ReleaseRTV();
    if (g_context) { g_context->Release(); g_context = nullptr; }
    if (g_device) { g_device->Release();  g_device = nullptr; }
}

void InitializeHooks()
{
    if (g_hooked) return;

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"cs2esp_dummy";
    RegisterClassExW(&wc);

    HWND dummy = CreateWindowExW(0, wc.lpszClassName, L"x", WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);
    if (!dummy) { inp::Log("hooks: dummy okno FAIL"); return; }

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = dummy;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* sc = nullptr;
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    D3D_FEATURE_LEVEL fl{};

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &sc, &dev, &fl, &ctx);

    if (FAILED(hr) || !sc)
    {
        inp::Logf("hooks: dummy swapchain FAIL 0x%08lX", (unsigned long)hr);
        DestroyWindow(dummy);
        return;
    }

    void** vt = *(void***)sc;
    oPresent = (Present_t)vt[8];
    oResizeBuffers = (ResizeBuffers_t)vt[13];
    oPresent1 = (Present1_t)vt[22];
    oResizeBuffers1 = (ResizeBuffers1_t)vt[39];

    DWORD old{};
    VirtualProtect(vt, 0x100 * sizeof(void*), PAGE_EXECUTE_READWRITE, &old);
    vt[8] = &hkPresent;
    vt[13] = &hkResizeBuffers;
    if (oPresent1) vt[22] = &hkPresent1;
    if (oResizeBuffers1) vt[39] = &hkResizeBuffers1;
    VirtualProtect(vt, 0x100 * sizeof(void*), old, &old);

    g_hooked = true;
    inp::Log("hooks: vtable OK");

    sc->Release();
    dev->Release();
    ctx->Release();
    DestroyWindow(dummy);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    inp::SetUnloadCallback([] { RemoveHooks(); Shutdown(); });
}

void RemoveHooksPublic() { RemoveHooks(); }