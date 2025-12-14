#include "d3ddev.h"
#pragma comment(lib, "d3d11.lib")
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

template<class T>
inline static void releaseCom(T*& com) {
    if (com) {
        com->Release();
        com = {};
    }
}


static UINT resizeWidth = 0;
static UINT resizeHeight = 0;
static wchar_t path[4096];

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_DROPFILES:
    {
        uint32_t count = DragQueryFileW((HDROP)wParam, 0xffffffff, nullptr, 0);
        D3D11Device::singleton->dropped.clear();
        for (uint32_t i = 0; i < count; ++i) {
            int reqSize = DragQueryFileW((HDROP)wParam, i, NULL, 0);
            if (reqSize + 1 < sizeof(path) / sizeof(path[0])) {
                if (DragQueryFileW((HDROP)wParam, i, path, reqSize + 1)) {
                    D3D11Device::singleton->dropped.emplace_back(path);
                }
            }
        }
		D3D11Device::singleton->droppedFrameSN = D3D11Device::singleton->frameSN;
        return 0;
    }
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

bool D3D11Device::init(const wchar_t* name) {
    if (singleton) {
        return singleton == this;
    }

    wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ytool", nullptr };
    ::RegisterClassExW(&wc);
    hwnd = ::CreateWindowW(wc.lpszClassName, name, WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd) return false;
    DragAcceptFiles(hwnd, TRUE);
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    const D3D_FEATURE_LEVEL lev[]{ D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL foundLevel{};
#ifdef _DEBUG
    UINT flags = D3D11_CREATE_DEVICE_DEBUG;
#else
    UINT flags = 0;
#endif
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, lev, sizeof(lev) / sizeof(lev[0]), D3D11_SDK_VERSION, &device, &foundLevel, &context);
    if (FAILED(hr)) {
        return false;
	}
    if (hwnd) {
        IDXGIDevice* dxgiDevice = nullptr;
        device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
        if (dxgiDevice) {
			dxgiDevice->Release();
            IDXGIAdapter* adapter = nullptr;
            if (dxgiDevice->GetAdapter(&adapter) == S_OK) {
				adapter->Release();
                IDXGIFactory* factory = nullptr;
                if (adapter->GetParent(__uuidof(IDXGIFactory), (void**)&factory) == S_OK) {
					factory->Release();
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
                    sd.OutputWindow = hwnd;
                    sd.SampleDesc.Count = 1;
                    sd.SampleDesc.Quality = 0;
                    sd.Windowed = TRUE;
                    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                    HRESULT hr = factory->CreateSwapChain(device, &sd, &swapChain);
                    if (FAILED(hr)) {
                        return false;
                    }
                }
            }
        }
    }

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
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device, context);

    ImFontGlyphRangesBuilder b;
    auto range = ImFontAtlas().GetGlyphRangesKorean();
    auto font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\malgunbd.ttf", 18.0f, 0, range);
    ImGui::GetIO().FontDefault = font;
    singleton = this;
    return true;
}

void D3D11Device::finalize() {
    context->ClearState();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

	releaseCom(swapChain);
	releaseCom(context);
	releaseCom(device);

    if (hwnd) {
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    }
    singleton = nullptr;
}

bool D3D11Device::preUpdate() {
    frameSN++;
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

    if (resizeWidth != 0 && resizeHeight != 0)
    {
        for(auto& pair : swapchainBuffer) {
			ID3D11Texture2D* tex = pair.first;
            pair.second->Release();
		}
        swapchainBuffer.clear();
        context->Flush();
        context->ClearState();
        auto hr = swapChain->ResizeBuffers(0, resizeWidth, resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
        resizeWidth = resizeHeight = 0;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    return true;
}

void D3D11Device::present() {
    ImGui::Render();

    ID3D11Texture2D* t2d{};
    ID3D11RenderTargetView* target{};
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&t2d);
    if (t2d) {
        target = swapchainBuffer[t2d];
        if (!target) {
            device->CreateRenderTargetView(t2d, nullptr, &target);
            swapchainBuffer[t2d] = target;
        }
        t2d->Release();
    }

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
}