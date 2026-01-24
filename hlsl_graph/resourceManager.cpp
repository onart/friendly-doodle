#define _CRT_SECURE_NO_WARNINGS
#include "resourcemanager.h"
#include "imgui.h"
#include "ubo.h"
#include "image.h"
#include "d3ddev.h"
#include "shader.h"
#include "stream.hpp"
#include "Node.h"

void ResourceManager::clear() {
	ubos.clear();
	textures.clear();
	vertexShaders.clear();
	pixelShaders.clear();
	computeShaders.clear();
	pipelines.clear();
}

static char inputName[2][4096 * 3]{};
static int inputSize = 1;
static int comboIndex[16]{};
static int input4[4]{};

const static struct __fmt {
	const char* name;
	DXGI_FORMAT fmt;
} fmts[] = { {"RGBA8", DXGI_FORMAT_R8G8B8A8_UNORM}, {"RGBA32F", DXGI_FORMAT_R32G32B32A32_FLOAT} };

std::filesystem::path ResourceManager::drawExplore(const char* name, const char** filter, int filterCount) {
	std::filesystem::path result;
	if (explore.empty()) { explore = std::filesystem::current_path(); }
	if (!std::filesystem::is_directory(explore)) {
		explore = explore.parent_path();
	}
	bool confirmed = false;
	ImGui::SetNextWindowSize({640, 480}, ImGuiCond_Appearing);
	if (ImGui::Begin(name)) {
		static char u8p[4096];
		ImGui::InputText("##path", u8p, sizeof(u8p));
		ImGui::SameLine();
		std::filesystem::path newp = std::filesystem::u8path(u8p);
		if (ImGui::Button("browse")) {
			if (std::filesystem::exists(newp)) {
				if (std::filesystem::is_directory(newp)) {
					explore = newp;
				}
				else {
					explore = newp.parent_path();
				}
			}
			else {
				std::string str = explore.u8string();
				std::memcpy(u8p, str.data(), str.size());
				u8p[str.size()] = 0;
			}
		}
		if (ImGui::Selectable("..", exploreSelected == "..")) {
			exploreSelected = explore.parent_path();
		}
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			explore = explore.parent_path();
			exploreSelected.clear();
			std::string str = explore.u8string();
			std::memcpy(u8p, str.data(), str.size());
			u8p[str.size()] = 0;
		}
		for (auto& entry : std::filesystem::directory_iterator(explore)) {
			const auto& p = entry.path();
			if (filterCount) {
				bool accepted = false;
				for (int i = 0; i < filterCount; i++) {
					if (filter[i][0] == 0) {
						if (std::filesystem::is_directory(p)) {
							accepted = true;
							break;
						}
					}
					if (p.extension() == filter[i]) {
						accepted = true;
						break;
					}
				}
				if (!accepted) {
					continue;
				}
			}
			if (ImGui::Selectable(p.filename().u8string().c_str(), p == exploreSelected)) {
				exploreSelected = p;
			}
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				if (std::filesystem::is_directory(p)) {
					std::string str = p.u8string();
					std::memcpy(u8p, str.data(), str.size());
					u8p[str.size()] = 0;
					explore = p;
				}
				else {
					ImGui::End();
					return p;
				}
				break;
			}
		}

		static char u8p2[256];
		if (exploreSaveMode) {
			ImGui::InputText("##name", u8p2, sizeof(u8p2));
			ImGui::SameLine();
			if (ImGui::Button("Save Here")) {
				result = explore / u8p2;
				result.replace_extension(".fdd");
				if (std::filesystem::exists(result)) {
					if (std::filesystem::is_directory(result)) {
						result.clear();
					}
					else {
						ImGui::OpenPopup("overwrite");
					}
				}
			}
			if (ImGui::BeginPopup("overwrite")) {
				result = explore / u8p2;
				result.replace_extension(".fdd");
				ImGui::Text("%s: Already exsists. Overwrite?", result.u8string().c_str());
				bool confirm = false;
				if (ImGui::Button("Yes")) {
					confirm = true;
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::Button("No")) {
					ImGui::CloseCurrentPopup();
				}
				if (!confirm) {
					result.clear();
				}
				else {
					u8p[0] = 0;
					u8p2[0] = 0;
				}
				ImGui::EndPopup();
			}
		}
	}
	ImGui::End();
	return result;
}

void ResourceManager::draw(){
	ImGui::PushID(this);
	ImGui::Text("Project: %s", currentProject.u8string().c_str());
	if (ImGui::Button("Save")) {
		if (currentProject.empty()) {
			explore = std::filesystem::current_path();
			exploreSaveMode = true;
		}
		else {
			save(currentProject);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Save As")) {
		explore = std::filesystem::current_path();
		exploreSaveMode = true;
	}
	if (ImGui::Button("Load")) {
		explore = std::filesystem::current_path();
		exploreSaveMode = false;
	}

	if (!explore.empty()) {
		const char* filter[] = { u8"", u8".fdd" };
		auto path = drawExplore("Save as..", filter, std::size(filter));
		if (!path.empty()) {
			explore.clear();
			if (exploreSaveMode) {
				save(path);
				currentProject = path;
			}
			else {
				if (load(path)) {
					currentProject = path;
				}
				else {
					currentProject = "";
				}
			}
		}
	}

	ImGui::Checkbox("resource window", &showResource);
	ImGui::Checkbox("shader window", &showShader);
	ImGui::Checkbox("pipeline window", &showPipeline);
	if (showResource) {
		if (ImGui::Begin("resources", &showResource)) {
			ImGui::PushID("ubo");
			if (ImGui::TreeNode("UBO")) {
				for(auto it = ubos.begin(); it != ubos.end(); ) {
					auto& [name, ubo] = *it;
					ImGui::PushID(name.c_str());
					if (ImGui::TreeNode(name.c_str())) {
						ubo->draw();
						if (ImGui::Button("x")) {
							it = ubos.erase(it);
							ImGui::TreePop();
							ImGui::PopID();
							continue;
						}
						ImGui::TreePop();
					}
					ImGui::PopID();
					++it;
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
				for (auto it = textures.begin(); it != textures.end(); ) {
					auto& [name, texture] = *it;
					ImGui::PushID(name.c_str());
					if (ImGui::TreeNode(name.c_str())) {
						texture->show(256, 256);
						if (ImGui::Button("x")) {
							it = textures.erase(it);
							ImGui::TreePop();
							ImGui::PopID();
							continue;
						}
						ImGui::TreePop();
					}
					ImGui::PopID();
					++it;
				}
				if (ImGui::Button("+")) {
					ImGui::OpenPopup("Add Shader Buffer Object");
				}
				if (ImGui::BeginPopup("Add Shader Buffer Object")) {
					ImGui::InputText("Name", inputName[0], sizeof(inputName[0]));
					const char* names[] = { "Resource", "Render Target", "UAV Texture", "UAV Structured Buffer" };
					if (ImGui::BeginCombo("Type", names[comboIndex[0]])) {
						for (int i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
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
	}
	if (showShader) {
		if (ImGui::Begin("Shaders")) {
			ImGui::PushID("vs");
			if (ImGui::TreeNode("Vertex Shaders")) {
				for (auto it = vertexShaders.begin(); it != vertexShaders.end(); ) {
					auto& [name, vs] = *it;
					ImGui::PushID(name.c_str());
					if (ImGui::TreeNode(name.c_str())) {
						vs->draw();
						if (ImGui::Button("x")) {
							it = vertexShaders.erase(it);
							ImGui::TreePop();
							ImGui::PopID();
							continue;
						}
						ImGui::TreePop();
					}
					ImGui::PopID();
					++it;
				}
				if (ImGui::Button("+")) {
					ImGui::OpenPopup("New Vertex Shader");
				}
				if (ImGui::BeginPopup("New Vertex Shader")) {
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
			ImGui::PushID("fs");
			if (ImGui::TreeNode("Pixel Shaders")) {
				for (auto it = pixelShaders.begin(); it != pixelShaders.end(); ) {
					auto& [name, vs] = *it;
					ImGui::PushID(name.c_str());
					if (ImGui::TreeNode(name.c_str())) {
						vs->draw();
						if (ImGui::Button("x")) {
							it = pixelShaders.erase(it);
							ImGui::TreePop();
							ImGui::PopID();
							continue;
						}
						ImGui::TreePop();
					}
					ImGui::PopID();
					++it;
				}
				if (ImGui::Button("+")) {
					ImGui::OpenPopup("New Pixel Shader");
				}
				if (ImGui::BeginPopup("New Pixel Shader")) {
					ImGui::InputText("Name", inputName[0], sizeof(inputName[0]));
					if (ImGui::Button("confirm")) {
						auto vs = FragmentShader::create();
						if (vs) {
							pixelShaders.insert({ inputName[0], vs });
						}
						inputName[0][0] = 0;
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
				ImGui::TreePop();
			}
			ImGui::PopID(); // fs
			ImGui::PushID("cs");
			if (ImGui::TreeNode("Compute Shaders")) {
				for (auto it = computeShaders.begin(); it != computeShaders.end(); ) {
					auto& [name, vs] = *it;
					ImGui::PushID(name.c_str());
					if (ImGui::TreeNode(name.c_str())) {
						vs->draw();
						if (ImGui::Button("x")) {
							it = computeShaders.erase(it);
							ImGui::TreePop();
							ImGui::PopID();
							continue;
						}
						ImGui::TreePop();
					}
					ImGui::PopID();
					++it;
				}
				if (ImGui::Button("+")) {
					ImGui::OpenPopup("New Compute Shader");
				}
				if (ImGui::BeginPopup("New Compute Shader")) {
					ImGui::InputText("Name", inputName[0], sizeof(inputName[0]));
					if (ImGui::Button("confirm")) {
						auto vs = ComputeShader::create();
						if (vs) {
							computeShaders.insert({ inputName[0], vs });
						}
						inputName[0][0] = 0;
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
				ImGui::TreePop();
			}
			ImGui::PopID(); // fs
		}
		ImGui::End(); // shader
	}

	if (showPipeline) {
		if (ImGui::Begin("Pipelines")) {
			for (auto it = pipelines.begin(); it != pipelines.end(); ) {
				auto& [name, p] = *it;
				ImGui::PushID(name.c_str());
				ImGui::Checkbox((name + "##check").c_str(), &p.second);
				if (p.second) {
					if (ImGui::Begin(name.c_str(), &p.second)) {
						p.first->draw();
						if (ImGui::Button("x")) {
							it = pipelines.erase(it);
							ImGui::End();
							ImGui::PopID();
							continue;
						}
					}
					ImGui::End();
				}
				ImGui::PopID();
				++it;
			}
			ImGui::Separator();
			static char nameBuffer[256]{};
			if (auto newNode = Node::drawAdd(nameBuffer, sizeof(nameBuffer))) {
				if (nameBuffer[0] != '\0') {
					pipelines.insert({ nameBuffer, {newNode, true} });
					nameBuffer[0] = 0;
				}
			}
		}
		ImGui::End();
	}

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

void ResourceManager::save(const std::filesystem::path& name) {
	std::map<std::string, std::shared_ptr<struct Node>> pipelines;
	std::vector<uint8_t> data;

	size_t size = 36;
	
	size += 8;
	for (auto& [name, ubo] : ubos) {
		size += ubo->getBinSize();
		size += name.size() + 16;
	}

	size += 8;
	for (auto& [name, texture] : textures) {
		size += texture->getBinSize();
		size += name.size() + 16;
	}

	size += 8;
	for (auto& [name, s] : vertexShaders) {
		size += s->getBinSize();
		size += name.size() + 16;
	}

	size += 8;
	for (auto& [name, s] : pixelShaders) {
		size += s->getBinSize();
		size += name.size() + 16;
	}

	size += 8;
	for (auto& [name, s] : computeShaders) {
		size += s->getBinSize();
		size += name.size() + 16;
	}
	
	size += 8;
	for (auto& [name, p] : pipelines) {
		size += p->getBinSize();
		size += name.size() + 16;
	}

	data.resize(size);
	stream st(data.data(), size);
	st.writeRaw("FDD\1", 4);
	st.writes((uint32_t)ubos.size(), (uint32_t)textures.size(), (uint32_t)vertexShaders.size(), (uint32_t)pixelShaders.size(), (uint32_t)computeShaders.size(), (uint32_t)pipelines.size());
	st.writes(size);

	auto writeName = [&st](const std::string& name) {
		size_t size = name.size();
		st.write(size);
		st.writeRaw(name.c_str(), size);
		st.write(size);
	};

	st.writes((uint32_t)ubos.size());
	for (auto& [name, ubo] : ubos) {
		writeName(name);
		if (!ubo->serialize(st)) return;
	}
	st.writes((uint32_t)st.tell());

	st.writes((uint32_t)textures.size());
	for (auto& [name, texture] : textures) {
		writeName(name);
		if (!texture->serialize(st)) return;
	}
	st.writes((uint32_t)st.tell());

	st.writes((uint32_t)vertexShaders.size());
	for (auto& [name, s] : vertexShaders) {
		writeName(name);
		if (!s->serialize(st)) return;
	}
	st.writes((uint32_t)st.tell());

	st.writes((uint32_t)pixelShaders.size());
	for (auto& [name, s] : pixelShaders) {
		writeName(name);
		if (!s->serialize(st)) return;
	}
	st.writes((uint32_t)st.tell());

	st.writes((uint32_t)computeShaders.size());
	for (auto& [name, s] : computeShaders) {
		writeName(name);
		if (!s->serialize(st)) return;
	}
	st.writes((uint32_t)st.tell());

	st.writes((uint32_t)pipelines.size());
	for (auto& [name, p] : pipelines) {
		writeName(name);
		if (!p->serialize(st)) return;
	}
	st.writes((uint32_t)st.tell());

	FILE* fp = fopen(name.string().c_str(), "wb");
	if (!fp) {
		return;
	}
	fwrite(data.data(), 1, data.size(), fp);
	fclose(fp);
}

bool ResourceManager::load(const std::filesystem::path& name) {
	clear();
	FILE* fp = fopen(name.string().c_str(), "rb");
	if (!fp) {
		return false;
	}
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	std::vector<uint8_t> data(size);
	fread(data.data(), 1, size, fp);
	fclose(fp);
	
	stream reader(data.data(), data.size());
	char marker[4]{};
	reader.readRaw(marker, 4);
	if (std::memcmp(marker, "FDD\1", sizeof(marker)) != 0) {
		return false;
	}

	auto [uboCount, texCount, vsCount, fsCount, csCount, pipeCount] = reader.reads<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>();
	if (reader.hadFault()) return false;
	if (reader.read<size_t>() != size) return false;

	auto readName = [&reader]() -> std::string {
		size_t size = reader.read<size_t>();
		std::vector<char> name(size);
		reader.readRaw(name.data(), size);
		if (reader.read<size_t>() != size) {
			return "";
		}
		return std::string(name.data(), size);
	};

	auto checkPos = [&reader]() {
		uint32_t pos = reader.read<uint32_t>();
		return reader.tell() == pos + sizeof(uint32_t);
	};

	if (reader.read<uint32_t>() != uboCount) return false;
	for (uint32_t i = 0; i < uboCount; i++) {
		std::string name = readName();
		if (name.empty()) return false;
		auto obj = UBO::deserialize(reader);
		if (!obj) return false;
		ubos[name] = std::move(obj);
	}
	if (!checkPos()) return false;
	if (ubos.size() != uboCount) return false; // overlapping name

	if (reader.read<uint32_t>() != texCount) return false;
	for (uint32_t i = 0; i < texCount; i++) {
		std::string name = readName();
		if (name.empty()) return false;
		auto obj = ShaderBufferObject::deserialize(reader);
		if (!obj) return false;
		textures[name] = obj;
	}
	if (!checkPos()) return false;
	if (textures.size() != texCount) return false; // overlapping name

	if (reader.read<uint32_t>() != vsCount) return false;
	for (uint32_t i = 0; i < vsCount; i++) {
		std::string name = readName();
		if (name.empty()) return false;
		auto obj = Shader::deserialize<VertexShader>(reader);
		if (!obj) return false;
		vertexShaders[name] = obj;
	}
	if (!checkPos()) return false;
	if (vertexShaders.size() != vsCount) return false; // overlapping name

	if (reader.read<uint32_t>() != fsCount) return false;
	for (uint32_t i = 0; i < fsCount; i++) {
		std::string name = readName();
		if (name.empty()) return false;
		auto obj = Shader::deserialize<FragmentShader>(reader);
		if (!obj) return false;
		pixelShaders[name] = obj;
	}
	if (!checkPos()) return false;
	if (pixelShaders.size() != fsCount) return false; // overlapping name

	if (reader.read<uint32_t>() != csCount) return false;
	for (uint32_t i = 0; i < csCount; i++) {
		std::string name = readName();
		if (name.empty()) return false;
		auto obj = Shader::deserialize<ComputeShader>(reader);
		if (!obj) return false;
		computeShaders[name] = obj;
	}
	if (!checkPos()) return false;
	if (computeShaders.size() != csCount) return false; // overlapping name

	if (reader.read<uint32_t>() != pipeCount) return false;
	for (uint32_t i = 0; i < pipeCount; i++) {
		std::string name = readName();
		if (name.empty()) return false;
		auto obj = Node::deserialize(reader);
		if (!obj) return false;
		pipelines[name] = { obj, false };
	}
	if (!checkPos()) return false;
	if (pipelines.size() != pipeCount) return false; // overlapping name
	
	return true;
}