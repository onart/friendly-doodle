#ifndef __RESOURCE_MANAGER_H__
#define __RESOURCE_MANAGER_H__

#include <map>
#include <string>
#include <memory>

class ResourceManager {
public:
	void clear();
	void draw();
private:
	std::map<std::string, std::shared_ptr<class UBO>> ubos;
	std::map<std::string, std::shared_ptr<class Texture2D>> textures;
	std::map<std::string, std::shared_ptr<class VertexShader>> vertexShaders;
	std::map<std::string, std::shared_ptr<class PixelShader>> pixelShaders;

};

#endif // !__RESOURCE_MANAGER_H__

