#ifndef __D3DDEV_H__
#define __D3DDEV_H__

#include <d3d11.h>
#include <Windows.h>
#include <map>
#include <vector>
#include <filesystem>

class D3D11Device {
	friend LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
	bool init(const wchar_t* windowName);
	void finalize();
	bool preUpdate();
	void present();
	static ID3D11Device* getDevice() { return singleton->device; }
	static ID3D11DeviceContext* getContext() { return singleton->context; }
	static const auto& getRecentDroppedPaths() { return singleton->dropped; }
	static void clearRecentDroppedPaths() { singleton->dropped.clear(); }
	static int64_t getFrameSN() { return singleton->frameSN++; }
	static bool isNewDropInCurrentFrame() { return singleton->droppedFrameSN == singleton->frameSN; }

private:
	inline static D3D11Device* singleton{};
	HWND hwnd{};
	WNDCLASSEXW wc{};
	ID3D11Device* device{};
	ID3D11DeviceContext* context{};
	IDXGISwapChain* swapChain{};
	std::map<ID3D11Texture2D*, ID3D11RenderTargetView*> swapchainBuffer{};
	std::vector<std::filesystem::path> dropped;
	int64_t droppedFrameSN = 0;
	int64_t frameSN = 1;
};

#endif
