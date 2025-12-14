#include "shader.h"
#include "d3ddev.h"
#include <d3dcompiler.h>

ID3D11VertexShader* createVertexShader(const void* data, size_t size) {
	ID3D11VertexShader* vertexShader = nullptr;
	ID3DBlob* shaderBlob = nullptr;
	HRESULT hr = D3DCompile(data, size, nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &shaderBlob, nullptr);
	if(FAILED(hr)) {
		return nullptr;
	}
	hr = D3D11Device::getDevice()->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &vertexShader);
	shaderBlob->Release();
	if (FAILED(hr)) {
		return nullptr;
	}
	return vertexShader;
}

ID3D11PixelShader* createPixelShader(const void* data, size_t size) {
	ID3D11PixelShader* pixelShader = nullptr;
	ID3DBlob* shaderBlob = nullptr;
	HRESULT hr = D3DCompile(data, size, nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &shaderBlob, nullptr);
	if (FAILED(hr)) {
		return nullptr;
	}
	hr = D3D11Device::getDevice()->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &pixelShader);
	shaderBlob->Release();
	if (FAILED(hr)) {
		return nullptr;
	}
	return pixelShader;
}