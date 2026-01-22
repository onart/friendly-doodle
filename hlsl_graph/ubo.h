#ifndef __UBO_H__
#define __UBO_H__

#include <d3d11.h>
#include <memory>
#include <vector>
#include <string>

#include "stream.hpp"

class UBO {
public:
	enum class BindOn {
		VertexShader = 0,
		PixelShader = 1,
		ComputeShader = 2,
	};
	friend class ResourceManager;
	std::shared_ptr<UBO> static create(size_t size) {
		UBO ubo;
		if (ubo.init(size)) {
			auto ret = std::shared_ptr<UBO>(new UBO());
			*ret = std::move(ubo);
			ubo.buffer = {};
			return ret;
		}
		return {};
	}
	~UBO() {
		if (buffer) buffer->Release();
	}

	void setName(const std::string& name, int idx) {
		if(_meta.size() <= static_cast<size_t>(idx)) {
			return;
		}
		_meta[idx].name = name;
	}

	void setData16(int idx, void* _16B) {
		if (data.size() <= static_cast<size_t>(idx)) {
			return;
		}
		std::memcpy(&data[idx], _16B, 16);
		dirty = true;
	}

	void draw();
	void bind(BindOn b, int slot);

	std::string toPrimaryCode(size_t binding);
	size_t getBinSize();
	bool serialize(stream& s);
	static std::shared_ptr<UBO> deserialize(stream& s);
private:
	bool init(size_t sizeIn16Bytes);
	void update();
	
	ID3D11Buffer* buffer{};
	union v16 {
		float f32[4]{};
		uint32_t u32[4];
		int32_t i32[4];
	};
	struct meta {
		std::string name;
	};
	std::vector<v16> data;
	std::vector<meta> _meta;
	bool dirty = true;
};

#endif
