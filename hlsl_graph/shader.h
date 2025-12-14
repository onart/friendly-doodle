#ifndef __SHADER_H__
#define __SHADER_H__

#include <d3d11.h>
#include <vector>
#include <list>
#include <string>
#include <memory>

ID3D11VertexShader* createVertexShader(const void* data, size_t size);
ID3D11PixelShader* createPixelShader(const void* data, size_t size);
ID3D11ComputeShader* createComputeShader(const void* data, size_t size);

class VertexShader {
	public:
	friend class ResourceManager;
	static std::shared_ptr<VertexShader> create() {
		return std::shared_ptr<VertexShader>(new VertexShader());
	}
	~VertexShader() {
		if (shader) shader->Release();
	}
	void draw();
private:
	VertexShader():sourceCode(16384) {}
	ID3D11VertexShader* shader{};
	std::list<std::shared_ptr<class ShaderBufferObject>> buffers{};
	std::list<std::shared_ptr<class UBO>> ubos{};
	std::vector<char> sourceCode;
};

#endif // !__SHADER_H__

