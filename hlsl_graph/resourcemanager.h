#ifndef __RESOURCE_MANAGER_H__
#define __RESOURCE_MANAGER_H__

#include <map>
#include <string>
#include <memory>
#include <filesystem>

class ResourceManager {
public:
	void clear();
	void draw();
	void save(const std::filesystem::path& p);
	bool load(const std::filesystem::path& p);
	auto getUBO(const std::string& name) {
		auto it = ubos.find(name);
		if (it != ubos.end()) {
			return it->second;
		}
		return std::shared_ptr<class UBO>{};
	}
	auto getTexture(const std::string& name) {
		auto it = textures.find(name);
		if (it != textures.end()) {
			return it->second;
		}
		return std::shared_ptr<class ShaderBufferObject>{};
	}
	auto getVertexShader(const std::string& name) {
		auto it = vertexShaders.find(name);
		if (it != vertexShaders.end()) {
			return it->second;
		}
		return std::shared_ptr<class VertexShader>{};
	}
	auto getPixelShader(const std::string& name) {
		auto it = pixelShaders.find(name);
		if (it != pixelShaders.end()) {
			return it->second;
		}
		return std::shared_ptr<class FragmentShader>{};
	}
	auto getComputeShader(const std::string& name) {
		auto it = computeShaders.find(name);
		if (it != computeShaders.end()) {
			return it->second;
		}
		return std::shared_ptr<class ComputeShader>{};
	}
	std::shared_ptr<class UBO> addUBOUI(bool reset);
	std::shared_ptr<class ShaderBufferObject> addSBOUI(bool reset);
private:
	std::filesystem::path drawExplore(const char* name, const char** extFilter, int filterCount);
	std::map<std::string, std::shared_ptr<class UBO>> ubos;
	std::map<std::string, std::shared_ptr<class ShaderBufferObject>> textures;
	std::map<std::string, std::shared_ptr<class VertexShader>> vertexShaders;
	std::map<std::string, std::shared_ptr<class FragmentShader>> pixelShaders;
	std::map<std::string, std::shared_ptr<class ComputeShader>> computeShaders;
	std::map<std::string, std::shared_ptr<struct Node>> pipelines;
	bool showResource{};
	bool showShader{};
	std::filesystem::path currentProject{};
	std::filesystem::path explore{};
	std::filesystem::path exploreSelected{};
};

extern ResourceManager mgr;

#endif // !__RESOURCE_MANAGER_H__

