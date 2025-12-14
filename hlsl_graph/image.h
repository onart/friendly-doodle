#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <d3d11.h>
#include <memory>

class Texture2D {
public:
	friend class ResourceManager;

	std::shared_ptr<Texture2D> static create(const char* u8path);

	std::shared_ptr<Texture2D> static create(const void* pixels, size_t rowPitch, UINT width, UINT height, DXGI_FORMAT format) {
		Texture2D tex;
		if (tex.initResource(pixels, rowPitch, width, height, format)) {
			auto ret = std::shared_ptr<Texture2D>(new Texture2D());
			*ret = tex;
			std::memset(&tex, 0, sizeof(Texture2D));
			return ret;
		}
		return {};
	}

	std::shared_ptr<Texture2D> static createTarget(UINT width, UINT height, DXGI_FORMAT format) {
		Texture2D tex;
		if (tex.initTarget(width, height, format)) {
			auto ret = std::shared_ptr<Texture2D>(new Texture2D());
			*ret = tex;
			std::memset(&tex, 0, sizeof(Texture2D));
			return ret;
		}
		return {};
	}

	std::shared_ptr<Texture2D> static createUAV(UINT width, UINT height, DXGI_FORMAT format) {
		Texture2D tex;
		if (tex.initUAV(width, height, format)) {
			auto ret = std::shared_ptr<Texture2D>(new Texture2D());
			*ret = tex;
			std::memset(&tex, 0, sizeof(Texture2D));
			return ret;
		}
		return {};
	}

	void clear();
	void show(int renderW, int renderH);

	~Texture2D() {
		if (texture) texture->Release();
		if (srv) srv->Release();
		if (rtv) rtv->Release();
		if (uav)uav->Release();
	}
private:
	Texture2D() = default;
	bool initResource(const void* pixels, size_t rowPitch, UINT width, UINT height, DXGI_FORMAT format);
	bool initTarget(UINT width, UINT height, DXGI_FORMAT format);
	bool initUAV(UINT width, UINT height, DXGI_FORMAT format);
	ID3D11Texture2D* texture{};
	ID3D11ShaderResourceView* srv{};
	ID3D11RenderTargetView* rtv{};
	ID3D11UnorderedAccessView* uav{};
	int w, h;
	bool shown = 0;
};

#endif // !__IMAGE_H__

