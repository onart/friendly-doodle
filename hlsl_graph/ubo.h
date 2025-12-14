#ifndef __UBO_H__
#define __UBO_H__

#include <d3d11.h>
#include <memory>
#include <vector>
#include <string>

class UBO {
public:
	friend class ResourceManager;
	enum vecType {
		F32,
		U32,
		I32
	};
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

	void setName(const std::string& name, int idx, vecType type = F32) {
		if(_meta.size() <= static_cast<size_t>(idx)) {
			return;
		}
		_meta[idx].name = name;
		_meta[idx].type = type;
	}

	void setData16(int idx, void* _16B) {
		if (data.size() <= static_cast<size_t>(idx)) {
			return;
		}
		std::memcpy(&data[idx], _16B, 16);
		dirty = true;
	}

	void draw();

private:
	bool init(size_t sizeIn16Bytes);
	std::string toPrimaryCode(size_t binding);
	void update();
	
	ID3D11Buffer* buffer{};
	union v16 {
		float f32[4]{};
		uint32_t u32[4];
		int32_t i32[4];
	};
	struct meta {
		vecType type;
		std::string name;
	};
	std::vector<v16> data;
	std::vector<meta> _meta;
	bool dirty = true;
};

#endif
