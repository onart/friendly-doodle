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
		_meta[i].name = "v" + std::to_string(i);
	}
	return true;
}

std::string UBO::toPrimaryCode(size_t binding) {
	std::stringstream ss;
	ss << "cbuffer _b" << binding << " :register(b" << binding << ") {\n";
	for (size_t i = 0; i < data.size(); ++i) {
		ss << "float4 " << _meta[i].name << ";\n";
	}
	
	ss << "};\n";
	return ss.str();
}

size_t UBO::getBinSize() {
	return data.size() * sizeof(data[0]) + sizeof(size_t);
}

bool UBO::serialize(stream& s) {
	const size_t size = data.size() * sizeof(data[0]);
	s.write(size);
	s.writeRaw(data.data(), data.size() * sizeof(data[0]));
	return !s.hadFault();
}

std::shared_ptr<UBO> UBO::deserialize(stream& s) {
	size_t sz = s.read<size_t>();
	if (s.hadFault()) return {};
	if (sz % 16) return {};
	auto ret = create(sz / 16);
	s.readRaw(ret->data.data(), sz);
	if (s.hadFault()) return {};
	return ret;
}

void UBO::draw() {
	ImGui::PushID(this);
	const ImGuiDataType typeArray[] = { ImGuiDataType_Float, ImGuiDataType_U32, ImGuiDataType_S32 };
	for (size_t i = 0; i < data.size(); ++i) {
		auto& meta = _meta[i];
		auto& dat = data[i];
		dirty = ImGui::DragScalarN(meta.name.c_str(), ImGuiDataType_Float, &dat, 4, 0.01f, nullptr, nullptr, "%.3f", ImGuiSliderFlags_None) || dirty;
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