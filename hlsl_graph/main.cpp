#include <Windows.h>
#include "imgui.h"
#include "variant.h"
#include "d3ddev.h"

void update() {

    ID3D11DeviceContext* context = Global::get<ID3D11DeviceContext*>("d3d11/context", nullptr);
    ID3D11Device* device = Global::get<ID3D11Device*>("d3d11/device", nullptr);
    IDXGISwapChain* swapChain = Global::get<IDXGISwapChain*>("d3d11/swapchain", nullptr);
    ID3D11RenderTargetView* target = Global::get<ID3D11RenderTargetView*>("d3d11/target", nullptr);
    
    if (ImGui::Begin(u8"main window")) {
		ImGui::Text("Hello, world!");
    }
    ImGui::End();
}

int main() {
    D3D11Device device;
    if (!device.init(L"ytool")) {
        return 1;
    }

    bool done = false;
    while (1) {
        if (!device.preUpdate()) break;
        update();
        device.present();
    }
    device.finalize();
	return 0;
}