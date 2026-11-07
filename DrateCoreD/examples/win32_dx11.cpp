#ifdef _MSC_VER
#include <crtdbg.h>
#endif
#include <getrekt/Win32Input.hpp>
#include <d3d11.h>
#include <wrl/client.h>
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "../assets/preview_assets.h"
using Microsoft::WRL::ComPtr;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
static getrekt::UI ui;
static bool ready=false;
static ComPtr<ID3D11Device> device;
static ComPtr<ID3D11DeviceContext> context;
static ComPtr<IDXGISwapChain> swapchain;
static ComPtr<ID3D11RenderTargetView> target;
static bool MakeTarget(){ComPtr<ID3D11Texture2D> back;return SUCCEEDED(swapchain->GetBuffer(0,IID_PPV_ARGS(&back)))&&SUCCEEDED(device->CreateRenderTargetView(back.Get(),nullptr,&target));}
static ComPtr<ID3D11ShaderResourceView> Texture(const unsigned char* data,unsigned w,unsigned h){
    D3D11_TEXTURE2D_DESC d{};d.Width=w;d.Height=h;d.MipLevels=d.ArraySize=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA pixels{data,w*4,0};ComPtr<ID3D11Texture2D> tex;ComPtr<ID3D11ShaderResourceView> view;
    if(SUCCEEDED(device->CreateTexture2D(&d,&pixels,&tex)))device->CreateShaderResourceView(tex.Get(),nullptr,&view);return view;
}
    static HRESULT Capture(const wchar_t* capturePath) {
        ComPtr<ID3D11Texture2D> source, staging;
        HRESULT hr = swapchain->GetBuffer(0, IID_PPV_ARGS(&source));
        if (FAILED(hr)) return hr;
        D3D11_TEXTURE2D_DESC desc{}; source->GetDesc(&desc);
        desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; desc.MiscFlags = 0;
        hr = device->CreateTexture2D(&desc, nullptr, &staging); if (FAILED(hr)) return hr;
        context->CopyResource(staging.Get(), source.Get());
        std::vector<unsigned char> row(desc.Width * 4);
        D3D11_MAPPED_SUBRESOURCE mapped{};
        hr = context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped); if (FAILED(hr)) return hr;
        FILE* file = nullptr;
        if (_wfopen_s(&file, capturePath, L"wb") || !file) { context->Unmap(staging.Get(), 0); return E_ACCESSDENIED; }
        BITMAPFILEHEADER header{}; header.bfType = 0x4d42;
        header.bfOffBits = sizeof(header) + sizeof(BITMAPINFOHEADER);
        header.bfSize = header.bfOffBits + desc.Width * desc.Height * 4;
        BITMAPINFOHEADER info{}; info.biSize = sizeof(info);
        info.biWidth = static_cast<LONG>(desc.Width); info.biHeight = -static_cast<LONG>(desc.Height);
        info.biPlanes = 1; info.biBitCount = 32; info.biCompression = BI_RGB;
        bool okay = fwrite(&header, sizeof(header), 1, file) == 1 && fwrite(&info, sizeof(info), 1, file) == 1;
        for (UINT y = 0; y < desc.Height && okay; ++y) {
            const auto* pixels = static_cast<const unsigned char*>(mapped.pData) + y * mapped.RowPitch;
            for (UINT x = 0; x < desc.Width; ++x) {
                row[x * 4] = pixels[x * 4 + 2]; row[x * 4 + 1] = pixels[x * 4 + 1];
                row[x * 4 + 2] = pixels[x * 4]; row[x * 4 + 3] = 255;
            }
            okay = fwrite(row.data(), row.size(), 1, file) == 1;
        }
        if (fclose(file)) okay = false;
        context->Unmap(staging.Get(), 0);
        return okay ? S_OK : E_FAIL;
    }

static LRESULT CALLBACK WndProc(HWND window,UINT msg,WPARAM w,LPARAM l){
    if(ready){
        if(getrekt::Win32Input(ui,msg,w,l))return 1;
        if(msg==WM_KEYDOWN&&w==VK_INSERT&&!(l&(1LL<<30))){ui.setVisible(!ui.visible());return 0;}
        if(ImGui_ImplWin32_WndProcHandler(window,msg,w,l))return 1;
    }
    if(msg==WM_SIZE&&swapchain&&w!=SIZE_MINIMIZED){
        context->OMSetRenderTargets(0,nullptr,nullptr);target.Reset();
        if(FAILED(swapchain->ResizeBuffers(0,LOWORD(l),HIWORD(l),DXGI_FORMAT_UNKNOWN,0))||!MakeTarget())PostQuitMessage(1);
        return 0;
    }
    if(msg==WM_DESTROY){PostQuitMessage(0);return 0;}
    return DefWindowProcW(window,msg,w,l);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR command,int show){
#ifdef _MSC_VER
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
#endif

    const bool capture=wcsstr(command,L"--capture")!=nullptr;
    const bool smoke=capture||wcsstr(command,L"--smoke")!=nullptr;
    if(capture)std::filesystem::create_directories("qa");
    WNDCLASSW wc{};wc.lpfnWndProc=WndProc;wc.hInstance=instance;wc.lpszClassName=L"GetRektUIExample";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    if(!RegisterClassW(&wc))return 1;
    HWND window=CreateWindowW(wc.lpszClassName,L"GetRekt ImGui Kit - standalone UI",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1100,760,nullptr,nullptr,instance,nullptr);
    if(!window)return 2;
    DXGI_SWAP_CHAIN_DESC desc{};desc.BufferCount=2;desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.OutputWindow=window;desc.SampleDesc.Count=1;desc.Windowed=TRUE;desc.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
    auto init=[&](D3D_DRIVER_TYPE driver){return D3D11CreateDeviceAndSwapChain(nullptr,driver,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&desc,&swapchain,&device,nullptr,&context);};
    HRESULT hr=init(smoke?D3D_DRIVER_TYPE_WARP:D3D_DRIVER_TYPE_HARDWARE);
    if(FAILED(hr)){swapchain.Reset();device.Reset();context.Reset();hr=init(D3D_DRIVER_TYPE_WARP);}
    if(FAILED(hr)||!MakeTarget())return 3;
    IMGUI_CHECKVERSION();ImGui::CreateContext();
    if(!ImGui_ImplWin32_Init(window)||!ImGui_ImplDX11_Init(device.Get(),context.Get()))return 4;
    auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
    wchar_t winDir[MAX_PATH]{};GetWindowsDirectoryW(winDir,MAX_PATH);
    const auto font=std::filesystem::path(winDir)/L"Fonts"/L"tahoma.ttf";
    if(std::filesystem::exists(font))io.Fonts->AddFontFromFileTTF(font.string().c_str(),13.f);
    using namespace reference_assets;
    auto playerImage=Texture(player,player_width,player_height),knifeImage=Texture(knife,knife_width,knife_height),gloveImage=Texture(glove,glove_width,glove_height);
    ui.setPreviewTextures((ImTextureID)(intptr_t)playerImage.Get(),(ImTextureID)(intptr_t)knifeImage.Get(),(ImTextureID)(intptr_t)gloveImage.Get());
    ready=true;if(!smoke)ShowWindow(window,show);
    MSG msg{};int frames=0;
    while(msg.message!=WM_QUIT){
        if(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);continue;}
        if(!target)break;
        ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();
        if(capture){
            int shot=frames/4;ui.state.page=shot<6?shot:shot==6?1:shot==7?2:5;
            ui.state.legit=shot==6?1:0;ui.state.visuals=shot==7?1:0;
            if(shot==7)ui.state.colors.Light();else ui.state.colors.Dark();
            ui.state.clubWindow=shot==8;ui.state.spectators=shot==9||shot==10;
            ui.state.spectatorRows=shot==10?std::vector<getrekt::VisualReplica::Spectator>{{"Demo 1",false,0},{"Demo 2",true,0}}:std::vector<getrekt::VisualReplica::Spectator>{};
        }
        ui.draw();
        const float clear[]{.025f,.03f,.04f,1};context->OMSetRenderTargets(1,target.GetAddressOf(),nullptr);context->ClearRenderTargetView(target.Get(),clear);
        ImGui::Render();ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        if(capture&&frames%4==3){auto path=std::filesystem::path("qa")/("page-"+std::to_string(frames/4)+".bmp");if(FAILED(Capture(path.c_str())))return 6;}
        if(FAILED(swapchain->Present(smoke?0:1,0)))return 5;
        if(smoke&&++frames==(capture?44:8))PostQuitMessage(0);
    }
    ready=false;ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();context->ClearState();DestroyWindow(window);UnregisterClassW(wc.lpszClassName,instance);return (int)msg.wParam;
}
