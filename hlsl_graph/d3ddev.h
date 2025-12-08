#ifndef __D3DDEV_H__
#define __D3DDEV_H__

#include <d3d11.h>
#include <Windows.h>
#include <map>

class D3D11Device {
public:
	bool init(const wchar_t* windowName);
	void finalize();
	bool preUpdate();
	void present();
	static ID3D11Device* getDevice() { return singleton->device; }
	static ID3D11DeviceContext* getContext() { return singleton->context; }
private:
	inline static D3D11Device* singleton{};
	HWND hwnd{};
	WNDCLASSEXW wc{};
	ID3D11Device* device{};
	ID3D11DeviceContext* context{};
	IDXGISwapChain* swapChain{};
	std::map<ID3D11Texture2D*, ID3D11RenderTargetView*> swapchainBuffer{};
};

#endif
