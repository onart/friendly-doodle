#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <d3d11.h>
#include <memory>
#include <string>

class ShaderBufferObject {
public:
	friend class ResourceManager;

	std::shared_ptr<ShaderBufferObject> static create(const char* u8path);

	std::shared_ptr<ShaderBufferObject> static create(const void* pixels, size_t rowPitch, UINT width, UINT height, DXGI_FORMAT format) {
		ShaderBufferObject tex;
		if (tex.initResource(pixels, rowPitch, width, height, format)) {
			auto ret = std::shared_ptr<ShaderBufferObject>(new ShaderBufferObject());
			*ret = tex;
			std::memset(&tex, 0, sizeof(ShaderBufferObject));
			return ret;
		}
		return {};
	}

	std::shared_ptr<ShaderBufferObject> static createTarget(UINT width, UINT height, DXGI_FORMAT format) {
		ShaderBufferObject tex;
		if (tex.initTarget(width, height, format)) {
			auto ret = std::shared_ptr<ShaderBufferObject>(new ShaderBufferObject());
			*ret = tex;
			std::memset(&tex, 0, sizeof(ShaderBufferObject));
			return ret;
		}
		return {};
	}

	std::shared_ptr<ShaderBufferObject> static createUAV(UINT width, UINT height, DXGI_FORMAT format) {
		ShaderBufferObject tex;
		if (tex.initUAV(width, height, format)) {
			auto ret = std::shared_ptr<ShaderBufferObject>(new ShaderBufferObject());
			*ret = tex;
			std::memset(&tex, 0, sizeof(ShaderBufferObject));
			return ret;
		}
		return {};
	}

	std::shared_ptr<ShaderBufferObject> static createStructuredUAV(UINT size, UINT stride) {
		ShaderBufferObject tex;
		if (tex.initBufferUAV(size, stride)) {
			auto ret = std::shared_ptr<ShaderBufferObject>(new ShaderBufferObject());
			*ret = tex;
			std::memset(&tex, 0, sizeof(ShaderBufferObject));
			return ret;
		}
		return {};
	}

	void clear();
	void show(int renderW, int renderH);

	~ShaderBufferObject() {
		if (buffer) buffer->Release();
		if (texture) texture->Release();
		if (srv) srv->Release();
		if (rtv) rtv->Release();
		if (uav) uav->Release();
	}

	std::string toPrimaryCode(size_t binding);
	inline ID3D11Buffer* getBuffer() { return buffer; }
	inline ID3D11Texture2D* getTexture() { return texture; }
	inline ID3D11ShaderResourceView* getSRV() { return srv; }
	inline ID3D11RenderTargetView* getRTV() { return rtv; }
	inline ID3D11UnorderedAccessView* getUAV() { return uav; }
private:
	ShaderBufferObject() = default;
	bool initResource(const void* pixels, size_t rowPitch, UINT width, UINT height, DXGI_FORMAT format);
	bool initTarget(UINT width, UINT height, DXGI_FORMAT format);
	bool initUAV(UINT width, UINT height, DXGI_FORMAT format);
	bool initBufferUAV(UINT size, UINT stride);
	ID3D11Buffer* buffer{};
	ID3D11Texture2D* texture{};
	ID3D11ShaderResourceView* srv{};
	ID3D11RenderTargetView* rtv{};
	ID3D11UnorderedAccessView* uav{};
	int w, h;
	bool shown = 0;
};

#endif // !__IMAGE_H__

