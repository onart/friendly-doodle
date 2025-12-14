#include "ubo.h"
#include "d3ddev.h"
#include <sstream>
#include "imgui.h"

bool UBO::init(size_t size) {
	if (buffer) {
		return false;
	}
	D3D11_BUFFER_DESC desc{};
	desc.ByteWidth = static_cast<UINT>(size * 16);
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	HRESULT hr = D3D11Device::getDevice()->CreateBuffer(&desc, nullptr, &buffer);
	if (FAILED(hr)) {
		buffer = nullptr;
		return false;
	}
	data.resize(size);
	_meta.resize(size);
	for (int i = 0; i < size; i++) {
		_meta[i].type = F32;
		_meta[i].name = "v" + std::to_string(i);
	}
	return true;
}

std::string UBO::toPrimaryCode(size_t binding) {
	std::stringstream ss;
	ss << "cbuffer _b" << binding << " :register(b" << binding << ") {\n";
	for (size_t i = 0; i < data.size(); ++i) {
		switch (_meta[i].type) {
		case F32:
			ss << "float4 ";
			break;
		case U32:
			ss << "uint4 ";
			break;
		case I32:
			ss << "int4 ";
			break;
		}
		ss << _meta[i].name << ";\n";
	}
	
	ss << "};\n";
	return ss.str();
}

void UBO::draw() {
	ImGui::PushID(this);
	const ImGuiDataType typeArray[] = { ImGuiDataType_Float, ImGuiDataType_U32, ImGuiDataType_S32 };
	const char* formatArray[] = {"%.3f", "%u", "%d"};
	const float stepArray[] = { 0.1f, 1.0f, 1.0f };
	const char* formatNames[] = { "F32", "U32", "I32" };
	for (size_t i = 0; i < data.size(); ++i) {
		auto& meta = _meta[i];
		auto& dat = data[i];
		dirty = ImGui::DragScalarN(meta.name.c_str(), typeArray[meta.type], &dat, 4, stepArray[meta.type], nullptr, nullptr, formatArray[meta.type], ImGuiSliderFlags_None) || dirty;
		ImGui::SameLine();
		ImGui::PushID(i);
		if (ImGui::BeginCombo("change type", formatNames[meta.type])) {
			for (int t = 0; t < 3; ++t) {
				if (ImGui::Selectable(formatNames[t], meta.type == static_cast<vecType>(t))) {
					meta.type = static_cast<vecType>(t);
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopID();
	}
	ImGui::PopID();
}

void UBO::update(){
	if (!dirty) {
		return;
	}
	ID3D11DeviceContext* ctx = D3D11Device::getContext();
	ctx->UpdateSubresource(buffer, 0, nullptr, data.data(), 0, 0);
	dirty = false;
}