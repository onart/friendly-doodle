#include <Windows.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "variant.h"
#pragma comment(lib, "d3d11.lib")
#include <d3d11.h>

static UINT resizeWidth = 0;
static UINT resizeHeight = 0;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_DROPFILES:
        return 0;
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        resizeWidth = (UINT)LOWORD(lParam); // Queue resize
        resizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            //const int dpi = HIWORD(wParam);
            //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

static WNDCLASSEXW wc;
static bool createWindow() {
    wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ytool", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Tool", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd) return false;
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);
    Global::set("window/handle", hwnd);
    return true;
}

static void destroyWindow() {
    HWND hwnd = Global::get<HWND>("window/handle", NULL);
    if (hwnd) {
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        Global::reset("window/handle");
    }
}

static bool initD3D() {
    const D3D_FEATURE_LEVEL lev[]{ D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL foundLevel{};
    ID3D11Device* device{};
    ID3D11DeviceContext* context{};
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, lev, sizeof(lev) / sizeof(lev[0]), D3D11_SDK_VERSION, &device, &foundLevel, &context);
    if(hr == S_OK) {
        Global::set("d3d11/device", device);
        Global::set("d3d11/context", context);
	}
    else {
        return false;
    }
    return true;
}

static bool initWSI() {
    HWND hw = Global::get<HWND>("window/handle", NULL);
    if (hw == NULL) { return false; }
	ID3D11Device* device = Global::get<ID3D11Device*>("d3d11/device", nullptr);
	if (device == nullptr) { return false; }
	IDXGIDevice* dxgiDevice = nullptr;
    device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if(dxgiDevice){
        IDXGIAdapter* adapter = nullptr;
        if (dxgiDevice->GetAdapter(&adapter) == S_OK) {
            IDXGIFactory* factory = nullptr;
            if (adapter->GetParent(__uuidof(IDXGIFactory), (void**)&factory) == S_OK) {
                DXGI_SWAP_CHAIN_DESC sd;
                ZeroMemory(&sd, sizeof(sd));
                sd.BufferCount = 1;
                sd.BufferDesc.Width = 0;
                sd.BufferDesc.Height = 0;
                sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                sd.BufferDesc.RefreshRate.Numerator = 60;
                sd.BufferDesc.RefreshRate.Denominator = 1;
                sd.Flags = 0;
                sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                sd.OutputWindow = hw;
                sd.SampleDesc.Count = 1;
                sd.SampleDesc.Quality = 0;
                sd.Windowed = TRUE;
                sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                IDXGISwapChain* swapChain = nullptr;
                if (factory->CreateSwapChain(device, &sd, &swapChain) == S_OK) {
                    Global::set("d3d11/swapchain", swapChain);
                }
                factory->Release();
            }
            adapter->Release();
        }
		dxgiDevice->Release();
    }

	return true;
}

static void destroyD3D() {
    IDXGISwapChain* swapChain = Global::get<IDXGISwapChain*>("d3d11/swapchain", nullptr);
    if (swapChain) {
        swapChain->Release();
    }
    ID3D11DeviceContext* context = Global::get<ID3D11DeviceContext*>("d3d11/context", nullptr);
    if (context) {
        context->Flush();
        context->ClearState();
        context->Release();
    }
    ID3D11Device* device = Global::get<ID3D11Device*>("d3d11/device", nullptr);
    if (device) {
        device->Release();
    }
    Global::reset("d3d11");
}

static void initImGUI() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    //io.ConfigViewportsNoDefaultParent = true;
    //io.ConfigDockingAlwaysTabBar = true;
    //io.ConfigDockingTransparentPayload = true;
    //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: Experimental. THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
    //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI: Experimental.

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(Global::get<HWND>("window/handle", NULL));
    ImGui_ImplDX11_Init(Global::get<ID3D11Device*>("d3d11/device", nullptr), Global::get<ID3D11DeviceContext*>("d3d11/context", nullptr));

    ImFontGlyphRangesBuilder b;
    auto range = ImFontAtlas().GetGlyphRangesKorean();
    auto font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\malgunbd.ttf", 18.0f, 0, range);
    ImGui::GetIO().FontDefault = font;
}

static void destroyImGUI() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

bool update() {
    MSG msg;
    bool done = false;
    while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
            done = true;
    }
    if (done) return false;

    ID3D11DeviceContext* context = Global::get<ID3D11DeviceContext*>("d3d11/context", nullptr);
    ID3D11Device* device = Global::get<ID3D11Device*>("d3d11/device", nullptr);
    IDXGISwapChain* swapChain = Global::get<IDXGISwapChain*>("d3d11/swapchain", nullptr);
    ID3D11RenderTargetView* target = Global::get<ID3D11RenderTargetView*>("d3d11/target", nullptr);
    if (resizeWidth != 0 && resizeHeight != 0)
    {
        if (target) {
            target->Release();
            target = {};
        }
        swapChain->ResizeBuffers(0, resizeWidth, resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
        resizeWidth = resizeHeight = 0;
    }
    if (!target) {
        ID3D11Texture2D* t2d{};
        swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&t2d);
        if (t2d) {
            device->CreateRenderTargetView(t2d, nullptr, &target);
            t2d->Release();
        }
        Global::set("d3d11/target", target);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    if (ImGui::Begin(u8"main window")) {
		ImGui::Text("Hello, world!");
    }
    ImGui::End();

    ImGui::Render();

    if (target) {
        ImVec4 clear = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        context->OMSetRenderTargets(1, &target, nullptr);
        context->ClearRenderTargetView(target, &clear.x);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
    swapChain->Present(1, 0);

    // Update and Render additional Platform Windows
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
    return true;
}

int main() {
    if (!createWindow()) {
        return 1;
    }
    if(!initD3D()) {
        destroyWindow();
        return 1;
	}
    if(!initWSI()) {
        destroyD3D();
        destroyWindow();
        return 1;
	}
    initImGUI();

    bool done = false;
    while (update());

    destroyImGUI();
    destroyD3D();
    destroyWindow();
	return 0;
}