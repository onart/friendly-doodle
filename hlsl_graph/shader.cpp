#include "shader.h"
#include "d3ddev.h"
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include "imgui.h"
#include "ubo.h"
#include "image.h"
#include "resourcemanager.h"

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

ID3D11ComputeShader* createComputeShader(const void* data, size_t size) {
	ID3D11ComputeShader* computeShader = nullptr;
	ID3DBlob* shaderBlob = nullptr;
	HRESULT hr = D3DCompile(data, size, nullptr, nullptr, nullptr, "main", "cs_5_0", 0, 0, &shaderBlob, nullptr);
	if(FAILED(hr)) {
		return nullptr;
	}
	hr = D3D11Device::getDevice()->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &computeShader);
	shaderBlob->Release();
	if (FAILED(hr)) {
		return nullptr;
	}
	return computeShader;
}

void VertexShader::draw() {
	ImGui::PushID(this);
	ImGui::Text("Vertex Shader: %p", shader);
	int i = 0;
	if (ImGui::BeginTable("ubo_table", 2)) {
		for (auto it = ubos.begin(); it != ubos.end(); ) {
			ImGui::PushID((size_t)it->get() + (i++));
			ImGui::TableNextColumn();
			(*it)->draw();
			ImGui::TableNextColumn();
			if (ImGui::Button("x")) {
				it = ubos.erase(it);
			}
			else {
				++it;
			}
			ImGui::TableNextRow();
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	if (ImGui::Button("+##ubo")) {
		ImGui::OpenPopup("ubo");
		mgr.addUBOUI(true);
	}
	if (ImGui::BeginPopup("ubo")) {
		if (auto ubo = mgr.addUBOUI(false)) {
			ubos.push_back(ubo);
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginTable("sbo_table", 2)) {
		for (auto it = buffers.begin(); it != buffers.end(); ) {
			ImGui::PushID((size_t)it->get() + (i++));
			ImGui::TableNextColumn();
			(*it)->show(256, 256);
			ImGui::TableNextColumn();
			if (ImGui::Button("x")) {
				it = buffers.erase(it);
			}
			else {
				++it;
			}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	if (ImGui::Button("+##sbo")) {
		ImGui::OpenPopup("sbo");
		mgr.addSBOUI(true);
	}
	if (ImGui::BeginPopup("sbo")) {
		if (auto sbo = mgr.addSBOUI(false)) {
			buffers.push_back(sbo);
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::InputTextMultiline("Source Code", (char*)sourceCode.data(), sourceCode.size() - 1, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), 0);
	bool compiled = shader;
	ImGui::Checkbox("compiled", &compiled);
	if (ImGui::Button("Compile")) {
		std::string baseStr;
		int i = 0;
		for (auto& ubo : ubos) {
			baseStr += ubo->toPrimaryCode(i++);
		}
		for (auto& buffer : buffers) {
			baseStr += buffer->toPrimaryCode(i++);
		}
		baseStr += sourceCode.data();
		shader = createVertexShader(baseStr.data(), baseStr.size());
	}
	
	ImGui::PopID();
}