#include "resourcemanager.h"
#include "imgui.h"
#include "ubo.h"
#include "image.h"
#include "d3ddev.h"
#include "shader.h"

void ResourceManager::clear() {
	ubos.clear();
	textures.clear();
	vertexShaders.clear();
	pixelShaders.clear();
}

static char inputName[2][4096 * 3]{};
static int inputSize = 1;
static int comboIndex[16]{};
static int input4[4]{};

const static struct __fmt {
	const char* name;
	DXGI_FORMAT fmt;
} fmts[] = { {"RGBA8", DXGI_FORMAT_R8G8B8A8_UNORM}, {"RGBA32F", DXGI_FORMAT_R32G32B32A32_FLOAT} };

void ResourceManager::draw(){
	ImGui::PushID(this);
	if (ImGui::Begin("resources")) {
		ImGui::PushID("ubo");
		if (ImGui::TreeNode("UBO")) {
			for (auto& [name, ubo] : ubos) {
				ImGui::PushID(name.c_str());
				if (ImGui::TreeNode(name.c_str())) {
					ubo->draw();
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			if (ImGui::Button("+")) {
				ImGui::OpenPopup("Add UBO");
			}
			if (ImGui::BeginPopup("Add UBO")) {
				ImGui::InputText("Name", inputName[0], sizeof(inputName[0]));
				ImGui::DragInt("Size (16B units)", &inputSize, 1.0f, 1, 4096);
				if (ImGui::Button("confirm")) {
					auto ubo = UBO::create(static_cast<size_t>(inputSize));
					ubos.insert({ inputName[0], ubo });
					inputName[0][0] = 0;
					inputSize = 1;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::TreePop();
		}
		ImGui::PopID(); // ubo

		ImGui::PushID("texture");
		if (ImGui::TreeNode("Shader Buffer Object")) {
			for (auto& [name, texture] : textures) {
				ImGui::PushID(name.c_str());
				if (ImGui::TreeNode(name.c_str())) {
					texture->show(256, 256);
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			if (ImGui::Button("+")) {
				ImGui::OpenPopup("Add Shader Buffer Object");
			}
			if (ImGui::BeginPopup("Add Shader Buffer Object")) {
				ImGui::InputText("Name", inputName[0], sizeof(inputName[0]));
				const char* names[] = { "Resource", "Render Target", "UAV Texture", "UAV Structured Buffer" };
				if (ImGui::BeginCombo("Type", names[comboIndex[0]])) {
					for (int i = 0; i < sizeof(names)/sizeof(names[0]); i++) {
						bool selected = ImGui::Selectable(names[i], comboIndex[0] == i);
						if (selected) comboIndex[0] = i;
					}
					ImGui::EndCombo();
				}
				switch (comboIndex[0])
				{
				case 0:
				{
					ImGui::InputText("Path", inputName[1], sizeof(inputName[1]));
					if (ImGui::IsItemHovered() && D3D11Device::isNewDropInCurrentFrame()) {
						auto& paths = D3D11Device::getRecentDroppedPaths();
						if (paths.size() > 0) {
							std::string pathStr = paths[0].u8string();
							std::memcpy(inputName[1], pathStr.c_str(), pathStr.size());
							inputName[1][pathStr.size()] = 0;
						}
					}
					break;
				}
				case 1:
				case 2:
				{
					ImGui::DragInt2("Size", input4);
					if (ImGui::BeginCombo("Format", fmts[comboIndex[1]].name)) {
						int _i = 0;
						for (auto& [name, fmt] : fmts) {
							bool selected = ImGui::Selectable(name, fmts[comboIndex[1]].fmt == fmt);
							if (selected) comboIndex[1] = _i;
							_i++;
						}
						ImGui::EndCombo();
					}
					break;
				}
				case 3:
				{
					ImGui::DragInt("Size (bytes)", input4, 1, 16, INT32_MAX);
					ImGui::DragInt("Stride (bytes)", input4 + 1, 1, 1, INT32_MAX);
					break;
				}
				default:
					break;
				}
				if (ImGui::Button("confirm")) {
					if (inputName[0][0] != 0) {
						std::shared_ptr<ShaderBufferObject> texture{};
						switch (comboIndex[0])
						{
						case 0:
						{
							texture = ShaderBufferObject::create(inputName[1]);
							break;
						}
						case 1:
						{
							texture = ShaderBufferObject::createTarget(static_cast<UINT>(input4[0]), static_cast<UINT>(input4[1]), fmts[comboIndex[1]].fmt);
							break;
						}
						case 2:
						{
							texture = ShaderBufferObject::createTarget(static_cast<UINT>(input4[0]), static_cast<UINT>(input4[1]), fmts[comboIndex[1]].fmt);
							break;
						}
						case 3:
						{
							texture = ShaderBufferObject::createStructuredUAV(input4[0], input4[1]);
							break;
						}
						}
						if (texture) {
							textures.insert({ inputName[0], texture });
						}
						std::memset(input4, 0, sizeof(input4));
						inputName[0][0] = 0;
						inputName[1][0] = 0;
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::EndPopup();
			}
			ImGui::TreePop();
		}

		ImGui::PopID(); // texture
	}
	ImGui::End(); // resources

	if (ImGui::Begin("Shaders")) {
		ImGui::PushID("vs");
		if (ImGui::TreeNode("Vertex Shaders")) {
			for (auto& [name, vs] : vertexShaders) {
				ImGui::PushID(name.c_str());
				if (ImGui::TreeNode(name.c_str())) {
					vs->draw();
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			if (ImGui::Button("+")) {
				ImGui::OpenPopup("New Vertex Shader");
			}
			if(ImGui::BeginPopup("New Vertex Shader")) {
				ImGui::InputText("Name", inputName[0], sizeof(inputName[0]));
				if (ImGui::Button("confirm")) {
					auto vs = VertexShader::create();
					if (vs) {
						vertexShaders.insert({ inputName[0], vs });
					}
					inputName[0][0] = 0;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::TreePop();
		}
		ImGui::PopID(); // vs
	}
	ImGui::End(); // shader

	ImGui::PopID(); // this
}

std::shared_ptr<class UBO> ResourceManager::addUBOUI(bool reset) {
	static std::string selectedStr;
	if (reset) { selectedStr.clear();  return {}; }
	if (ImGui::BeginCombo("select UBO", selectedStr.c_str())) {
		for (auto& [name, ubo] : ubos) {
			bool selected = ImGui::Selectable(name.c_str(), name == selectedStr);
			if (selected) {
				selectedStr = name;
			}
		}
		ImGui::EndCombo();
	}
	auto it = ubos.find(selectedStr);
	if (it != ubos.end()) {
		return it->second;
	}
	return {};
}
std::shared_ptr<class ShaderBufferObject> ResourceManager::addSBOUI(bool reset) {
	static std::string selectedStr;
	if (reset) { selectedStr.clear();  return {}; }
	if (ImGui::BeginCombo("select SBO", selectedStr.c_str())) {
		for (auto& [name, sbo] : textures) {
			bool selected = ImGui::Selectable(name.c_str(), name == selectedStr);
			if (selected) {
				selectedStr = name;
			}
		}
		ImGui::EndCombo();
	}
	auto it = textures.find(selectedStr);
	if (it != textures.end()) {
		return it->second;
	}
	return {};
}