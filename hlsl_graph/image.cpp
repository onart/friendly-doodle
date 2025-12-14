#define _CRT_SECURE_NO_WARNINGS
#include "image.h"
#include "d3ddev.h"
#include <d3d11_1.h>
#include "imgui.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cstdio>
#include <sstream>

bool ShaderBufferObject::initResource(const void* pixels, size_t rowPitch, UINT width, UINT height, DXGI_FORMAT format) {
	if (texture) { return false; }
	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = format;
	desc.Width = width;
	desc.Height = height;
	desc.CPUAccessFlags = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.ArraySize = 1;
	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = pixels;
	w = width;
	h = height;
	
	initData.SysMemPitch = rowPitch;
	HRESULT hr = D3D11Device::getDevice()->CreateTexture2D(&desc, &initData, &texture);
	if(FAILED(hr)) {
		return false;
	}
	hr = D3D11Device::getDevice()->CreateShaderResourceView(texture, nullptr, &srv);
	if (FAILED(hr)) {
		return false;
	}
	w = width;
	h = height;
	return true;
}

bool ShaderBufferObject::initTarget(UINT width, UINT height, DXGI_FORMAT format) {
	if (texture) { return false; }
	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = format;
	desc.Width = width;
	desc.Height = height;
	desc.CPUAccessFlags = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.ArraySize = 1;

	HRESULT hr = D3D11Device::getDevice()->CreateTexture2D(&desc, nullptr, &texture);
	if (FAILED(hr)) {
		return false;
	}
	hr = D3D11Device::getDevice()->CreateShaderResourceView(texture, nullptr, &srv);
	if (FAILED(hr)) {
		return false;
	}
	hr = D3D11Device::getDevice()->CreateRenderTargetView(texture, nullptr, &rtv);
	if (FAILED(hr)) {
		return false;
	}
	w = width;
	h = height;
	return true;
}

bool ShaderBufferObject::initUAV(UINT width, UINT height, DXGI_FORMAT format) {
	if (texture) { return false; }
	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = format;
	desc.Width = width;
	desc.Height = height;
	desc.CPUAccessFlags = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.ArraySize = 1;

	HRESULT hr = D3D11Device::getDevice()->CreateTexture2D(&desc, nullptr, &texture);
	if (FAILED(hr)) {
		return false;
	}
	hr = D3D11Device::getDevice()->CreateShaderResourceView(texture, nullptr, &srv);
	if (FAILED(hr)) {
		return false;
	}
	hr = D3D11Device::getDevice()->CreateUnorderedAccessView(texture, nullptr, &uav);
	if (FAILED(hr)) {
		return false;
	}
	w = width;
	h = height;
	return true;
}

void ShaderBufferObject::clear() {
	if (!texture) { return; }
	if (!rtv && !uav) return;
	float clearColor[4] = { 0, 0, 0, 0 };
	ID3D11DeviceContext* ctx = D3D11Device::getContext();
	ID3D11DeviceContext1* ctx1 = nullptr;
	ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&ctx1);
	if (ctx1) {
		if (rtv) ctx1->ClearView(rtv, clearColor, nullptr, 0);
		else if (uav) ctx1->ClearView(uav, clearColor, nullptr, 0);
	}
}

void ShaderBufferObject::show(int renderW, int renderH) {
	if (srv) {
		if (buffer) { ImGui::Text("UAV Structured Buffer"); }
		else if (rtv) { ImGui::Text("Render Target"); }
		else if (uav) { ImGui::Text("UAV Texture"); }
		else { ImGui::Text("Resource"); }
		ImGui::SameLine();
		if (!buffer) {
			ImGui::Text("%p [%d x %d]", this, w, h);
			ImGui::Checkbox("show", &shown);
			if (shown) {
				ImGui::Image((ImTextureID)srv, ImVec2(renderW, renderH));
			}
		}
		else {
			ImGui::Text("%p [%d bytes]", this, w);
		}
	}
}

std::shared_ptr<ShaderBufferObject> ShaderBufferObject::create(const char* u8path) {
	auto path = std::filesystem::u8path(u8path);
	std::vector<uint8_t> fileData;
	{
		FILE* fp = fopen(path.string().c_str(), "rb");
		if (!fp) return {};
		size_t size = std::filesystem::file_size(path);
		fileData.resize(size);
		std::fread(fileData.data(), 1, size, fp);
		std::fclose(fp);
	}
	int w, h, ch;
	auto pix = stbi_load_from_memory(fileData.data(), fileData.size(), &w, &h, &ch, 4);
	if (!pix) return {};
	auto tex = ShaderBufferObject::create(pix, static_cast<size_t>(w) * 4, static_cast<UINT>(w), static_cast<UINT>(h), DXGI_FORMAT_R8G8B8A8_UNORM);
	stbi_image_free(pix);
	return tex;
}

bool ShaderBufferObject::initBufferUAV(UINT size, UINT stride) {
	if (texture || buffer) return false;
	D3D11_BUFFER_DESC desc{};
	desc.ByteWidth = size;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.CPUAccessFlags = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.StructureByteStride = stride;
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	HRESULT hr = D3D11Device::getDevice()->CreateBuffer(&desc, nullptr, &buffer);
	if (FAILED(hr)) {
		return false;
	}
	hr = D3D11Device::getDevice()->CreateShaderResourceView(buffer, nullptr, &srv);
	if (FAILED(hr)) {
		return false;
	}
	hr = D3D11Device::getDevice()->CreateUnorderedAccessView(buffer, nullptr, &uav);
	if (FAILED(hr)) {
		return false;
	}
	w = size;
	return true;
}

std::string ShaderBufferObject::toPrimaryCode(size_t binding) {
	std::stringstream ss;
	if (buffer) {
		ss << "StructuredBuffer<float4> _t" << binding << " :register(t" << binding << ");\n";
	}
	else {
		ss << "Texture2D _t" << binding << " :register(t" << binding << ");\n";
	}
	return ss.str();
}