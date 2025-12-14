#include "resourcemanager.h"
#include "imgui.h"
#include "ubo.h"
#include "image.h"
#include "d3ddev.h"

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
				ubos.insert({ inputName[0], ubo});
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
	if(ImGui::TreeNode("Texture")) {
		for (auto& [name, texture] : textures) {
			ImGui::PushID(name.c_str());
			if (ImGui::TreeNode(name.c_str())) {
				texture->show(256, 256);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		if (ImGui::Button("+")) {
			ImGui::OpenPopup("Add Texture");
		}
		if (ImGui::BeginPopup("Add Texture")) {
			ImGui::InputText("Name", inputName[0], sizeof(inputName[0]));
			const char* names[] = { "Resource", "Render Target", "UAV" };
			if (ImGui::BeginCombo("Type", names[comboIndex[0]])) {
				for (int i = 0; i < 3; i++) {
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
					for(auto&[name, fmt]: fmts){
						bool selected = ImGui::Selectable(name, fmts[comboIndex[1]].fmt == fmt);
						if (selected) comboIndex[1] = _i;
						_i++;
					}
					ImGui::EndCombo();
				}
				break;
			}
			default:
				break;
			}
			if (ImGui::Button("confirm")) {
				if(inputName[0][0] != 0) {
					std::shared_ptr<Texture2D> texture{};
					switch (comboIndex[0])
					{
					case 0:
					{
						texture = Texture2D::create(inputName[1]);
						break;
					}
					case 1:
					{
						texture = Texture2D::createTarget(static_cast<UINT>(input4[0]), static_cast<UINT>(input4[1]), fmts[comboIndex[1]].fmt);
						break;
					}
					case 2:
					{
						texture = Texture2D::createTarget(static_cast<UINT>(input4[0]), static_cast<UINT>(input4[1]), fmts[comboIndex[1]].fmt);
						break;
					}
					}
					if (texture) {
						textures.insert({ inputName[0], texture });
					}
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
		ImGui::TreePop();
	}

	ImGui::PopID(); // texture

	ImGui::PopID(); // this
}