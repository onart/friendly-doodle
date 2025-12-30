#include "shader.h"
#include "d3ddev.h"
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include "imgui.h"
#include "ubo.h"
#include "image.h"
#include "resourcemanager.h"
#include "logger.h"

static std::string lastError;

ID3D11VertexShader* createVertexShader(const void* data, size_t size) {
	ID3D11VertexShader* vertexShader = nullptr;
	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errMessage = nullptr;
	HRESULT hr = D3DCompile(data, size, nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &shaderBlob, &errMessage);
	if(FAILED(hr)) {
		if (errMessage) {
			lastError = (char*)errMessage->GetBufferPointer();
			errMessage->Release();
		}
		return nullptr;
	}
	if (errMessage) {
		errMessage->Release();
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
	ID3DBlob* errMessage = nullptr;
	HRESULT hr = D3DCompile(data, size, nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &shaderBlob, &errMessage);
	if (FAILED(hr)) {
		if (errMessage) {
			lastError = (char*)errMessage->GetBufferPointer();
			errMessage->Release();
		}
		return nullptr;
	}
	if (errMessage) {
		errMessage->Release();
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
	ID3DBlob* errMessage = nullptr;
	HRESULT hr = D3DCompile(data, size, nullptr, nullptr, nullptr, "main", "cs_5_0", 0, 0, &shaderBlob, &errMessage);
	if(FAILED(hr)) {
		if (errMessage) {
			lastError = (char*)errMessage->GetBufferPointer();
			errMessage->Release();
		}
		return nullptr;
	}
	if (errMessage) {
		errMessage->Release();
	}
	hr = D3D11Device::getDevice()->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &computeShader);
	shaderBlob->Release();
	if (FAILED(hr)) {
		return nullptr;
	}
	return computeShader;
}

size_t Shader::getBinSize() {
	return 12 + sizeof(uint32_t) * ubos.size() + sizeof(sourceCode.size()) + sourceCode.size();
}

bool Shader::serialize(stream& s) {
	s.writes(texCount, sboCount, (uint32_t)ubos.size());
	for (uint32_t ub : ubos) {
		s.write(ub);
	}
	s.write(sourceCode.size());
	s.writeRaw(sourceCode.data(), sourceCode.size());
	return !s.hadFault();
}

bool Shader::commonDeserialize(stream& s) {
	auto [tex, sbo, ubo] = s.reads<uint32_t, uint32_t, uint32_t>();
	texCount = tex;
	sboCount = sbo;
	for (uint32_t i = 0; i < ubo; i++) {
		ubos.push_back(s.read<uint32_t>());
		if (s.hadFault()) return false;
	}
	size_t sourceSize = s.read<size_t>();
	sourceCode.resize(sourceSize);
	s.readRaw(sourceCode.data(), sourceSize);
	return !s.hadFault();
}

void Shader::drawShaderResourceUI(bool compiled) {
	int i = 0;
	ImGui::Text("Uniform buffers");
	if (ImGui::BeginTable("ubo_table", 2)) {
		uint32_t id = 0;
		for (auto it = ubos.begin(); it != ubos.end(); ) {
			uint32_t len = *it;
			ImGui::PushID(id);
			ImGui::TableNextColumn();
			ImGui::Text("%d: %dx4 ubo", id, len);
			ImGui::TableNextColumn();
			if (ImGui::Button("x")) {
				it = ubos.erase(it);
			}
			else {
				++it;
			}
			ImGui::TableNextRow();
			id++;
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	if (ImGui::Button("+##ubo")) {
		ImGui::OpenPopup("ubo");
	}
	if (ImGui::BeginPopup("ubo")) {
		static int f4count = 1;
		ImGui::DragInt("float4 count", &f4count, 1, 1, 256);
		if (ImGui::Button("confirm##ubo")) {
			if (f4count > 0) {
				ubos.push_back(f4count);
			}
			f4count = 1;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	int sboCount = this->sboCount;
	if (ImGui::InputInt("structured buffer<float4> count", &sboCount, 1)) {
		this->sboCount = sboCount;
	}

	int texCount = this->texCount;
	if (ImGui::InputInt("texture count", &texCount, 1)) {
		this->texCount = texCount;
	}

	ImGui::InputTextMultiline("Source Code", (char*)sourceCode.data(), sourceCode.size() - 1, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), 0);
	ImGui::Checkbox("compiled", &compiled);
	static std::string _lastCompileError{};
	if (ImGui::Button("Compile")) {
		std::string baseStr;
		int i = 0;
		for (uint32_t len : ubos) {
			baseStr += asString2<0>("cbuffer _b", i, ":register(b", i, "({\nfloat4 u", i, '[', len, "];\n};\n");
			i++;
		}

		for (uint32_t tid = 0; tid < this->texCount; tid++) {
			baseStr += asString2<0>("Texture2D _t", tid, " :register(t", tid, ");\n");
			baseStr += asString2<0>("SamplerState _s", tid, " :register(s", tid, ");\n");
		}

		for (uint32_t bid = 0; bid < this->sboCount; bid++) {
			baseStr += asString2<0>("StructuredBuffer<float4> _b", bid + this->texCount, ":register(t", bid + this->texCount, ");\n");
		}
		baseStr += sourceCode.data();

		if (!compileShaderUI(baseStr.data(), baseStr.size())) {
			ImGui::OpenPopup("compile error");
			_lastCompileError = getLastCompileError();
		}
	}

	if (ImGui::BeginPopup("compile error")) {
		ImGui::Text(_lastCompileError.c_str());
		ImGui::EndPopup();
	}
}

void VertexShader::draw() {
	ImGui::PushID(this);
	ImGui::Text("Vertex Shader: %p", shader);
	drawShaderResourceUI(shader);
	ImGui::PopID();
}

bool VertexShader::compileShaderUI(const char* data, size_t size) {
	auto p = createVertexShader(data, size);
	if (shader) { shader->Release(); }
	shader = p;
	return p;
}

const std::string& getLastCompileError() {
	return lastError;
}

void FragmentShader::draw() {
	ImGui::PushID(this);
	ImGui::Text("Pixel Shader: %p", shader);
	drawShaderResourceUI(shader);
	ImGui::PopID();
}

bool FragmentShader::compileShaderUI(const char* data, size_t size) {
	auto p = createPixelShader(data, size);
	if (shader) { shader->Release(); }
	shader = p;
	return p;
}

void ComputeShader::draw() {
	ImGui::PushID(this);
	ImGui::Text("Compute Shader: %p", shader);
	drawShaderResourceUI(shader);
	ImGui::PopID();
}

bool ComputeShader::compileShaderUI(const char* data, size_t size) {
	auto p = createComputeShader(data, size);
	if (shader) { shader->Release(); }
	shader = p;
	return p;
}
