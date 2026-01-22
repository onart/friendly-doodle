#ifndef __SHADER_H__
#define __SHADER_H__

#include <d3d11.h>
#include <vector>
#include <list>
#include <string>
#include <memory>

#include "stream.hpp"

ID3D11VertexShader* createVertexShader(const void* data, size_t size);
ID3D11PixelShader* createPixelShader(const void* data, size_t size);
ID3D11ComputeShader* createComputeShader(const void* data, size_t size);
const std::string& getLastCompileError();

class Shader {
public:
	size_t getBinSize();
	bool serialize(stream& s);
	template<class Derived>
	inline static std::shared_ptr<Derived> deserialize(stream& s);
protected:
	bool commonDeserialize(stream& s);
	Shader() = default;
	void drawShaderResourceUI(bool);
	virtual bool compileShaderUI(const char* data, size_t size) = 0;

	uint32_t texCount{};
	uint32_t sboCount{};
	std::list<uint32_t> ubos{};
	std::vector<char> sourceCode;
};

class VertexShader: public Shader {
	public:
	friend class ResourceManager;
	static std::shared_ptr<VertexShader> create() {
		return std::shared_ptr<VertexShader>(new VertexShader());
	}
	~VertexShader() {
		if (shader) shader->Release();
	}
	void draw();
	void bind();
private:
	VertexShader() { sourceCode.resize(16384); }
	bool compileShaderUI(const char* data, size_t size) override;
	ID3D11VertexShader* shader{};
};

class FragmentShader: public Shader {
public:
	friend class ResourceManager;
	static std::shared_ptr<FragmentShader> create() {
		return std::shared_ptr<FragmentShader>(new FragmentShader());
	}
	~FragmentShader() {
		if (shader) shader->Release();
	}
	void draw();
	void bind();
private:
	FragmentShader() { sourceCode.resize(16384); }
	ID3D11PixelShader* shader{};
	bool compileShaderUI(const char* data, size_t size) override;
};

class ComputeShader : public Shader {
public:
	friend class ResourceManager;
	static std::shared_ptr<ComputeShader> create() {
		return std::shared_ptr<ComputeShader>(new ComputeShader());
	}
	~ComputeShader() {
		if (shader) shader->Release();
	}
	void draw();
	void bind();
private:
	ComputeShader() { sourceCode.resize(16384); }
	ID3D11ComputeShader* shader{};
	bool compileShaderUI(const char* data, size_t size) override;
};

template<class Derived>
std::shared_ptr<Derived> Shader::deserialize(stream& s) {
	static_assert(std::is_base_of_v<Shader, Derived>);
	auto ret = Derived::create();
	if (!ret->commonDeserialize(s)) return {};
	return ret;
}

#endif // !__SHADER_H__

